/*
 *  SentenceFinalWordModel.cpp
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
using namespace std;

#include "Docent.h"
#include "DocumentState.h"
#include "FeatureFunction.h"
#include "SearchStep.h"
#include "SentenceFinalWordModel.h"

#include <boost/unordered_map.hpp>


struct SentenceFinalWordModelState : public FeatureFunction::State, public FeatureFunction::StateModifications {
  SentenceFinalWordModelState(uint nsents) : wordFreq(nsents), mostFreq(0), mostFreqWord(".") { docSize = nsents; mostFreq = 0; mostFreqWord = "."; }
  
  typedef boost::unordered_map<std::string,uint> WordFreq_;
  WordFreq_ wordFreq;
  uint docSize;
  uint mostFreq;
  std::string mostFreqWord;

  Float score() const {
    return -Float(docSize - mostFreq);
  }

  Float entropy() const {
    float entropy;
    boost::unordered_map<std::string,uint>::const_iterator it;
    for (WordFreq_::const_iterator it = wordFreq.begin(); it != wordFreq.end(); ++it ) {
      float prob = float(it->second) / float(docSize);
      entropy += prob * log(prob) / log(2);
    }
    return entropy;
  }


  void changeWord(std::string oldWord, std::string newWord){

    if (newWord == oldWord){
      return;
    }
    if ( oldWord.length() > 1 ){
      if (wordFreq.find(oldWord) == wordFreq.end()){
	cerr << " .... hash key does not exist for ... " << oldWord << endl;
      }
      if (wordFreq[oldWord] == 0){
	cerr << " .... strange: zero frequency for " << oldWord << endl;
      }
      else{
	wordFreq[oldWord]--;
	if (oldWord == mostFreqWord){
	  findMostFreq();
	}
      }
    }

    if ( newWord.length() > 1 ){
      wordFreq[newWord]++;
      if (wordFreq[newWord] > mostFreq){
	mostFreq = wordFreq[newWord];
	mostFreqWord = newWord;
	// cerr << " .... new highest freq for " << newWord << " = " << mostFreq << endl;
      }
    }

  }

  uint findMostFreq () {
    mostFreq = 0;
    boost::unordered_map<std::string,uint>::const_iterator it;
    for (WordFreq_::const_iterator it = wordFreq.begin(); it != wordFreq.end(); ++it ) {
      if ( it->second > mostFreq ){
	mostFreqWord = it->first;
	mostFreq = wordFreq[mostFreqWord];
      }
    }
    return mostFreq;
  }


  void printWordTable(){
    boost::unordered_map<std::string,uint>::const_iterator it;
    cerr << mostFreq << endl;
    cerr << "-----------------------------------" << endl;
    for (WordFreq_::const_iterator it = wordFreq.begin(); it != wordFreq.end(); ++it ) {
      cerr << " *** " << it->first << " = " << it->second << endl;
    }
    cerr << "-----------------------------------" << endl;
  }


  virtual SentenceFinalWordModelState *clone() const {
    return new SentenceFinalWordModelState(*this);
  }
};





FeatureFunction::State *SentenceFinalWordModel::initDocument(const DocumentState &doc, Scores::iterator sbegin) const {
	const std::vector<PhraseSegmentation> &sentences = doc.getPhraseSegmentations();

	SentenceFinalWordModelState *s = new SentenceFinalWordModelState(sentences.size());

	for(uint i = 0; i < sentences.size(); i++){
	  std::string lastWord = *sentences[i].rbegin()->second.get().getTargetPhrase().get().rbegin();
	  if ( lastWord.length() > 1 ){
	    s->wordFreq[lastWord]++;
	    if (s->wordFreq[lastWord] > s->mostFreq){
	      s->mostFreq = s->wordFreq[lastWord];
	      s->mostFreqWord = lastWord;
	    }
	  }
	}

	*sbegin = s->score();
	return s;
}


void SentenceFinalWordModel::computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const {
	*sbegin = Float(0);
}

FeatureFunction::StateModifications *SentenceFinalWordModel::estimateScoreUpdate(const DocumentState &doc, const SearchStep &step, const State *state,
		Scores::const_iterator psbegin, Scores::iterator sbegin) const {

	const SentenceFinalWordModelState *prevstate = dynamic_cast<const SentenceFinalWordModelState *>(state);
	SentenceFinalWordModelState *s = prevstate->clone();

	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	for(std::vector<SearchStep::Modification>::const_iterator it = mods.begin(); it != mods.end(); ++it) {
	  uint sentno = it->sentno;
	  const PhraseSegmentation &sentence = doc.getPhraseSegmentation(sentno);

	  if (it->to_it == sentence.end()) {

	    std::string oldWord = *sentence.rbegin()->second.get().getTargetPhrase().get().rbegin();
	    std::string newWord = *it->proposal.rbegin()->second.get().getTargetPhrase().get().rbegin();

	    if (newWord != oldWord){
	      // s->printWordTable();
	      s->changeWord(oldWord,newWord);
	    }
	  }
	}

	*sbegin = s->score();
	return s;
}


FeatureFunction::StateModifications *SentenceFinalWordModel::updateScore(const DocumentState &doc, const SearchStep &step, const State *state,
		FeatureFunction::StateModifications *estmods, Scores::const_iterator psbegin, Scores::iterator estbegin) const {
  return estmods;
}


FeatureFunction::State *SentenceFinalWordModel::applyStateModifications(FeatureFunction::State *oldState, FeatureFunction::StateModifications *modif) const {
	SentenceFinalWordModelState *os = dynamic_cast<SentenceFinalWordModelState *>(oldState);
	SentenceFinalWordModelState *ms = dynamic_cast<SentenceFinalWordModelState *>(modif);
	os->wordFreq.swap(ms->wordFreq);
	os->mostFreq = ms->mostFreq;
	return os;
}
