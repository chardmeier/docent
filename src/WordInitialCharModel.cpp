/*
 *  WordInitialCharModel.cpp
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
#include "WordInitialCharModel.h"
#include "PhrasePair.h"


#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/unordered_map.hpp>
#include <boost/regex.hpp>

struct WordInitialCharModelState : public FeatureFunction::State, public FeatureFunction::StateModifications {
  WordInitialCharModelState(uint nwords) : charFreq(nwords), logger_("WordInitialCharModel") { docSize = nwords; }

  mutable Logger logger_;

  typedef boost::unordered_map<char,uint> CharFreq_;
  CharFreq_ charFreq;
  uint docSize;

  Float score() const {

    uint mostFreq = 0;
    char mostFreqChar;

    boost::unordered_map<char,uint>::const_iterator it;
    for (CharFreq_::const_iterator it = charFreq.begin(); it != charFreq.end(); ++it ) {
      if (it->second > mostFreq){
	mostFreqChar = it->first;
	mostFreq = it->second;
      }
    }
    
    LOG(logger_, debug, "most freq = " << mostFreqChar << " freq = " << mostFreq );

    return -Float(docSize - mostFreq);
  }

  virtual WordInitialCharModelState *clone() const {
    return new WordInitialCharModelState(*this);
  }
};

FeatureFunction::State *WordInitialCharModel::initDocument(const DocumentState &doc, Scores::iterator sbegin) const {
	const std::vector<PhraseSegmentation> &sentences = doc.getPhraseSegmentations();

	uint count = 0;
	for(uint i = 0; i < sentences.size(); i++){
	  count += countSourceWords(sentences[i].begin(),sentences[i].end());
	}

	WordInitialCharModelState *s = new WordInitialCharModelState(count);

	boost::regex initialRE("^[a-zA-Z].*");

	for(uint i = 0; i < sentences.size(); i++){
	  for (PhraseSegmentation::const_iterator it = sentences[i].begin(); it != sentences[i].end(); ++it){
	    Phrase phrase = it->second.get().getTargetPhrase();
	    const std::vector<Word> words = phrase.get();
	    for (uint wi = 0; wi < words.size(); wi++){
	      std::string word = words[wi];
	      if (boost::regex_match (word,initialRE))
		s->charFreq[word[0]]++;
	    }
	  }
	}

	*sbegin = s->score();
	return s;
}



void WordInitialCharModel::computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const {
	*sbegin = Float(0);
}

FeatureFunction::StateModifications *WordInitialCharModel::estimateScoreUpdate(const DocumentState &doc, const SearchStep &step, const State *state,
		Scores::const_iterator psbegin, Scores::iterator sbegin) const {
	const WordInitialCharModelState *prevstate = dynamic_cast<const WordInitialCharModelState *>(state);
	WordInitialCharModelState *s = prevstate->clone();
	boost::regex initialRE("^[a-zA-Z].*");


	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	for(std::vector<SearchStep::Modification>::const_iterator it = mods.begin(); it != mods.end(); ++it) {

	  // subtract initial char counts for old words

	  for (PhraseSegmentation::const_iterator pi = it->from_it; pi != it->to_it; ++pi){
	    Phrase phrase = pi->second.get().getTargetPhrase();
	    const std::vector<Word> words = phrase.get();
	    for (uint wi = 0; wi < words.size(); wi++){
	      std::string word = words[wi];
	      if (boost::regex_match (word,initialRE))
		s->charFreq[word[0]]--;
	    }
	  }

	  // add counts for proposed words

	  for (PhraseSegmentation::const_iterator pi = it->proposal.begin(); pi != it->proposal.end(); ++pi){
	    Phrase phrase = pi->second.get().getTargetPhrase();
	    const std::vector<Word> words = phrase.get();
	    for (uint wi = 0; wi < words.size(); wi++){
	      std::string word = words[wi];
	      if (boost::regex_match (word,initialRE))
		s->charFreq[word[0]]++;
	    }
	  }
	}

	*sbegin = s->score();
	return s;
}

FeatureFunction::StateModifications *WordInitialCharModel::updateScore(const DocumentState &doc, const SearchStep &step, const State *state,
		FeatureFunction::StateModifications *estmods, Scores::const_iterator psbegin, Scores::iterator estbegin) const {
	return estmods;
}

FeatureFunction::State *WordInitialCharModel::applyStateModifications(FeatureFunction::State *oldState, FeatureFunction::StateModifications *modif) const {
	WordInitialCharModelState *os = dynamic_cast<WordInitialCharModelState *>(oldState);
	WordInitialCharModelState *ms = dynamic_cast<WordInitialCharModelState *>(modif);
	os->charFreq.swap(ms->charFreq);
	return oldState;
}
