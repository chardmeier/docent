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
  FinalWordRhymeModelState(uint nsents) : rhymeMap(nsents), rhymeEntropy(nsents), currentScore(0) { docSize = nsents; }

  uint docSize;

  typedef boost::unordered_map<std::string,boost::dynamic_bitset<> > WordMap_;
  typedef boost::unordered_map<std::string,float> WordEntropy_;
  typedef boost::unordered_map<uint,uint> GapFreq_;

  WordMap_ rhymeMap;
  WordEntropy_ rhymeEntropy;
  float currentScore;

  Float score() const {
    return currentScore-100;
  }


  void updateScore() {


    boost::dynamic_bitset<> covered(docSize);

    float oldScore = currentScore;
    currentScore = 0;
    covered.reset();

    for (WordEntropy_::const_iterator it = rhymeEntropy.begin(); it != rhymeEntropy.end(); ++it ) {
      currentScore+=it->second;
      WordMap_::const_iterator r_it;
      r_it = rhymeMap.find(it->first);
      if (r_it != rhymeMap.end()){
	// XOR of bitsets
	boost::dynamic_bitset<> combined((covered^=r_it->second));
	covered = combined;
	/*
	cerr << " ---- "  << combined << " = " << ratio << " = " << currentScore << endl;
	*/
      }
    }

    /*
    Float ratio = (float) covered.count() / docSize;
    currentScore *= (1-ratio);
    */
    

    if (currentScore < oldScore){
      cerr << "-----------------------------" << endl;
      cerr << oldScore << " : " << currentScore << endl;
      cerr << "-----------------------------" << endl;
      printRhymeMap();
      printRhymeEntropies();
    }

  }


  void computeAllEntropies() {
    computeAllEntropies(rhymeMap,rhymeEntropy);
  }

  void computeAllEntropies(const WordMap_ &map, WordEntropy_ &entropy) {
    for (WordMap_::const_iterator it = map.begin(); it != map.end(); ++it ) {
      computeEntropy(map,it->first,entropy);
    }
  }


  void printRhymeMap() const {
    for (WordMap_::const_iterator it = rhymeMap.begin(); it != rhymeMap.end(); ++it ) {
      if (it->second.count() > 1){
	cerr << it->second << it->first << endl;
      }
    }
  }

  void printRhymeEntropies() const {
    for (WordEntropy_::const_iterator it = rhymeEntropy.begin(); it != rhymeEntropy.end(); ++it ) {
      if (it->second > 0){
	cerr << "entropy: " << it->first << " = " << it->second << endl;
      }
    }
  }


  void computeEntropy(const WordMap_ &map, const std::string word, WordEntropy_ &entropy){
    WordMap_::const_iterator it;
    it = map.find(word);
    if (it == map.end()){
      return;
    }

    GapFreq_ gapFreq;

    /// TODO: could use bitset.find_next instead of for-loop (more efficient?)

    uint lastPos = 0;
    // boost::dynamic_bitset<> bitmap(docSize);
    // bitmap = (it->second);
    const boost::dynamic_bitset<> &bitmap(it->second);
    // for(uint i = 0; i < bitmap.size(); ++i){
    for(uint i = bitmap.find_first(); i < bitmap.size(); ++i){
      if (bitmap.test(i)){
	if (lastPos > 0){
	  uint gap = i+1-lastPos;
	  gapFreq[gap]++;
	}
	lastPos=i+1;
      }
    }

    // uint nrGaps = gapFreq.size();
    uint nrGaps = bitmap.count()-1;
    entropy[word] = 0;
    for (GapFreq_::const_iterator g_it = gapFreq.begin(); g_it != gapFreq.end(); ++g_it ) {
      float prob = float(g_it->second) / float(nrGaps);
      entropy[word] -= prob * log(prob) / log(2);
    }
  }


  void setWord(const uint sentno, const std::string word, const std::string rhyme){
    if (rhyme == word){ return; }
    WordMap_::const_iterator it = rhymeMap.find(rhyme);
    if (it == rhymeMap.end()){
      rhymeMap[rhyme].resize(docSize);
    }
    rhymeMap[rhyme].set(sentno);
    // cerr << " rhymemap " << rhymeMap[rhyme] << " / " << rhyme << endl;
  }



  void resetWord(const uint sentno, const std::string word, const std::string rhyme){
    if (rhyme == word){ return; }
    rhymeMap[rhyme].reset(sentno);
  }



  void changeWord(const uint sentno, 
		  const std::string oldWord, const std::string oldRhyme,
		  const std::string newWord, const std::string newRhyme){
    if (oldRhyme != newRhyme){
      resetWord(sentno,oldWord,oldRhyme);
      setWord(sentno,newWord,newRhyme);
      computeEntropy(rhymeMap,oldRhyme,rhymeEntropy);
      computeEntropy(rhymeMap,newRhyme,rhymeEntropy);
      updateScore();
    }
  }


  virtual FinalWordRhymeModelState *clone() const {
    return new FinalWordRhymeModelState(*this);
  }
};





