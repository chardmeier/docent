/*
 *  SelectedWordLM.cpp
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

#include "SelectedWordLM.h"

#include "DocumentState.h"
#include "SearchStep.h"

#include <cmath>
#include <vector>

#include "lm/binary_format.hh"  // from kenlm
#include "lm/model.hh"

template<class Model>
class SelectedWordLM
: public FeatureFunction
{
	friend struct SelectedWordLMFactory;

private:
	typedef typename Model::Vocabulary VocabularyType_;
	typedef typename Model::State StateType_;

	typedef std::pair<StateType_,Float> WordState_;
	typedef std::vector<WordState_> SentenceState_;

	mutable Logger logger_;
	Model *model_;
	uint minWordLength;

	SelectedWordLM(
		const std::string &file,
		const Parameters &params
	);
	Float scoreNgram(
		const StateType_ &old_state,
		lm::WordIndex word,
		WordState_ &out_state
	) const;

public:
	virtual ~SelectedWordLM();
	virtual State *initDocument(
		const DocumentState &doc,
		Scores::iterator sbegin
	) const;
	virtual StateModifications *estimateScoreUpdate(
		const DocumentState &doc,
		const SearchStep &step,
		const State *state,
		Scores::const_iterator psbegin,
		Scores::iterator sbegin
	) const;
	virtual StateModifications *updateScore(
		const DocumentState &doc,
		const SearchStep &step,
		const State *state,
		StateModifications *estmods,
		Scores::const_iterator,
		Scores::iterator estbegin
	) const;

	virtual FeatureFunction::State *applyStateModifications(
		FeatureFunction::State *oldState,
		FeatureFunction::StateModifications *modif
	) const;

	virtual uint getNumberOfScores() const {
		return 1;
	}

	virtual void computeSentenceScores(
		const DocumentState &doc,
		uint sentno,
		Scores::iterator sbegin
	) const;
};


struct SelectedWord {
	SelectedWord(
		uint pn,
		uint wn,
		std::string s,
		std::string t
	) : srcWord(""),
		trgWord(""),
		phrNo(0),
		wordNo(0)
	{
		phrNo = pn;
		wordNo = wn;
		srcWord = s;
		trgWord = t;
	};

	/*
	SelectedWord(uint pn, std::string t)  :
		phrNo(0), wordNo(0), srcWord(""), trgWord("")
	{
		phrNo = pn;
		trgWord = t;
	};
	*/

	std::string srcWord, trgWord;
	uint phrNo, wordNo;
};


struct SelectedWordLMState
:	public FeatureFunction::State,
	public FeatureFunction::StateModifications
{
	SelectedWordLMState(
		uint nsents
	) :	logger_("SelectedWordLM"),
		currentScore(0)
	{
		selectedWords.resize(nsents);
	};

	std::vector< std::vector< SelectedWord > > selectedWords;

	mutable Logger logger_;
	Float currentScore;

	Float score() {
		return currentScore;
	};

	std::string GetWords(
		uint sentno
	) {
		std::string words = "";
		for(uint i=0; i<selectedWords[sentno].size(); i++) {
			words += selectedWords[sentno][i].trgWord+" ";
		}
		return words;
	}

	void GetHistory(
		std::vector<std::string>& history,
		uint sentNo,
		const uint size
	) {
		while(history.size() < size) {
			std::vector<SelectedWord>::const_iterator
				it = selectedWords[sentNo].end();
			while(it != selectedWords[sentNo].begin()) {
				it--;
				history.push_back(it->trgWord);
				if(history.size() == size)
					return;
			}
			if(sentNo == 0)
				break;
			sentNo--;
		}
	}

	void GetFuture(
		std::vector<std::string>& future,
		uint sentNo,
		const uint idx,
		const uint size,
		const uint stopSentNo,
		const uint stopPhrNo
	) {
		std::vector<SelectedWord>::const_iterator
			it = selectedWords[sentNo].begin()+idx;
		while(future.size() < size) {
			while(it != selectedWords[sentNo].end()) {
				if(sentNo == stopSentNo && it->phrNo == stopPhrNo)
					return;
				future.push_back((*it).trgWord);
				if(future.size() == size)
					return;
				it++;
			}
			sentNo++;
			if(sentNo >= selectedWords.size())
				return;
			if(sentNo > stopSentNo)
				return;
			it = selectedWords[sentNo].begin();
		}
	}

	void ClearWords(const uint sentNo) {
		selectedWords[sentNo].clear();
	};

	void CopyWords(
		const SelectedWordLMState& state,
		const uint sentno,
		const uint start,
		const uint end,
		const int diff
	) {
		for(uint i = 0; i < state.selectedWords[sentno].size(); ++i) {
			if(state.selectedWords[sentno][i].phrNo >= start) {
				if(state.selectedWords[sentno][i].phrNo < end) {
					selectedWords[sentno].push_back(state.selectedWords[sentno][i]);
					if(diff != 0)
						selectedWords[sentno][selectedWords[sentno].size()-1].phrNo += diff;
				}
			}
		}
	};

	uint AddWord(
		const uint sentno,
		const uint phrno,
		const AnchoredPhrasePair &app,
		const uint minLength
	) {
		WordAlignment wa = app.second.get().getWordAlignment();
		PhraseData sd = app.second.get().getSourcePhrase().get();
		PhraseData td = app.second.get().getTargetPhrase().get();

		uint addCount=0;
		for(uint j=0; j<sd.size(); ++j) {
			// just for testing: words longer than 5 characters .... (should use some other criteria here!)
			if((minLength == 0) || (sd[j].size() >= minLength)) {
				for(WordAlignment::const_iterator
					wit = wa.begin_for_source(j);
					wit != wa.end_for_source(j);
					++wit
				) {
					SelectedWord word(
						phrno,
						*wit,
						sd[j],
						td[*wit]
					);
					selectedWords[sentno].push_back(word);
					addCount++;
					// LOG(logger_, debug, "add word " << td[*wit] << " aligned to " << sd[j]);
				}
			}
		}
		return addCount;
	};

	virtual SelectedWordLMState *clone() const {
		return new SelectedWordLMState(*this);
	}
};


