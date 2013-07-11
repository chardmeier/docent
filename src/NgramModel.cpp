/*
 *  NgramModel.cpp
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

#include "Docent.h"

#include "DocumentState.h"
#include "FeatureFunction.h"
#include "NgramModel.h"
//#include "NgramModelIrstlm.h"
#include "PhrasePair.h"
#include "PiecewiseIterator.h"
#include "SearchStep.h"

#include <algorithm>
#include <vector>

#include<iostream>

// from kenlm
#include "lm/binary_format.hh"
#include "lm/model.hh"

#include <boost/foreach.hpp>

template<class M> struct NgramDocumentState;
template<class M> struct NgramDocumentModifications;

template<class Model>
class NgramModel : public FeatureFunction {
	friend class NgramModelFactory;
	friend struct NgramDocumentState<NgramModel<Model> >;
	friend struct NgramDocumentModifications<NgramModel<Model> >;

private:
	typedef typename Model::Vocabulary VocabularyType_;
	typedef typename Model::State StateType_;

	typedef NgramDocumentState<NgramModel<Model> > NgramDocumentState_;
	typedef NgramDocumentModifications<NgramModel<Model> > NgramDocumentModifications_;

	typedef std::pair<StateType_,Float> WordState_;
	typedef std::vector<WordState_> SentenceState_;

	mutable Logger logger_;

	Model *model_;

	NgramModel(const std::string &file, int annotation_level);

	Float scoreNgram(const StateType_ &old_state, lm::WordIndex word, WordState_ &out_state) const;

	template<bool ScoreCompleteSentence,class PhrasePairIterator,class StateIterator>
	Float scorePhraseSegmentation(const StateType_ *last_state, PhrasePairIterator from_it,
		PhrasePairIterator to_it, PhrasePairIterator eos,
		StateIterator state_it, bool atEos = false) const;

	int annotation_level;

public:
	virtual ~NgramModel();

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

FeatureFunction *NgramModelFactory::createNgramModel(const Parameters &params) {
	std::string file = params.get<std::string>("lm-file");
	lm::ngram::ModelType mtype;

	int annotation_level = params.get<uint>("annotation-level", -1);

	std::string smtype = params.get<std::string>("model-type", "");

	Logger logger("NgramModel");

	if(!lm::ngram::RecognizeBinary(file.c_str(), mtype)) {
		if(smtype.empty() || smtype == "hash-probing")
			mtype = lm::ngram::HASH_PROBING;
		else if(smtype == "trie-sorted")
			mtype = lm::ngram::TRIE_SORTED;
		//else if(smtype == "irstlm")
			//return new NgramModelIrstlm(params);
		//else if(smtype == "quant-trie-sorted")
			//mtype = lm::ngram::QUANT_TRIE_SORTED;
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

		return new NgramModel<lm::ngram::ProbingModel>(file,annotation_level);

	case lm::ngram::TRIE_SORTED:
		if(!smtype.empty() && smtype != "trie-sorted")
			LOG(logger, error, "Incorrect LM type in configuration "
				"for file " << file);
		return new NgramModel<lm::ngram::TrieModel>(file,annotation_level);

/*
	case lm::ngram::QUANT_TRIE_SORTED:
		if(!smtype.empty() && smtype != "quant-trie-sorted")
			LOG(logger, error, "Incorrect LM type in configuration "
				"for file " << file);

		return new NgramModel<lm::ngram::QuantTrieModel>(file);
*/
	default:
		LOG(logger, error, "Unsupported LM type for file " << file);
		BOOST_THROW_EXCEPTION(FileFormatException());
	}
}

template<class M>
struct NgramDocumentState : public FeatureFunction::State {
	std::vector<typename M::SentenceState_> lmCache;
	
	virtual FeatureFunction::State *clone() const {
		return new NgramDocumentState(*this);
	}
};

struct NgramDocumentPreviousScore : public FeatureFunction::StateModifications {
	Float score;

	NgramDocumentPreviousScore(Float s) : score(s) {}
};

