/*
 *  SelectedWordSlowLM.cpp
 *
 *  Copyright 2012 by Joerg Tiedemann. All rights reserved.
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

#include "Docent.h"
#include "DocumentState.h"
#include "FeatureFunction.h"
#include "SearchStep.h"
#include "SelectedWordSlowLM.h"

#include <boost/unordered_map.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/regex.hpp>

#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>

// from kenlm
#include "lm/binary_format.hh"
#include "lm/model.hh"



template<class Model>
class SelectedWordSlowLM : public FeatureFunction {
  friend class SelectedWordSlowLMFactory;

private:

  typedef typename Model::Vocabulary VocabularyType_;
  typedef typename Model::State StateType_;

  typedef std::pair<StateType_,Float> WordState_;
  typedef std::vector<WordState_> SentenceState_;


  mutable Logger logger_;
  Model *model_;
  uint maxWordLength;

  SelectedWordSlowLM(const std::string &file,const Parameters &params);
  Float scoreNgram(const StateType_ &old_state, lm::WordIndex word, WordState_ &out_state) const;

public:

	virtual ~SelectedWordSlowLM();
	virtual State *initDocument(const DocumentState &doc, Scores::iterator sbegin) const;
	virtual StateModifications *estimateScoreUpdate(const DocumentState &doc, const SearchStep &step, const State *state,
		Scores::const_iterator psbegin, Scores::iterator sbegin) const;
	virtual StateModifications *updateScore(const DocumentState &doc, const SearchStep &step, const State *state,
		StateModifications *estmods, Scores::const_iterator, Scores::iterator estbegin) const;
	virtual FeatureFunction::State *applyStateModifications(FeatureFunction::State *oldState, FeatureFunction::StateModifications *modif) const;
	
	virtual uint getNumberOfScores() const {
		return 1;
	}

	virtual void computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const;
};




struct SelectedWordSlowLMState : public FeatureFunction::State, public FeatureFunction::StateModifications {
  SelectedWordSlowLMState(uint nsents)  : logger_("SelectedWordSlowLM"), currentScore(0) {
    sentWords.resize(nsents);
  };

  std::vector< std::vector<std::string> > sentWords;

  mutable Logger logger_;
  Float currentScore;

  Float score() {
    return currentScore;
  }

  uint AddWord(const uint sentno, const AnchoredPhrasePair &app, const uint maxLength){
    WordAlignment wa = app.second.get().getWordAlignment();
    PhraseData sd = app.second.get().getSourcePhrase().get();
    PhraseData td = app.second.get().getTargetPhrase().get();

    uint addCount=0;
    for (uint j=0; j<sd.size(); ++j) {
      // just for testing: words longer than 5 characters .... (should use some other criteria here!)
      if ( (maxLength==0) || (sd[j].size() > maxLength) ){
	for (WordAlignment::const_iterator wit = wa.begin_for_source(j);
	     wit != wa.end_for_source(j); ++wit) {
	  sentWords[sentno].push_back(td[*wit]);
	  addCount++;
	  // LOG(logger_, debug, "add word " << td[*wit] << " aligned to " << sd[j]);
	}
      }
    }
    return addCount;
  }


  virtual SelectedWordSlowLMState *clone() const {
    return new SelectedWordSlowLMState(*this);
  }
};





FeatureFunction *SelectedWordSlowLMFactory::createNgramModel(const Parameters &params) {
	std::string file = params.get<std::string>("lm-file");
	lm::ngram::ModelType mtype;

	std::string smtype = params.get<std::string>("model-type", "");

	Logger logger("SelectedWordSlowLM");

	if(!lm::ngram::RecognizeBinary(file.c_str(), mtype)) {
		if(smtype.empty() || smtype == "hash-probing")
			mtype = lm::ngram::HASH_PROBING;
		else if(smtype == "trie-sorted")
			mtype = lm::ngram::TRIE_SORTED;
		else {
			LOG(logger, error, "Unsupported LM type " << smtype <<
				" for file " << file);
		}
	}

	switch(mtype) {
	case lm::ngram::HASH_PROBING:
		if(!smtype.empty() && smtype != "hash-probing")
			LOG(logger, error, "Incorrect LM type in configuration "
				"for file " << file);

		return new SelectedWordSlowLM<lm::ngram::ProbingModel>(file,params);
	case lm::ngram::TRIE_SORTED:
		if(!smtype.empty() && smtype != "trie-sorted")
			LOG(logger, error, "Incorrect LM type in configuration "
				"for file " << file);
		return new SelectedWordSlowLM<lm::ngram::TrieModel>(file,params);
	default:
		LOG(logger, error, "Unsupported LM type for file " << file);
		BOOST_THROW_EXCEPTION(FileFormatException());
	}
}




template<class Model>
SelectedWordSlowLM<Model>::SelectedWordSlowLM(const std::string &file,const Parameters &params) : logger_("SelectedWordSlowLM") {
  model_ = new Model(file.c_str());
  maxWordLength = params.get<uint>("max-word-length");

}

template<class M>
SelectedWordSlowLM<M>::~SelectedWordSlowLM() {
  delete model_;
}

template<class M>
inline Float SelectedWordSlowLM<M>::scoreNgram(const StateType_ &state,
		lm::WordIndex word, WordState_ &out_state) const {
	Float s = model_->Score(state, word, out_state.first);
	s *= Float(2.30258509299405); // log10 -> ln
	out_state.second = s;
	return s;
}




template<class M>
FeatureFunction::State *SelectedWordSlowLM<M>::initDocument(const DocumentState &doc, Scores::iterator sbegin) const {
  const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();

  SelectedWordSlowLMState *s = new SelectedWordSlowLMState(segs.size());

  uint count=0;
  for(uint i = 0; i < segs.size(); i++) {
    BOOST_FOREACH(const AnchoredPhrasePair &app, segs[i]) {
      if (s->AddWord(i,app,maxWordLength)){
	count++;
      }
    }
  }

  StateType_ state(model_->BeginSentenceState()), out_state;
  const VocabularyType_ &vocab = model_->GetVocabulary();

  Float score;
  for (uint i = 0; i < s->sentWords.size(); ++i){
    for (uint j = 0; j < s->sentWords[i].size(); ++j){
      score += model_->Score(state,vocab.Index(s->sentWords[i][j]),out_state);
      // LOG(logger_, debug, "add score for " << s->sentWords[i][j] << " = " << score);
      state = out_state;
    }
  }

  s->currentScore = score;
  *sbegin = score;
  return s;
}


template<class M>
void SelectedWordSlowLM<M>::computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const {
	*sbegin = Float(0);
}





template<class M>
FeatureFunction::StateModifications *SelectedWordSlowLM<M>::estimateScoreUpdate(const DocumentState &doc, const SearchStep &step, const State *state,
		Scores::const_iterator psbegin, Scores::iterator sbegin) const {

  const SelectedWordSlowLMState *prevstate = dynamic_cast<const SelectedWordSlowLMState *>(state);
  SelectedWordSlowLMState *s = prevstate->clone();

  uint sentNo = 0;
  uint lastPos = 0;

  // run through all modifications and check if we need to update the word list
  const std::vector<SearchStep::Modification> &mods = step.getModifications();
  std::vector<SearchStep::Modification>::const_iterator it = mods.begin();
  while (it!=mods.end()){

    PhraseSegmentation::const_iterator from_it = it->from_it;
    PhraseSegmentation::const_iterator to_it = it->to_it;

    const PhraseSegmentation &current = doc.getPhraseSegmentation(it->sentno);
    if (sentNo != it->sentno){
      sentNo = it->sentno;
      s->sentWords[sentNo].clear();
      for (PhraseSegmentation::const_iterator pit=current.begin(); pit!=from_it; pit++) {
	s->AddWord(sentNo,*pit,maxWordLength);
      }
      lastPos = 0;
    }
    if ( lastPos > std::distance(current.begin(), from_it) ){
      LOG(logger_, debug, "WARNING! Modifications are not in target sentence order! " 
	  << lastPos << "  " <<  std::distance(current.begin(), from_it));
    }
    lastPos = std::distance(current.begin(), from_it);

    // record the words in the proposed modification (and compare to current word list)
    BOOST_FOREACH(const AnchoredPhrasePair &app, it->proposal) {
      s->AddWord(sentNo,app,maxWordLength);
    }

    it++;
    if (it!=mods.end()){
      if (sentNo != it->sentno){
	for (PhraseSegmentation::const_iterator pit=to_it; pit!=current.end(); pit++) {
	  s->AddWord(sentNo,*pit,maxWordLength);
	}
      }
    }
  }


  // TODO: this is stupid: re-score the entire document!

  StateType_ in_state(model_->BeginSentenceState()), out_state;
  const VocabularyType_ &vocab = model_->GetVocabulary();

  Float score;
  for (uint i = 0; i < s->sentWords.size(); ++i){
    for (uint j = 0; j < s->sentWords[i].size(); ++j){
      score += model_->Score(in_state,vocab.Index(s->sentWords[i][j]),out_state);
      // LOG(logger_, debug, "add score for " << s->sentWords[i][j] << " = " << score);
      in_state = out_state;
    }
  }
  LOG(logger_, debug, "new LM score " << score);
  s->currentScore = score;

  *sbegin = s->score();
  return s;
}


template<class M>
FeatureFunction::StateModifications *SelectedWordSlowLM<M>::updateScore(const DocumentState &doc, const SearchStep &step, const State *state,
		FeatureFunction::StateModifications *estmods, Scores::const_iterator psbegin, Scores::iterator estbegin) const {
	return estmods;
}

template<class M>
FeatureFunction::State *SelectedWordSlowLM<M>::applyStateModifications(FeatureFunction::State *oldState, FeatureFunction::StateModifications *modif) const {
	SelectedWordSlowLMState *os = dynamic_cast<SelectedWordSlowLMState *>(oldState);
	SelectedWordSlowLMState *ms = dynamic_cast<SelectedWordSlowLMState *>(modif);

	os->currentScore = ms->currentScore;
	os->sentWords.swap(ms->sentWords);

	return oldState;
}
