/*
 *  FinalWordRhymeModel.cpp
 *
 *  Copyright 2013 by Joerg Tiedemann. All rights reserved.
 *
 *  This file is part of Docent, a document-level decoder for phrase-based
 *  statistical machine translation.
 *
 *  Docent is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
 *
 *  Docent is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  Docent. If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace std;

#include "Docent.h"
#include "DocumentState.h"
#include "FeatureFunction.h"
#include "SearchStep.h"
#include "FinalWordRhymeModel.h"

#include <boost/unordered_map.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/regex.hpp>

struct FinalWordRhymeModelState : public FeatureFunction::State, public FeatureFunction::StateModifications {
  FinalWordRhymeModelState(uint nsents) : 
    rhymeMap(nsents), 
    wordList(nsents), 
    rhymeList(nsents),
    logger_("FinalWordRhymeModelState") {
    docSize = nsents; 
    wordList.resize(docSize);
    rhymeList.resize(docSize);
    avgRhymeDistance = docSize;
    overallRhymeRepetition=0;
    overallRhymes=0;
  }

  uint docSize;
  float avgRhymeDistance;
  uint overallRhymeRepetition;
  uint overallRhymes;
  mutable Logger logger_;


  typedef boost::unordered_map<std::string,boost::dynamic_bitset<> > WordMap_;

  WordMap_ rhymeMap;                  // rhyme chains stored in bitsets
  boost::dynamic_bitset<> coverage;   // rhyme coverage vector

  std::vector<std::string> wordList;                 // list of sentence-final words
  std::vector< std::vector<std::string> > rhymeList; // list of sentence-final rhymes (syllables)

  // for each rhyme - store word frequencies of words that appear with this rhyme
  boost::unordered_map<std::string, boost::unordered_map<std::string,uint> > rhymeRepetition;
  // boost::unordered_map<std::string, uint > rhymeRepetition2;


  float currentScore;

  Float score() const {
    if (docSize>0){
      // return (0 - (float)docSize + float(coverage.count()))/docSize;
      float repetitionPenalty = 0;
      if (overallRhymes>0){
	repetitionPenalty = (float)overallRhymeRepetition/overallRhymes;
      }
      return (0 - (float)docSize + float(coverage.count()))/docSize - 
      	avgRhymeDistance/docSize - repetitionPenalty;
      // return (0 - (float)docSize + float(coverage.count()))/docSize - avgRhymeDistance/docSize;
    }
    return 0;
  }

  void updateScore() {
    updateCoverage();
    avgRhymeDistance = averageRhymeDistance();
    currentScore = score();
  }

  float averageRhymeDistance() const {
    uint dist=0;
    for(uint i = 0; i < docSize; i++){
      dist += rhymeDistance(i);
    }
    return (float)dist/docSize;
  }

  uint rhymeDistance(const uint &sentno) const {
    uint distance = docSize;
    if (rhymeList[sentno].size()){
      for (uint i=0;i<rhymeList[sentno].size();++i){
	WordMap_::const_iterator it = rhymeMap.find(rhymeList[sentno][i]);
	if (it != rhymeMap.end()){
	  uint pos = it->second.find_next(sentno);
	  if (pos != docSize){
	    if (pos-sentno < distance){
	      distance = pos-sentno;
	    }
	  }
	}
      }
    }
    return distance;
  }

  /*
  // return average repetition ratio for rhyming words
  float averageRepetitionRatio() const {
    for (WordMap_::const_iterator it = rhymeMap.begin(); it != rhymeMap.end(); ++it ) {
      uint  = it->second.find_first();
      while (
      if (it->second.count() > 1){
	
  }
  */

  void updateCoverage(){
    coverage = getCoverage();
  }

  boost::dynamic_bitset<> getCoverage(){
    boost::dynamic_bitset<> covered(docSize);
    covered.reset();
    for (WordMap_::const_iterator it = rhymeMap.begin(); it != rhymeMap.end(); ++it ) {
      if (it->second.count() > 1){
	boost::dynamic_bitset<> combined((covered^=it->second));
	covered = combined;
      }
    }
    return covered;
  }

  void printRepetitionTable() const {
    for (auto it1 = rhymeRepetition.begin(); it1 != rhymeRepetition.end(); ++it1){
      cout << it1->first << ":";
      for (auto it2 = it1->second.begin(); it2 != it1->second.end(); ++it2){
	cout << it2->first << "=" << it2->second << " ";
      }
      cout << endl;
    }
  }

  void printRhymeMap() const {
    for (WordMap_::const_iterator it = rhymeMap.begin(); it != rhymeMap.end(); ++it ) {
      if (it->second.count() > 1){
	cout << it->second << it->first << endl;
      }
    }
  }


  void setWord(const uint &sentno, const std::string &word, const std::string &rhyme){

    // TODO: really skip all cases where word = rhyme?
    if (rhyme == word){ return; }

    if (rhymeMap.find(rhyme) == rhymeMap.end()){
      rhymeMap[rhyme].resize(docSize);
    }
    rhymeMap[rhyme].set(sentno);
    if (rhymeMap[rhyme].count() > 1){
      overallRhymes++;
    }

    // increase word count for this rhyme pattern
    if (rhymeRepetition.find(rhyme) == rhymeRepetition.end()){
      rhymeRepetition[rhyme][word] = 1;
    }
    else{
      if (rhymeRepetition[rhyme].find(word) == rhymeRepetition[rhyme].end()){
	rhymeRepetition[rhyme][word] = 1;
      }
      else{
	rhymeRepetition[rhyme][word]++;
      }
    }

    if (rhymeRepetition[rhyme][word] > 1){
      overallRhymeRepetition++;
      if (logger_.loggable(debug)){
	LOG(logger_, debug,
	    "++++++ set " << word << " (" << rhyme << "in sentence " << sentno << ", repetition = " << overallRhymeRepetition);
	printRhymeMap();
	// printRepetitionTable();
      }
    }

    // update list of words corresponding list of rhymes
    wordList[sentno] = word;
    rhymeList[sentno].push_back(rhyme);
  }


  void resetWord(const uint &sentno, const std::string &word, const std::string &rhyme){

    // TODO: really skip all cases where word = rhyme?
    if (rhyme == word){ return; }


    if (rhymeMap.find(rhyme) == rhymeMap.end()){
      rhymeMap[rhyme].resize(docSize);
    }
    // unset rhyme pattern for this sentence
    else{
      uint nrWords = rhymeMap[rhyme].count();
      rhymeMap[rhyme].reset(sentno);
      // decrease number of rhymes if the count is below 2 now
      if ( rhymeMap[rhyme].count() < 2 && nrWords > 1 ){
	if (overallRhymes > 0){
	  overallRhymes--;
	}
      }
    }

    if ( rhymeRepetition.find(rhyme) != rhymeRepetition.end() ){
      if ( rhymeRepetition[rhyme].find(word) != rhymeRepetition[rhyme].end() ){
	if (rhymeRepetition[rhyme][word] > 1){
	  if (overallRhymeRepetition > 0){
	    overallRhymeRepetition--;
	  }
	  else{
	    LOG(logger_, debug,
		"???? repeated word but overall-count = 0 " << word << " (" 
		<< rhyme << ") in sentence " 
		<< sentno << ", repetition = " 
		<< rhymeRepetition[rhyme][word]);
	  }
	}
	if (rhymeRepetition[rhyme][word] > 0){
	  rhymeRepetition[rhyme][word]--;
	}
	else{
	    LOG(logger_, debug,
		"???? unset word but no count " << word << " (" 
		<< rhyme << ") in sentence " 
		<< sentno << ", repetition = " << overallRhymeRepetition);
	}
      }
      else{
	LOG(logger_, debug,
	    "???? non-existing word " << word << " (" << rhyme << ") in sentence " 
	    << sentno << ", repetition = " << overallRhymeRepetition);
      }
    }
    else{
      LOG(logger_, debug,
	  "???? non-existing rhyme pattern " << word << " (" << rhyme << ") in sentence " 
	  << sentno << ", repetition = " << overallRhymeRepetition);
    }

    // clear list of rhymes for this sentence
    rhymeList[sentno].clear();
    wordList[sentno]="";
  }



  /*
  // compute the number of word repetitions for a given rhyme pattern
  uint computeRhymeRepetition(const std::string &rhyme){
    boost::unordered_map<std::string, uint> freq;
    if (rhymeMap[rhyme].count() > 0){
      uint pos = rhymeMap[rhyme].find_first();
      do {
	auto it = freq.find(wordList[pos]);
	if (it == freq.end()){
	  freq[wordList[pos]] = 1;
	}
	else{
	  freq[wordList[pos]]++;
	}
      } while (pos = rhymeMap[rhyme].find_next(pos) != boost::dynamic_bitset<>::npos);

      uint repetitions = 0;
      for (auto it = freq.begin(); it != freq.end(); ++it ){
	if (it->second > 1){
	  cerr << " ..... freq > 1! " << it->first << ":" << it->second << " rhyme=" << rhyme << endl;
	  repetitions += it->second - 1;
	}
      }
      return repetitions;
    }
  }
  */


  void changeWord(const uint &sentno, 
		  const std::string &oldWord, const std::vector<std::string> &oldRhyme,
		  const std::string &newWord, const std::vector<std::string> &newRhyme){

    bool needUpdate=false;
    for (auto rhyme = oldRhyme.begin() ; rhyme != oldRhyme.end(); ++rhyme){
      // if(std::find(newRhyme.begin(), newRhyme.end(), *rhyme)!=newRhyme.end()){
      resetWord(sentno,oldWord,*rhyme);
      needUpdate=true;
      //  }
    }
    for (auto rhyme = newRhyme.begin() ; rhyme != newRhyme.end(); ++rhyme){
      // if(std::find(oldRhyme.begin(), oldRhyme.end(), *rhyme)!=oldRhyme.end()){
      setWord(sentno,newWord,*rhyme);
      needUpdate=true;
      // }
    }
    if (needUpdate){
      updateScore();
    }
  }


  virtual FinalWordRhymeModelState *clone() const {
    return new FinalWordRhymeModelState(*this);
  }
};