template<class M>
struct NgramDocumentModifications : public FeatureFunction::StateModifications {
	std::vector<std::pair<uint,typename M::SentenceState_> > modifications;
};

template<class Model>
NgramModel<Model>::NgramModel(const std::string &file, const int annotation_level) :
		logger_("NgramModel") {
	model_ = new Model(file.c_str());
	this->annotation_level = annotation_level;
	LOG(logger_, debug, "Annotation level of N-gram model set to " << annotation_level);
}

template<class M>
NgramModel<M>::~NgramModel() {
	delete model_;
}

template<class M>
FeatureFunction::State *NgramModel<M>::initDocument(const DocumentState &doc, Scores::iterator sbegin) const {

	NgramDocumentState_ *state = new NgramDocumentState_();
	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();
	state->lmCache.resize(segs.size());
	Float &s = *sbegin;
	for(uint i = 0; i < segs.size(); i++) {
		state->lmCache[i].resize(countTargetWords(segs[i].begin(), segs[i].end()) + 1); // one for </s>
		s += scorePhraseSegmentation<true>(&model_->BeginSentenceState(), segs[i].begin(),
			segs[i].end(), segs[i].end(), state->lmCache[i].begin(), true);
	}
	return state;
}

template<class M>
void NgramModel<M>::computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const {
	const PhraseSegmentation &snt = doc.getPhraseSegmentation(sentno);
	SentenceState_ state(countTargetWords(snt.begin(), snt.end()) + 1);
	*sbegin = scorePhraseSegmentation<true>(&model_->BeginSentenceState(), snt.begin(), snt.end(), snt.end(),
		state.begin(), true);
}

template<class M>
FeatureFunction::StateModifications *NgramModel<M>::estimateScoreUpdate(const DocumentState &doc,
		const SearchStep &step, const FeatureFunction::State *ffstate,
		Scores::const_iterator psbegin, Scores::iterator sbegin) const {
	const NgramDocumentState_ &state = dynamic_cast<const NgramDocumentState_ &>(*ffstate);

	// copy scores and subtract those that will change so updateScore() only adds stuff
	Float s = *psbegin;
	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	std::vector<SearchStep::Modification>::const_iterator it = mods.begin();
	LOG(logger_, debug, "NgramModel::estimateScoreUpdate");
	while(it != mods.end()) {
		LOG(logger_, debug, "next modification");
		uint sentno = it->sentno;
		PhraseSegmentation::const_iterator from_it = it->from_it;
		PhraseSegmentation::const_iterator to_it = it->to_it;

		const PhraseSegmentation &current = doc.getPhraseSegmentation(sentno);
		uint clear_from = countTargetWords(current.begin(), from_it);
		uint w_to = clear_from + countTargetWords(from_it, to_it);
		uint clear_to = w_to + model_->Order() - 1;

		// don't clear further than to the start of the next modification
		++it;
		if(it != mods.end() && it->sentno == sentno) {
			uint next_from = w_to + countTargetWords(to_it, it->from_it);
			if(next_from < clear_to)
				clear_to = next_from;
		}
		// or the end of the sentence
		if(clear_to > state.lmCache[sentno].size())
			clear_to = state.lmCache[sentno].size();

		for(uint i = clear_from; i < clear_to; i++) {
			LOG(logger_, debug, "*** minus " << state.lmCache[sentno][i].second);
			s -= state.lmCache[sentno][i].second;
		}
	}

	*sbegin = s;
	return new NgramDocumentPreviousScore(*psbegin);
}