FeatureFunction
*SelectedWordLMFactory::createNgramModel(
	const Parameters &params
) {
	std::string file = params.get<std::string>("lm-file");
	lm::ngram::ModelType mtype;

	std::string smtype = params.get<std::string>("model-type", "");

	Logger logger("SelectedWordLM");

	if(!lm::ngram::RecognizeBinary(file.c_str(), mtype)) {
		if(smtype.empty() || smtype == "hash-probing")
			mtype = lm::ngram::HASH_PROBING;
		else if(smtype == "trie-sorted")
			mtype = lm::ngram::TRIE_SORTED;
		else if(smtype == "quant-trie-sorted")
			mtype = lm::ngram::QUANT_TRIE_SORTED;
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

		return new SelectedWordLM<lm::ngram::ProbingModel>(file,params);
	case lm::ngram::TRIE_SORTED:
		if(!smtype.empty() && smtype != "trie-sorted")
			LOG(logger, error, "Incorrect LM type in configuration for file " << file);
		return new SelectedWordLM<lm::ngram::TrieModel>(file,params);
	case lm::ngram::QUANT_ARRAY_TRIE:
		if(!smtype.empty() && smtype != "quant-trie-sorted")
			LOG(logger, error, "Incorrect LM type in configuration for file " << file);

		return new SelectedWordLM<lm::ngram::QuantArrayTrieModel>(file,params);
	default:
		LOG(logger, error, "Unsupported LM type for file " << file);
		BOOST_THROW_EXCEPTION(FileFormatException());
	}
}


template<class Model>
SelectedWordLM<Model>::SelectedWordLM(
	const std::string &file,
	const Parameters &params
) : logger_("SelectedWordLM") {
	model_ = new Model(file.c_str());
	minWordLength = params.get<uint>("min-word-length");

}

template<class M>
SelectedWordLM<M>::~SelectedWordLM() {
	delete model_;
}

template<class M>
inline Float
SelectedWordLM<M>::scoreNgram(
	const StateType_ &state,
	lm::WordIndex word,
	WordState_ &out_state
) const {
	Float s = model_->Score(state, word, out_state.first);
	s *= Float(2.30258509299405); // log10 -> ln
	out_state.second = s;
	return s;
}


template<class M>
FeatureFunction::State
*SelectedWordLM<M>::initDocument(
	const DocumentState &doc,
	Scores::iterator sbegin
) const {
	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();
	SelectedWordLMState *s = new SelectedWordLMState(segs.size());

	for(uint i = 0; i < segs.size(); i++) {
		uint phrCount=0;
		BOOST_FOREACH(const AnchoredPhrasePair &app, segs[i]) {
			s->AddWord(i, phrCount, app, minWordLength);
			phrCount++;
		}
	}

	StateType_ state(model_->BeginSentenceState()), out_state;
	const VocabularyType_ &vocab = model_->GetVocabulary();

	Float score;
	for(uint i = 0; i != s->selectedWords.size(); ++i) {
		for(uint j = 0; j != s->selectedWords[i].size(); ++j) {
			score += model_->Score(state,vocab.Index(s->selectedWords[i][j].trgWord),out_state);
			// LOG(logger_, debug, "add score for " << s->selectedWords[i][j].trgWord << " = " << score);
			state = out_state;
		}
	}
	// TODO: Do we need end-of-sentence?

	s->currentScore = score;
	*sbegin = score;
	return s;
}