void FinalWordRhymeModel::initializeRhymeModel(const Parameters &params){
  maxRhymeDistance = params.get<int>("max-rhyme-distance", 4);
  std::string file = params.get<std::string>("rhymes-file", "");

  if (file != ""){
    ifstream infile(file.c_str());
    std::string word, rhyme;
    while ( infile >> word >> rhyme ){
      rhymes[word].push_back(rhyme);
      // LOG(logger_, debug, "add " << word << " .... " << rhyme);
    }
    infile.close();
    for(Rhymes_::const_iterator it = rhymes.begin(); it != rhymes.end(); ++it) {
      rhymes[it->first].push_back(getLastSyllable(it->first));
      // LOG(logger_, debug, it->first << " .... " << getLastSyllable(it->first));
    }
  }

  // lastSyllRE: simple regex-heuristics to extract final rhyme syllables
  // TODO: move regexes to config files
  // --> make it more flexible for other languages
  lastSyllRE.push_back(boost::regex (".*[^aeiou]([aeiou].*?e[nr])"));
  lastSyllRE.push_back(boost::regex (".*[^aeiou]([aeiou]..*?)"));
  lastSyllRE.push_back(boost::regex ("^([aeiou].*)"));
}

void FinalWordRhymeModel::printPronounciationTable() const {
  typedef boost::unordered_map<std::string,vector<std::string> > Rhymes_;
  cout << "-----------------------------------" << endl;
  for (Rhymes_::const_iterator it = rhymes.begin(); it != rhymes.end(); ++it ) {
    cout << " *** " << it->first << " = " << it->second << endl;
  }
  cout << "-----------------------------------" << endl;
}