template<class M>
FeatureFunction::StateModifications *NgramModel<M>::updateScore(const DocumentState &doc,
		const SearchStep &step, const FeatureFunction::State *ffstate,
		StateModifications *estmods, Scores::const_iterator psbegin, Scores::iterator sbegin) const {
	LOG(logger_, debug, "NgramModel::updateScore");
	const NgramDocumentState_ &state = dynamic_cast<const NgramDocumentState_ &>(*ffstate);
	Float &s = *sbegin;
	Float estimated = s;

	NgramDocumentPreviousScore *prevscore = dynamic_cast<NgramDocumentPreviousScore *>(estmods);
	s = prevscore->score;
	delete prevscore;

	NgramDocumentModifications_ *modif = new NgramDocumentModifications_();
	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	std::vector<SearchStep::Modification>::const_iterator it1 = mods.begin();
	while(it1 != mods.end()) {
		uint sentno = it1->sentno;
		std::vector<SearchStep::Modification>::const_iterator it2 = it1;
		while(++it2 != mods.end())
			if(it2->sentno != sentno)
				break;

		const PhraseSegmentation &current = doc.getPhraseSegmentation(sentno);

		modif->modifications.push_back(std::make_pair(sentno, SentenceState_()));
		SentenceState_ &sntstate = modif->modifications.back().second;
		sntstate.reserve(state.lmCache[sentno].size()); // an approximation

		typename SentenceState_::const_iterator oldstate1 = state.lmCache[sentno].begin();
		PhraseSegmentation::const_iterator next_from_it = it1->from_it;
		typename SentenceState_::const_iterator oldstate2 = oldstate1 +
			countTargetWords(current.begin(), next_from_it);
		sntstate.insert(sntstate.end(), oldstate1, oldstate2);

		// keep a copy of the last state to avoid problems with invalidating iterators
		StateType_ last_state;
		if(sntstate.empty())
			last_state = model_->BeginSentenceState();
		else
			last_state = sntstate.back().first;

		std::vector<PhraseSegmentation::const_iterator> pieces;
		pieces.push_back(current.begin());
		pieces.push_back(next_from_it);

		for(std::vector<SearchStep::Modification>::const_iterator modit = it1; modit != it2; ++modit) {
			LOG(logger_, debug, "next modification");
			PhraseSegmentation::const_iterator from_it = modit->from_it;
			PhraseSegmentation::const_iterator to_it = modit->to_it;
			const PhraseSegmentation &proposal = modit->proposal;

			bool last_mod_in_sentence;
			if(modit + 1 != it2) {
				last_mod_in_sentence = false;
				next_from_it = (modit + 1)->from_it;
			} else {
				last_mod_in_sentence = true;
				next_from_it = current.end();
			}

			uint pieceIndex = pieces.size();
			pieces.push_back(proposal.begin());
			pieces.push_back(proposal.end());
			pieces.push_back(to_it);
			pieces.push_back(next_from_it);

			uint state_pos = sntstate.size();
			oldstate1 = oldstate2 + countTargetWords(from_it, to_it);
			
			for(typename SentenceState_::const_iterator pit = oldstate2; pit != oldstate1; ++pit) {
				LOG(logger_, debug, "(a) minus " << pit->second);
				s -= pit->second;
			}
			sntstate.insert(sntstate.end(), countTargetWords(proposal), WordState_());

			if(last_mod_in_sentence)
				oldstate2 = state.lmCache[sentno].end(); // also take along the </s> token
			else
				oldstate2 = oldstate1 + countTargetWords(to_it, next_from_it);

			sntstate.insert(sntstate.end(), oldstate1, oldstate2);

			typedef PiecewiseIterator<std::vector<PhraseSegmentation::const_iterator>::const_iterator> PieceIt;
			PieceIt from_piece(pieces.begin(), pieces.begin() + pieceIndex - 2, pieces.end(), from_it);
			PieceIt to_piece(pieces.begin(), pieces.begin() + pieceIndex + 2, pieces.end(), to_it);
			PieceIt end_piece(pieces.begin(), pieces.end() - 2, pieces.end(), next_from_it);
			s += scorePhraseSegmentation<false>(&last_state, from_piece, to_piece, end_piece,
				sntstate.begin() + state_pos, last_mod_in_sentence);
			
			if(!sntstate.empty())
				last_state = sntstate.back().first;

			it1 = it2;
		}
	}

	assert(s <= estimated);
	return modif;
}

