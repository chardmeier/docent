/*
 *  SentenceInitialCharModel.cpp
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
#include "SentenceInitialCharModel.h"

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/unordered_map.hpp>


struct SentenceInitialCharModelState : public FeatureFunction::State, public FeatureFunction::StateModifications {
  SentenceInitialCharModelState(uint nsents) : charFreq(nsents) { docSize = nsents; }
  
  // std::vector<char> initialChar;
  typedef boost::unordered_map<char,uint> CharFreq_;
  CharFreq_ charFreq;
  uint docSize;

  Float score() const {

    uint mostFreq = 0;
    char mostFreqChar;
    // float entropy;

    boost::unordered_map<char,uint>::const_iterator it;
    for (CharFreq_::const_iterator it = charFreq.begin(); it != charFreq.end(); ++it ) {
      // float prob = float(it->second) / float(docSize);
      // entropy += prob * log(prob) / log(2);
      if (it->second > mostFreq){
	mostFreqChar = it->first;
	mostFreq = it->second;
      }
    }
    // cerr << "most freq = " << mostFreqChar << " freq = " << mostFreq << " score = " << docSize - mostFreq << endl;

    // return entropy;
    return -Float(docSize - mostFreq);
  }

  virtual SentenceInitialCharModelState *clone() const {
    return new SentenceInitialCharModelState(*this);
  }
};

FeatureFunction::State *SentenceInitialCharModel::initDocument(const DocumentState &doc, Scores::iterator sbegin) const {
	const std::vector<PhraseSegmentation> &sentences = doc.getPhraseSegmentations();

	SentenceInitialCharModelState *s = new SentenceInitialCharModelState(sentences.size());

	for(uint i = 0; i < sentences.size(); i++){

	  // * get the target phrase of the first phrase pair
	  Phrase phrase = sentences[i].begin()->second.get().getTargetPhrase();
	  // * get all words in that phrase:
	  // const std::vector<Word> = phrase.get();
	  // * get only the first word as a string
	  std::string word = phrase.get()[0];
	  s->charFreq[word[0]]++;
	  // s->initialChar[i] = word[0];
	}

	*sbegin = s->score();
	return s;
}



void SentenceInitialCharModel::computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const {
	*sbegin = Float(0);
}

FeatureFunction::StateModifications *SentenceInitialCharModel::estimateScoreUpdate(const DocumentState &doc, const SearchStep &step, const State *state,
		Scores::const_iterator psbegin, Scores::iterator sbegin) const {
	const SentenceInitialCharModelState *prevstate = dynamic_cast<const SentenceInitialCharModelState *>(state);
	SentenceInitialCharModelState *s = prevstate->clone();

	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	for(std::vector<SearchStep::Modification>::const_iterator it = mods.begin(); it != mods.end(); ++it) {
	  uint sentno = it->sentno;
	  PhraseSegmentation sentence = doc.getPhraseSegmentation(sentno);
	  if (it->from == 0){
	    std::string oldWord = sentence.begin()->second.get().getTargetPhrase().get()[0];
	    std::string newWord = it->proposal.begin()->second.get().getTargetPhrase().get()[0];

	    // cerr << " before: " <<  oldWord[0] << " = " << s->charFreq[oldWord[0]] << " " <<  newWord[0] << " = " << s->charFreq[newWord[0]] << endl;

	    s->charFreq[oldWord[0]]--;
	    s->charFreq[newWord[0]]++;

	    // cerr << "  after: " <<  oldWord[0] << " = " << s->charFreq[oldWord[0]] << " " <<  newWord[0] << " = " << s->charFreq[newWord[0]] << endl;

	  }
	}

	*sbegin = s->score();
	return s;
}

FeatureFunction::StateModifications *SentenceInitialCharModel::updateScore(const DocumentState &doc, const SearchStep &step, const State *state,
		FeatureFunction::StateModifications *estmods, Scores::const_iterator psbegin, Scores::iterator estbegin) const {
	return estmods;
}

FeatureFunction::State *SentenceInitialCharModel::applyStateModifications(FeatureFunction::State *oldState, FeatureFunction::StateModifications *modif) const {
	SentenceInitialCharModelState *os = dynamic_cast<SentenceInitialCharModelState *>(oldState);
	SentenceInitialCharModelState *ms = dynamic_cast<SentenceInitialCharModelState *>(modif);
	os->charFreq.swap(ms->charFreq);
	return oldState;
}