void FinalWordRhymeModel::loadPronounciationModel(const Parameters &params){
  std::string file = params.get<std::string>("rhymes-file", "");
  ifstream infile(file.c_str());
  std::string word, rhyme;
  while ( infile >> word >> rhyme ){
    rhymes[word] = rhyme;
  }
  infile.close();
}

void FinalWordRhymeModel::printPronounciationTable() const {
  typedef boost::unordered_map<std::string,std::string> Rhymes_;
  boost::unordered_map<std::string,std::string>::const_iterator it;
    cerr << "-----------------------------------" << endl;
    for (Rhymes_::const_iterator it = rhymes.begin(); it != rhymes.end(); ++it ) {
      cerr << " *** " << it->first << " = " << it->second << endl;
    }
    cerr << "-----------------------------------" << endl;
}


FeatureFunction::State *FinalWordRhymeModel::initDocument(const DocumentState &doc, Scores::iterator sbegin) const {
	const std::vector<PhraseSegmentation> &sentences = doc.getPhraseSegmentations();

	FinalWordRhymeModelState *s = new FinalWordRhymeModelState(sentences.size());

	for(uint i = 0; i < sentences.size(); i++){
	  const std::string &word = *sentences[i].rbegin()->second.get().getTargetPhrase().get().rbegin();
	  const std::string rhyme = getPronounciation(word);

	  s->setWord(i,word,rhyme);
	}
	s->computeAllEntropies();
	s->updateScore();

	*sbegin = s->score();
	return s;
}


void FinalWordRhymeModel::computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const {
	*sbegin = Float(0);
}

const std::string FinalWordRhymeModel::getPronounciation(const std::string &word) const{
  boost::unordered_map<std::string,std::string>::const_iterator it;
  it = rhymes.find(word);
  if (it != rhymes.end()){
    return it->second;
  }
  boost::regex re(".*[^aeiou]([aeiou].*)");
  boost::cmatch matches;

  if (boost::regex_match(word.c_str(), matches, re)){

    // matches[0] contains the original string.  matches[n]
    // contains a sub_match object for each matching
    // subexpression

    // sub_match::first and sub_match::second are iterators that
    // refer to the first and one past the last chars of the
    // matching subexpression

    // cerr << " found matches: " << matches.size() << " - " << matches[1].first << " + " 
    //      << matches[1].second << endl;
    return matches[1].first;
  }

  boost::regex re2("^([aeiou].*)");
  if (boost::regex_match(word.c_str(), matches, re2)){
    return matches[1].first;
  }

  return word;
}

FeatureFunction::StateModifications *FinalWordRhymeModel::estimateScoreUpdate(const DocumentState &doc, const SearchStep &step, const State *state,
		Scores::const_iterator psbegin, Scores::iterator sbegin) const {

	const FinalWordRhymeModelState *prevstate = dynamic_cast<const FinalWordRhymeModelState *>(state);
	FinalWordRhymeModelState *s = prevstate->clone();
  
	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	for(std::vector<SearchStep::Modification>::const_iterator it = mods.begin(); it != mods.end(); ++it) {
	  uint sentno = it->sentno;
	  const PhraseSegmentation &sentence = doc.getPhraseSegmentation(sentno);

	  if (it->to_it == sentence.end()) {

	    const std::string &oldWord = *sentence.rbegin()->second.get().getTargetPhrase().get().rbegin();
	    const std::string oldRhyme = getPronounciation(oldWord);

	    const std::string &newWord = *it->proposal.rbegin()->second.get().getTargetPhrase().get().rbegin();
	    const std::string newRhyme = getPronounciation(newWord);

	    s->changeWord(sentno,oldWord,oldRhyme,newWord,newRhyme);

	  }
	}
	*sbegin = s->score();
	return s;
}


FeatureFunction::StateModifications *FinalWordRhymeModel::updateScore(const DocumentState &doc, const SearchStep &step, const State *state,
		FeatureFunction::StateModifications *estmods, Scores::const_iterator psbegin, Scores::iterator estbegin) const {
  return estmods;
}


FeatureFunction::State *FinalWordRhymeModel::applyStateModifications(FeatureFunction::State *oldState, FeatureFunction::StateModifications *modif) const {
	FinalWordRhymeModelState *os = dynamic_cast<FinalWordRhymeModelState *>(oldState);
	FinalWordRhymeModelState *ms = dynamic_cast<FinalWordRhymeModelState *>(modif);
	os->rhymeMap.swap(ms->rhymeMap);
	os->rhymeEntropy.swap(ms->rhymeEntropy);
	os->currentScore = ms->currentScore;
	return os;
}