FeatureFunction::State *FinalWordRhymeModel::initDocument(const DocumentState &doc, Scores::iterator sbegin) const {
	const std::vector<PhraseSegmentation> &sentences = doc.getPhraseSegmentations();

	FinalWordRhymeModelState *s = new FinalWordRhymeModelState(sentences.size());

	for(uint i = 0; i < sentences.size(); i++){
	  const std::string &word = sentences[i].back().second.get().getTargetPhrase().get().back();
	  const std::vector<std::string> rhyme = getRhyme(word);
	  LOG(logger_, debug, " + " << i << " " << word << " - " << rhyme );
	  if (rhyme.size()>0){
	    for (uint j=0;j<rhyme.size();++j){
	      s->setWord(i,word,rhyme[j]);
	    }
	  }
	}
	
	s->updateScore();

	if (logger_.loggable(debug)){
	  LOG(logger_, debug, "========================================================================");
	  LOG(logger_, debug, "  score: " << " -> " << s->currentScore);
	  LOG(logger_, debug, "  avg rhyme distance score: " << s->averageRhymeDistance()/s->docSize);
	  LOG(logger_, debug, "  coverage: " << s->coverage);
	  LOG(logger_, debug, "  repetitionPenalty: " << s->overallRhymeRepetition << '/' << s->overallRhymes);
	  LOG(logger_, debug, "========================================================================");
	  s->printRhymeMap();
	  s->printRepetitionTable();
	}

	*sbegin = s->score();
	return s;
}


void FinalWordRhymeModel::computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const {
	*sbegin = Float(0);
}

const std::vector<std::string> FinalWordRhymeModel::getRhyme(const std::string &word) const{
  boost::unordered_map<std::string,vector<std::string> >::const_iterator it;

  it = rhymes.find(word);
  if (it != rhymes.end()){
    return it->second;
  }

  std::vector<std::string> syll(1);
  syll[0] = getLastSyllable(word);
  return syll;
}

