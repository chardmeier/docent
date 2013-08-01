/*
 *  ReverseNgramModel.h
 *
 *  Copyright 2012 by Christian Hardmeier. All rights reserved.
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

#ifndef docent_ReverseNgramModel_h
#define docent_ReverseNgramModel_h

#include "FeatureFunction.h"

// from kenlm
#include "lm/binary_format.hh"
#include "lm/model.hh"

struct ReverseNgramModelFactory {
	static FeatureFunction *createNgramModel(const Parameters &);
};

template<class M> struct NgramDocumentState;
template<class M> struct NgramDocumentModifications;

template<class Model>
class ReverseNgramModel : public FeatureFunction {
	friend class ReverseNgramModelFactory;
	friend struct NgramDocumentState<ReverseNgramModel<Model> >;
	friend struct NgramDocumentModifications<ReverseNgramModel<Model> >;

private:
	typedef typename Model::Vocabulary VocabularyType_;
	typedef typename Model::State StateType_;

	typedef NgramDocumentState<ReverseNgramModel<Model> > NgramDocumentState_;
	typedef NgramDocumentModifications<ReverseNgramModel<Model> > NgramDocumentModifications_;

	typedef std::pair<StateType_,Float> WordState_;
	typedef std::vector<WordState_> SentenceState_;

	Float scoreNgram(const StateType_ &old_state, lm::WordIndex word, WordState_ &out_state) const;

	template<bool ScoreCompleteSentence,class PhrasePairIterator,class StateIterator>
	Float scorePhraseSegmentation(const StateType_ *last_state, PhrasePairIterator from_it,
		PhrasePairIterator to_it, PhrasePairIterator eos,
		StateIterator state_it, bool atEos = false) const;

 protected:
	mutable Logger logger_;
	Model *model_;
	Model getModel() { return model_; };
	Model getLogger() { return logger_; };
	ReverseNgramModel(const std::string &file);

	// TODO: I don't know why I had to add an empty constructor to make ReverseReverseNgramModel compile (Joerg)
	ReverseNgramModel() {};

 public:
	virtual ~ReverseNgramModel() { delete model_; };

	virtual FeatureFunction::State *initDocument(const DocumentState &doc, Scores::iterator sbegin) const;

	virtual StateModifications *estimateScoreUpdate(const DocumentState &doc, const SearchStep &step, const State *state,
		Scores::const_iterator psbegin, Scores::iterator sbegin) const;
	virtual StateModifications *updateScore(const DocumentState &doc, const SearchStep &step, const State *state,
		StateModifications *estmods, Scores::const_iterator psbegin, Scores::iterator estbegin) const;

	virtual FeatureFunction::State *applyStateModifications(FeatureFunction::State *oldState, FeatureFunction::StateModifications *modif) const;

	virtual void computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const;
	
	virtual uint getNumberOfScores() const {
		return 1;
	}
};



#endif
