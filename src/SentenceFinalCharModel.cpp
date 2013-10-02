/*
 *  SentenceFinalCharModel.cpp
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
#include "SentenceFinalCharModel.h"

#include <boost/unordered_map.hpp>


struct SentenceFinalCharModelState : public FeatureFunction::State, public FeatureFunction::StateModifications {
  SentenceFinalCharModelState(uint nsents) : charFreq(nsents), mostFreq(0), mostFreqChar('.') { docSize = nsents; mostFreq = 0; mostFreqChar = '.'; }
  
  // std::vector<char> initialChar;
  typedef boost::unordered_map<char,uint> CharFreq_;
  CharFreq_ charFreq;
  uint docSize;
  uint mostFreq;
  char mostFreqChar;

  Float score() const {
    return -Float(docSize - mostFreq);
  }

  void changeChar(char oldChar, char newChar){

    if (newChar == oldChar){
      return;
    }
    if (charFreq.find(oldChar) == charFreq.end()){
	cerr << " .... hash key does not exist for ... " << oldChar << endl;
    }
    if (charFreq[oldChar] == 0){
      cerr << " .... strange: zero frequency for " << oldChar << endl;
    }
    else{
      charFreq[oldChar]--;
      if (oldChar == mostFreqChar){
	findMostFreq();
      }
    }
    charFreq[newChar]++;
    if (charFreq[newChar] > mostFreq){
      mostFreq = charFreq[newChar];
      mostFreqChar = newChar;
      // cerr << " .... new highest freq for " << newChar << " = " << charFreq[newChar] << endl;
    }
  }

  uint findMostFreq () {
    mostFreq = 0;
    boost::unordered_map<char,uint>::const_iterator it;
    for (CharFreq_::const_iterator it = charFreq.begin(); it != charFreq.end(); ++it ) {
      if ( it->second > mostFreq ){
	mostFreqChar = it->first;
	mostFreq = charFreq[mostFreqChar];
      }
    }
    return mostFreq;
  }

  void printCharTable(){
    boost::unordered_map<char,uint>::const_iterator it;
    cerr << "-----------------------------------" << endl;
    for (CharFreq_::const_iterator it = charFreq.begin(); it != charFreq.end(); ++it ) {
      cerr << " *** " << it->first << " = " << it->second << endl;
    }
    cerr << "-----------------------------------" << endl;
  }


  virtual SentenceFinalCharModelState *clone() const {
    return new SentenceFinalCharModelState(*this);
  }
};





FeatureFunction::State *SentenceFinalCharModel::initDocument(const DocumentState &doc, Scores::iterator sbegin) const {
	const std::vector<PhraseSegmentation> &sentences = doc.getPhraseSegmentations();

	SentenceFinalCharModelState *s = new SentenceFinalCharModelState(sentences.size());

	for(uint i = 0; i < sentences.size(); i++){
	  std::string lastWord = *sentences[i].rbegin()->second.get().getTargetPhrase().get().rbegin();
	  char lastChar = *lastWord.rbegin();
	  s->charFreq[lastChar]++;
	  if (s->charFreq[lastChar] > s->mostFreq){
	    s->mostFreq = s->charFreq[lastChar];
	    s->mostFreqChar = lastChar;
	  }
	}

	*sbegin = s->score();
	return s;
}


void SentenceFinalCharModel::computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const {
	*sbegin = Float(0);
}

FeatureFunction::StateModifications *SentenceFinalCharModel::estimateScoreUpdate(const DocumentState &doc, const SearchStep &step, const State *state,
		Scores::const_iterator psbegin, Scores::iterator sbegin) const {

	const SentenceFinalCharModelState *prevstate = dynamic_cast<const SentenceFinalCharModelState *>(state);
	SentenceFinalCharModelState *s = prevstate->clone();

	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	for(std::vector<SearchStep::Modification>::const_iterator it = mods.begin(); it != mods.end(); ++it) {
	  uint sentno = it->sentno;
	  const PhraseSegmentation &sentence = doc.getPhraseSegmentation(sentno);

	  //// !!! it->to is not reliable! --> use iterator to_it instead!
	  //// also: countTargetWords(sentence)) is wrong! count segments instead!
	  //
	  // cerr << it->to << "  "<< sentence.size() << endl;
	  // if (it->to == sentence.size()){

	  if(it->to_it == sentence.end()) {

	    std::string oldWord = *sentence.rbegin()->second.get().getTargetPhrase().get().rbegin();
	    std::string newWord = *it->proposal.rbegin()->second.get().getTargetPhrase().get().rbegin();

	    char oldChar = *oldWord.rbegin();
	    char newChar = *newWord.rbegin();

	    if (newChar != oldChar){
	      s->changeChar(oldChar,newChar);
	    }
	  }
	}

	*sbegin = s->score();
	return s;
}


FeatureFunction::StateModifications *SentenceFinalCharModel::updateScore(const DocumentState &doc, const SearchStep &step, const State *state,
		FeatureFunction::StateModifications *estmods, Scores::const_iterator psbegin, Scores::iterator estbegin) const {
  return estmods;
}


FeatureFunction::State *SentenceFinalCharModel::applyStateModifications(FeatureFunction::State *oldState, FeatureFunction::StateModifications *modif) const {
	SentenceFinalCharModelState *os = dynamic_cast<SentenceFinalCharModelState *>(oldState);
	SentenceFinalCharModelState *ms = dynamic_cast<SentenceFinalCharModelState *>(modif);
	os->charFreq.swap(ms->charFreq);
	os->mostFreq = ms->mostFreq;
	return os;
}