template<class M>
void SelectedWordLM<M>::computeSentenceScores(
	const DocumentState &doc,
	uint sentno,
	Scores::iterator sbegin
) const {
	*sbegin = Float(0);
}

// TODO: do something smart to estimateScoreUpdate and move the code below to updateScore
// TODO: avoid recomputing scores if nothing actually changes (in the list of selected words)

template<class M>
FeatureFunction::StateModifications
*SelectedWordLM<M>::estimateScoreUpdate(
	const DocumentState &doc,
	const SearchStep &step,
	const State *state,
	Scores::const_iterator psbegin,
	Scores::iterator sbegin
) const {
	const SelectedWordLMState *prevstate = dynamic_cast<const SelectedWordLMState *>(state);
	SelectedWordLMState *s = prevstate->clone();

	bool firstSent = true;
	uint sentNo = 0;
	uint phrNo = 0;
	int phrNoDiff = 0;

	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();
	const uint docSize = segs.size();

	uint lmOrder = model_->Order();
	const VocabularyType_ &vocab = model_->GetVocabulary();

	StateType_ add_state;
	StateType_ remove_state;
	StateType_ out_state;

	// run through all modifications and check if we need to update the word list
	const std::vector<SearchStep::Modification>
		&mods = step.getModifications();
	std::vector<SearchStep::Modification>::const_iterator
		it = mods.begin();
	while(it != mods.end()) {
		PhraseSegmentation::const_iterator from_it = it->from_it;
		PhraseSegmentation::const_iterator to_it = it->to_it;

		const PhraseSegmentation &current = doc.getPhraseSegmentation(it->sentno);
		phrNo = std::distance(
			current.begin(),
			from_it
		);

		if(firstSent || (sentNo != it->sentno)) {
			sentNo = it->sentno;
			firstSent = false;

			LOG(logger_, debug, "word list before: " << s->GetWords(sentNo));
			phrNoDiff = 0;

			// start from scratch:
			// copy words from all phrases before the current modification
			s->ClearWords(sentNo);
			s->CopyWords(
				*prevstate,
				sentNo,
				0,
				phrNo,
				phrNoDiff
			);

			// get the LM history before the modification (initialize LM states)
			std::vector<std::string> history;
			s->GetHistory(
				history,
				sentNo,
				lmOrder-1
			);

			if(history.size() < lmOrder-1) {
				// TODO: Do we need begin-of-sentence?
				StateType_ begin_state(model_->BeginSentenceState());
				add_state = begin_state;
			} else {
				StateType_ begin_state(model_->NullContextState());
				add_state = begin_state;
			}

			while(!history.empty()) {
				// TODO: change state without scoring
				model_->Score(
					add_state,
					vocab.Index(history.back()),
					out_state
				);
				add_state = out_state;
				history.pop_back();
			}
			remove_state = add_state;
		}

		//-------------------------------------------------------
		// remove scores for modified part
		//-------------------------------------------------------

		StateType_ out_state;
		uint oldLength = 0;
		for(PhraseSegmentation::const_iterator
			pit = from_it;
			pit != to_it;
			++pit
		) {
			uint phr = std::distance(current.begin(), pit);

			if(prevstate->selectedWords[sentNo].size() > 0) {
				for(uint
					i = 0;
					i < prevstate->selectedWords[sentNo].size();
					++i
				) {
					if(prevstate->selectedWords[sentNo][i].phrNo > phr)
						break;
					if(prevstate->selectedWords[sentNo][i].phrNo == phr) {
						Float score = model_->Score(
							remove_state,
							vocab.Index(prevstate->selectedWords[sentNo][i].trgWord),
							out_state
						);
						s->currentScore -= score;
						remove_state = out_state;
						LOG(logger_, debug, "(a) remove score for "
							<< prevstate->selectedWords[sentNo][i].trgWord
							<< " (" << score << ")"
						);
					}
				}
			}
			oldLength++;
		}

		//-------------------------------------------------------
		// add scores
		//-------------------------------------------------------

		// record the words in the proposed modification (and compare to current word list)
		uint modLength = 0;
		BOOST_FOREACH(const AnchoredPhrasePair &app, it->proposal) {
			uint newPhrNo = phrNo+modLength+phrNoDiff;
			uint added = s->AddWord(sentNo, newPhrNo, app, minWordLength);
			for(uint
				j = s->selectedWords[sentNo].size() - added;
				j < s->selectedWords[sentNo].size();
				++j
			) {
				Float score = model_->Score(
					add_state,
					vocab.Index(s->selectedWords[sentNo][j].trgWord),
					out_state
				);
				s->currentScore += score;
				add_state = out_state;
				LOG(logger_, debug, "(b) add score for "
					<<  s->selectedWords[sentNo][j].trgWord
					<< " (" << score << ")"
				);
			}
			modLength++;
		}

		// accumulate length difference between modification and original
		phrNoDiff += modLength - oldLength;

		//------------------------------------------------------------------------------------
		// remove scores for subsequent words but not further than start of next modification
		//------------------------------------------------------------------------------------

		it++;
		uint currentSentNo = sentNo;
		uint nextModSentNo = docSize-1;
		uint nextModFrom = std::distance(
			segs[segs.size()-1].begin(),
			segs[segs.size()-1].end()
		);

		if(it!=mods.end()) {
			const PhraseSegmentation &next = doc.getPhraseSegmentation(it->sentno);
			nextModSentNo = it->sentno;
			nextModFrom = std::distance(
				next.begin(),
				it->from_it
			);
		}

		// don't forget to copy remaining selected words in currentSentNo!
		phrNo = std::distance(current.begin(), to_it);
		uint wordIdx = s->selectedWords[sentNo].size();

		if(phrNo < current.size()) {
			if(currentSentNo == nextModSentNo) {
				s->CopyWords(
					*prevstate,
					currentSentNo,
					phrNo,
					nextModFrom,
					phrNoDiff
				);
			} else {
				s->CopyWords(
					*prevstate,
					currentSentNo,
					phrNo,
					current.size(),
					phrNoDiff
				);
			}
		}
		LOG(logger_, debug, " word list after: " << s->GetWords(currentSentNo));

		// run through more context words as needed by LM until next modification point or end of document

		std::vector<std::string> future;
		s->GetFuture(
			future,
			sentNo,
			wordIdx,
			lmOrder-1,
			nextModSentNo,
			nextModFrom
		);

		for(std::vector<std::string>::const_iterator
			it = future.begin();
			it != future.end();
			++it
		) {
			Float oldScore = model_->Score(
				remove_state,
				vocab.Index(*it),
				out_state
			);
			remove_state = out_state;
			Float newScore = model_->Score(
				add_state,
				vocab.Index(*it),
				out_state
			);
			add_state = out_state;
			s->currentScore -= oldScore;
			s->currentScore += newScore;

			LOG(logger_, debug, "(c) change score for " <<  *it
				<< " (old: " << oldScore
				<< ", new: " << newScore << ")"
			);
		}

		// TODO: Do we need end-of-sentence?
		/*
		if(future.size() < lmOrder-1) {
			if(nextModSentNo == docSize-1) {
				if(nextModFrom == std::distance(segs[segs.size()-1].begin(), segs[segs.size()-1].end())) {
					Float oldScore = model_->Score(remove_state,vocab.EndSentence(),out_state);
					remove_state = out_state;
					Float newScore = model_->Score(add_state,vocab.EndSentence(),out_state);
					add_state = out_state;
					s->currentScore -= oldScore;
					s->currentScore += newScore;

					LOG(logger_, debug, "(c) change score for EOS " << " (old: " << oldScore << ", new: " << newScore << ")");
				}
			}
		}
		*/
	}

	////////////////////////////////////////////////////////
	// DEBUG: compare with computing the score from scratch
	////////////////////////////////////////////////////////
	if(logger_.loggable(debug)) {
		StateType_ in_state(model_->BeginSentenceState()), out_state;
		const VocabularyType_ &vocab = model_->GetVocabulary();

		Float score;
		for(uint i = 0; i < s->selectedWords.size(); ++i) {
			for(uint j = 0; j < s->selectedWords[i].size(); ++j) {
				score += model_->Score(
					in_state,
					vocab.Index(s->selectedWords[i][j].trgWord),
					out_state
				);
				in_state = out_state;
			}
		}
		if(score < s->currentScore-0.001 || score > s->currentScore+0.001)
			LOG(logger_, debug, "score difference: " << s->currentScore << " != " << score);
	}
	////////////////////////////////////////////////////////


	LOG(logger_, debug, "new LM score " << s->currentScore);
	*sbegin = s->score();
	return s;
}


template<class M>
FeatureFunction::StateModifications
*SelectedWordLM<M>::updateScore(
	const DocumentState &doc,
	const SearchStep &step,
	const State *state,
	FeatureFunction::StateModifications *estmods,
	Scores::const_iterator psbegin,
	Scores::iterator estbegin
) const {
	return estmods;
}

template<class M>
FeatureFunction::State
*SelectedWordLM<M>::applyStateModifications(
	FeatureFunction::State *oldState,
	FeatureFunction::StateModifications *modif
) const {
	SelectedWordLMState *os = dynamic_cast<SelectedWordLMState *>(oldState);
	SelectedWordLMState *ms = dynamic_cast<SelectedWordLMState *>(modif);

	os->currentScore = ms->currentScore;
	os->selectedWords.swap(ms->selectedWords);

	return oldState;
}