template<class M>
FeatureFunction::State *NgramModel<M>::applyStateModifications(FeatureFunction::State *oldState,
		FeatureFunction::StateModifications *modif) const {
	NgramDocumentState_ &state = dynamic_cast<NgramDocumentState_ &>(*oldState);
	NgramDocumentModifications_ *mod = dynamic_cast<NgramDocumentModifications_ *>(modif);
	for(typename std::vector<std::pair<uint,SentenceState_> >::iterator it = mod->modifications.begin();
			it != mod->modifications.end(); ++it)
		state.lmCache[it->first].swap(it->second);
	return oldState;
}

template<class M>
inline Float NgramModel<M>::scoreNgram(const StateType_ &state,
		lm::WordIndex word, WordState_ &out_state) const {
	Float s = model_->Score(state, word, out_state.first);
	s *= Float(2.30258509299405); // log10 -> ln
	out_state.second = s;
	return s;
}

template<class M>
template<bool ScoreCompleteSentence,class PhrasePairIterator,class StateIterator>
Float NgramModel<M>::scorePhraseSegmentation(const StateType_ *last_state, PhrasePairIterator from_it,
		PhrasePairIterator to_it, PhrasePairIterator eos, StateIterator state_it, bool atEos) const {
	const VocabularyType_ &vocab = model_->GetVocabulary();

	PhrasePairIterator ng_it = from_it;	

	uint last_statelen = last_state->Length();

	LOG(logger_, debug, 4);
	Float s = .0;
	while(ng_it != to_it) {
		LOG(logger_, debug, "running (a) loop");
		for(PhraseData::const_iterator wi = ng_it->second.get().getTargetPhraseOrAnnotations(annotation_level).get().begin();
				wi != ng_it->second.get().getTargetPhraseOrAnnotations(annotation_level).get().end(); ++wi) {
			Float lscore = scoreNgram(*last_state, vocab.Index(*wi), *state_it);
			// old score has already been subtracted
			last_state = &state_it->first;
			++state_it;
			s += lscore;
			LOG(logger_, debug, "(a) plus " << lscore << "\t" << *wi);
		}
		++ng_it;
	}
	
	LOG(logger_, debug, 5);
	uint future = 1;
	bool independent = false;
	while(!ScoreCompleteSentence && ng_it != eos && !independent) {
		LOG(logger_, debug, "running (b) loop");
		for(PhraseData::const_iterator wi = ng_it->second.get().getTargetPhraseOrAnnotations(annotation_level).get().begin();
				wi != ng_it->second.get().getTargetPhraseOrAnnotations(annotation_level).get().end(); ++wi) {
			if(future > last_state->Length() && future > last_statelen) {
				LOG(logger_, debug, "breaking, future = " << future
					<< ", last state size is " << uint(last_state->Length())
					<< ", old state size was " << uint(state_it->first.Length())
					<< ", last_statelen is " << last_statelen);
				independent = true;
				break;
			}
			++future;

			last_statelen = state_it->first.Length();
			s -= state_it->second;
			LOG(logger_, debug, "(b) minus " << state_it->second);
			Float lscore = scoreNgram(*last_state, vocab.Index(*wi), *state_it);
			last_state = &state_it->first;
			++state_it;
			s += lscore;
			LOG(logger_, debug, "(b) plus " << lscore << "\t" << *wi);
		}
		++ng_it;
	}

	if(future > last_state->Length() && future > last_statelen)
		independent = true;

	LOG(logger_, debug, 6);
	if((ScoreCompleteSentence || atEos) && !independent) {
		if(!ScoreCompleteSentence) {
			s -= state_it->second;
			LOG(logger_, debug, "(c) minus " << state_it->second);
		}
		Float lscore = scoreNgram(*last_state, vocab.EndSentence(), *state_it);
		s += lscore;
		LOG(logger_, debug, "(c) plus " << lscore << "\t</s>");
	}

	return s;
}