const std::string FinalWordRhymeModel::getLastSyllable(const std::string &word) const{

  // matches[0] contains the original string.
  // matches[n] contains a sub_match object for each matching subexpression
  // sub_match::first and sub_match::second are iterators that
  // refer to the first and the last chars of the matching subexpression

  if (lastSyllRE.size()>0){
    boost::cmatch matches; 
    for (uint i = 0; i<lastSyllRE.size();++i){
      if (boost::regex_match(word.c_str(), matches, lastSyllRE[i])){
	// cerr << " found matches: " << matches.size() << " - " << matches[1].first << " + " 
	//      << matches[1].second << endl;
	return matches[1].first;
      }
    }
  }

  else{

    // fallback for English if no lastSyllRE is given
    boost::cmatch matches; 
    boost::regex re0(".*[^aeiou]([aeiou].*?e[nr])");
    if (boost::regex_match(word.c_str(), matches, re0)){
      return matches[1].first;
    }
    boost::regex re1(".*[^aeiou]([aeiou]..*?)");
    if (boost::regex_match(word.c_str(), matches, re1)){
      return matches[1].first;
    }
    boost::regex re2("^([aeiou].*)");
    if (boost::regex_match(word.c_str(), matches, re2)){
      return matches[1].first;
    }
  }
  return word;
}


FeatureFunction::StateModifications *FinalWordRhymeModel::estimateScoreUpdate(const DocumentState &doc, 
									      const SearchStep &step, 
									      const State *state,
									      Scores::const_iterator psbegin, 
									      Scores::iterator sbegin) const {

  const FinalWordRhymeModelState *prevstate = dynamic_cast<const FinalWordRhymeModelState *>(state);
  FinalWordRhymeModelState *s = prevstate->clone();
  
  const std::vector<SearchStep::Modification> &mods = step.getModifications();

  for(std::vector<SearchStep::Modification>::const_iterator it = mods.begin(); it != mods.end(); ++it) {

    uint sentno = it->sentno;
    const PhraseSegmentation &sentence = doc.getPhraseSegmentation(sentno);

    if (it->to_it == sentence.end()) {
     
	const std::string &oldWord = sentence.back().second.get().getTargetPhrase().get().back();
	const std::vector<std::string> oldRhyme = getRhyme(oldWord);

	const std::string &newWord = it->proposal.back().second.get().getTargetPhrase().get().back();
	const std::vector<std::string> newRhyme = getRhyme(newWord);

	/*
	if (oldWord != oldWord2){
	  LOG(logger_, debug, "** replace " << oldWord << " -> " << newWord << " in " << sentno);
	  LOG(logger_, debug, "** but " << oldWord2 << " -> " << newWord << " in " << sentno);
	}
	*/

	Float oldScore = s->currentScore;
	s->changeWord(sentno,oldWord,oldRhyme,newWord,newRhyme);

	if (logger_.loggable(debug)){
	  if (s->currentScore > oldScore){
	    LOG(logger_, debug, "+ improved score: " << oldScore << " -> " << s->currentScore);
	    LOG(logger_, debug, "  avg rhyme distance score: " << s->averageRhymeDistance()/s->docSize);
	    LOG(logger_, debug, "  coverage: " << s->coverage);
	    LOG(logger_, debug, "  repetitionPenalty: " << s->overallRhymeRepetition << '/' << s->overallRhymes);
	    s->printRhymeMap();
	    // s->printRepetitionTable();
	  }
	  /*
	    else{
	    if (s->currentScore < oldScore){
	    LOG(logger_, debug, "- decreased score: " << oldScore << " -> " << s->currentScore);
	    LOG(logger_, debug, "  avg rhyme distance score: " << s->averageRhymeDistance()/s->docSize);
	    LOG(logger_, debug, "  coverage: " << s->coverage);
	    LOG(logger_, debug, "  repetitionPenalty: " << s->overallRhymeRepetition << '/' << s->overallRhymes);
	    s->printRhymeMap();
	    s->printRepetitionTable();
	    }
	    }
	  */
	}
    }
  }
  *sbegin = s->score();
  return s;
}



FeatureFunction::StateModifications *FinalWordRhymeModel::updateScore(const DocumentState &doc, 
								      const SearchStep &step, 
								      const State *state,
								      FeatureFunction::StateModifications *estmods, 
								      Scores::const_iterator psbegin, 
								      Scores::iterator estbegin) const {
  return estmods;
}


FeatureFunction::State *FinalWordRhymeModel::applyStateModifications(FeatureFunction::State *oldState, FeatureFunction::StateModifications *modif) const {
	FinalWordRhymeModelState *os = dynamic_cast<FinalWordRhymeModelState *>(oldState);
	FinalWordRhymeModelState *ms = dynamic_cast<FinalWordRhymeModelState *>(modif);
	os->rhymeMap.swap(ms->rhymeMap);
	os->wordList.swap(ms->wordList);
	os->rhymeList.swap(ms->rhymeList);
	os->rhymeRepetition.swap(ms->rhymeRepetition);
	os->currentScore = ms->currentScore;
	os->coverage = ms->coverage;
	os->overallRhymeRepetition = ms->overallRhymeRepetition;
	os->overallRhymes = ms->overallRhymes;
	return os;
}

