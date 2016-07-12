/*
 *  GappyLanguageModel.cpp
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
 *
 ********************************************************************************
 * parameters (in config file):
 *
 *   name               values        default    description
 * ------------------------------------------------------------------------------
 *   bilingual          true/false    false      use bilingual LM (or not)
 *   skip-unaligned     true/false    false      skip unaligned words (no token)
 *   merge-aligned      true/false    true       merge all aligned trg tokens
 *                                               otherwise: separate token for each link
 */

#include "models/GappyLanguageModel.h"

#include "DocumentState.h"
#include "SearchStep.h"
#include "MMAXDocument.h"

#include <boost/unordered_map.hpp>
#include <boost/regex.hpp>
//#include <boost/xpressive/xpressive.hpp>
#include <boost/algorithm/string.hpp>


#include <set>
#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

#include "lm/binary_format.hh"	// from kenlm
#include "lm/model.hh"

//using namespace boost::xpressive;

template<class Model>
class GappyLanguageModel : public FeatureFunction {
	friend struct GappyLanguageModelFactory;

private:
	typedef typename Model::Vocabulary VocabularyType_;
	typedef typename Model::State StateType_;

	typedef std::pair<StateType_,Float> WordState_;
	typedef std::vector<WordState_> SentenceState_;

	typedef boost::unordered_map< std::string, std::string > SrcMatchCond_;
	typedef boost::unordered_map< uint, std::string > TrgMatchCond_;
	typedef std::vector< SrcMatchCond_ > AltSrcMatchCond_;
	typedef std::vector< TrgMatchCond_ > AltTrgMatchCond_;

	mutable Logger logger_;
	Model *model_;

	AltSrcMatchCond_ srcMatchConditions;
	AltTrgMatchCond_ trgMatchConditions;

	std::set<std::string> requiredAnnotation;
	std::set<uint> requiredFactors;

	// bool useRegExLabel;
	// sregex selectedLabelRegEx;

	bool withinSentence;
	bool bilingual;
	bool skipUnaligned;
	bool mergeAligned;

	GappyLanguageModel(
		const std::string &file,
		const Parameters &params
	);
	Float scoreNgram(
		const StateType_ &old_state,
		lm::WordIndex word,
		WordState_ &out_state
	) const;

public:
	virtual ~GappyLanguageModel();
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
		return 2;
	}

	virtual void computeSentenceScores(
		const DocumentState &doc,
		uint sentno,
		Scores::iterator sbegin
	) const;
};

/*
	selected words:
	- phrNo = position of phrase in target language
	- srcWord = source language word
	- trgWord = aligned target language words
	- wordPair = concatenated source and target language word (for bilingual LMs)
	- srcNo = position or source language word in source sentence
*/


// constructor for merging aligned words
struct GappyLanguageModelSelectedWord
{
	GappyLanguageModelSelectedWord(
		uint sentno,
		uint pn,
		uint sn,
		PhraseData &sd,
		PhraseData &td,
		WordAlignment &wa,
		uint j
	) :	srcWord(""),
		trgWord(""),
		wordPair(""),
		sentNo(0),
		phrNo(0),
		srcNo(0),
		nrLinks(0)
	{
		sentNo = sentno;
		phrNo = pn;
		srcNo = sn;
		nrLinks = 0;

		srcWord = sd[j];
		trgWord = "TRG";		// to make sure that even empty alignments have non-empty string
		for(WordAlignment::const_iterator
			wit = wa.begin_for_source(j);
			wit != wa.end_for_source(j);
			++wit
		) {
			trgWord += "+" + td[*wit];
			nrLinks++;
		}
		wordPair = srcWord + "=" + trgWord;
	};

	// constructor for given word pair
	GappyLanguageModelSelectedWord(
		uint sentno,
		uint pn,
		uint wn,
		std::string s,
		std::string t,
		uint sn
	) :	srcWord(""),
		trgWord(""),
		wordPair(""),
		sentNo(0),
		phrNo(0),
		wordNo(0),
		srcNo(0),
		nrLinks(0)
	{
		sentNo = sentno;
		phrNo = pn;
		wordNo = wn;
		srcWord = s;
		trgWord = t;
		wordPair = s + "=TRG+" + t;
		srcNo = sn;
		nrLinks = 1;
	};

	std::string srcWord, trgWord, wordPair;
	uint sentNo, phrNo, wordNo, srcNo, nrLinks;
};


struct GappyLanguageModelState
:	public FeatureFunction::State,
	public FeatureFunction::StateModifications
{
	GappyLanguageModelState(
		uint nsents
	) :	logger_("GappyLanguageModel"),
		currentScore(0),
		nrSelected(0)
	{
		selectedWords.resize(nsents);
	};

	typedef boost::unordered_map< std::string, std::string > SrcMatchCond_;
	typedef boost::unordered_map< uint, std::string > TrgMatchCond_;
	typedef std::vector< SrcMatchCond_ > AltSrcMatchCond_;
	typedef std::vector< TrgMatchCond_ > AltTrgMatchCond_;

	std::vector< std::vector< GappyLanguageModelSelectedWord > > selectedWords;
	boost::unordered_map<std::string, std::vector< std::vector< std::string > > > selectedLabels;

	mutable Logger logger_;
	Float currentScore;
	uint nrSelected;

	Float score() {
		return currentScore;
	};

	uint GetNrOfWords(uint sentno)
	{
		return selectedWords[sentno].size();
	}

	std::string GetWords(uint sentno)
	{
		std::string words = "";
		for(uint i = 0; i < selectedWords[sentno].size(); i++) {
			words += selectedWords[sentno][i].trgWord + " ";
		}
		return words;
	}

	std::string GetWord(uint sentno,uint wordno,const bool bilingual) const{
		if(bilingual) {
			return selectedWords[sentno][wordno].wordPair;
		}
		return selectedWords[sentno][wordno].trgWord;
	}

	void GetHistory(
		std::vector<std::string>& history,
		uint sentNo,
		const uint size,
		const bool withinSent,
		const bool bilingual
	) {
		while(history.size() < size) {
			std::vector<GappyLanguageModelSelectedWord>::const_iterator
				it = selectedWords[sentNo].end();
			while(it != selectedWords[sentNo].begin()) {
				it--;
				if(bilingual)
					history.push_back(it->wordPair);
				else
					history.push_back(it->trgWord);
				if(history.size() == size)
					return;
			}
			if(sentNo == 0)
				return;	 // stop at document boundary
			if(withinSent)
				return;		// stop at sentence boundary
			sentNo--;									// continue with previous sentence
			// LOG(logger_, debug, "continue with sentence " << sentNo);
		}
	}

	void GetFuture(
		std::vector<std::string>& future,
		uint sentNo,
		const uint idx,
		const uint size,
		const uint stopSentNo,
		const uint stopPhrNo,
		const bool withinSent,
		const bool bilingual
	) {
		std::vector<GappyLanguageModelSelectedWord>::const_iterator
			it = selectedWords[sentNo].begin() + idx;

		while(future.size() < size) {
			while(it != selectedWords[sentNo].end()) {
				if(sentNo == stopSentNo && it->phrNo == stopPhrNo)
					return;
				if(bilingual) {
					future.push_back((*it).wordPair);
				} else {
					future.push_back((*it).trgWord);
				}
				if(future.size() == size)
					return;
				it++;
			}
			if(withinSent)
				return;
			sentNo++;
			if(sentNo >= selectedWords.size())
				return;
			if(sentNo > stopSentNo)
				return;
			it = selectedWords[sentNo].begin();
		}
	}

	void ClearWords(const uint sentNo)
	{
		selectedWords[sentNo].clear();
	};

	void CopyWords(
		const GappyLanguageModelState& state,
		const uint sentno,
		const uint start,
		const uint end,
		const int diff
	) {
		for(uint i = 0; i < state.selectedWords[sentno].size(); i++) {
			if(state.selectedWords[sentno][i].phrNo >= start) {
				if(state.selectedWords[sentno][i].phrNo < end) {
					selectedWords[sentno].push_back(state.selectedWords[sentno][i]);
					if(diff != 0)
						selectedWords[sentno][selectedWords[sentno].size()-1].phrNo += diff;
				}
			}
		}
	};

	void AddLabel(
		const uint sentno,
		const uint wordno,
		const std::string& type,
		const std::string& label
	) {
		selectedLabels[type][sentno].push_back(label);
	}


	bool MatchingSrcWord(
		const uint &sentno,
		PhraseData &pd,
		uint &wordno,
		const AltSrcMatchCond_ &cond
	) {
		if(cond.size() == 0)
			return true;
		for(uint i = 0; i < cond.size(); ++i) {
			bool match = true;
			SrcMatchCond_::const_iterator mit;
			for(mit = cond[i].begin(); mit != cond[i].end(); ++mit) {
				std::string annot = mit->first;
				std::string value = mit->second;
				if(selectedLabels[annot][sentno][wordno] != value) {
					match = false;
					break;
				}
			}
			if(match)
				return match;
		}
		return false;
	}

	bool MatchingTrgWord(
		const AnchoredPhrasePair &app,
		uint wordno,
		const AltTrgMatchCond_ &cond
	) {
		if(cond.size() == 0)
			return true;
		for(uint i = 0; i < cond.size(); ++i) {
			bool match = true;
			TrgMatchCond_::const_iterator mit;
			for(mit = cond[i].begin();mit != cond[i].end(); ++mit) {
				uint factor = mit->first;
				std::string value = mit->second;
				// TODO: is this efficient enough to always look up annotation here?
				PhraseData td = app.second.get().getTargetPhraseOrAnnotations(factor-1,false).get();
				// LOG(logger_, debug, "target " << wordno << " = " << td[wordno] );
				if(td[wordno] != value) {
					match = false;
					break;
				}
			}
			if(match)
				return match;
		}
		return false;
	}

	uint AddWord(
		const uint sentno,
		const uint phrno,
		const AnchoredPhrasePair &app,
		const AltSrcMatchCond_ &srcCond,
		const AltTrgMatchCond_ &trgCond,
		const bool bilingual,const bool skipUnaligned,
		const bool mergeAligned
	) {
		WordAlignment wa = app.second.get().getWordAlignment();
		PhraseData sd = app.second.get().getSourcePhrase().get();
		PhraseData td = app.second.get().getTargetPhrase().get();

		uint wordno = app.first.find_first();
		boost::smatch what;

		uint addCount = 0;
		// if there are conditions over the source language
		// or the LM is bilingual: --> run over source words
		if(srcCond.size() || bilingual) {
			for(uint j = 0; j < sd.size(); ++j) {
				if(MatchingSrcWord(sentno, sd, wordno, srcCond)) {
					if(mergeAligned && trgCond.size() == 0) {
						GappyLanguageModelSelectedWord word(sentno,phrno,wordno,sd,td,wa,j);
						if((!skipUnaligned) || word.nrLinks > 0) {
							selectedWords[sentno].push_back(word);
							LOG(logger_, debug, "add word '" << word.trgWord << "' aligned to '" << word.srcWord);
							addCount++;
							wordno++;
						}
					} else {
						// create separate tokens for each alignment
						for(WordAlignment::const_iterator
							wit = wa.begin_for_source(j);
							wit != wa.end_for_source(j);
							++wit
						) {
							if(MatchingTrgWord(app, *wit, trgCond)) {
								GappyLanguageModelSelectedWord word(
									sentno,
									phrno,
									*wit,
									sd[j],
									td[*wit],
									wordno
								);
								selectedWords[sentno].push_back(word);
								LOG(logger_, debug,
									"add word '" << word.trgWord
									<< "' aligned to '" << word.srcWord
								);
								addCount++;
								wordno++;
							}
						}
					}
				}
			}
		} else {
			// only run through target words that match
			// (does not work for bilingual --> just take arbitrary source word)
			for(uint j = 0; j < td.size(); ++j) {
				if(MatchingTrgWord(app, j, trgCond)) {
					GappyLanguageModelSelectedWord word(
						sentno,
						phrno,
						j,
						sd[0],
						td[j],
						wordno
					);
					selectedWords[sentno].push_back(word);
					LOG(logger_, debug, "add word '" << word.trgWord );
					addCount++;
					wordno++;
				}
			}
		}
		return addCount;
	};

	virtual GappyLanguageModelState *clone()
	const {
		return new GappyLanguageModelState(*this);
	}
};


FeatureFunction
*GappyLanguageModelFactory::createNgramModel(
	const Parameters &params
) {
	std::string file = params.get<std::string>("lm-file");
	lm::ngram::ModelType mtype;

	std::string smtype = params.get<std::string>("model-type", "");

	Logger logger("GappyLanguageModel");

	if(!lm::ngram::RecognizeBinary(file.c_str(), mtype)) {
		if(smtype.empty() || smtype == "hash-probing")
			mtype = lm::ngram::HASH_PROBING;
		else if(smtype == "trie-sorted")
			mtype = lm::ngram::TRIE_SORTED;
		else if(smtype == "quant-trie-sorted")
			mtype = lm::ngram::QUANT_TRIE_SORTED;
		else {
			LOG(logger, error,
				"Unsupported LM type " << smtype << " for file " << file
			);
		}
	}

	switch(mtype) {
	case lm::ngram::HASH_PROBING:
		if(!smtype.empty() && smtype != "hash-probing")
			LOG(logger, error, "Incorrect LM type in configuration for file " << file);
		return new GappyLanguageModel<lm::ngram::ProbingModel>(file,params);
	case lm::ngram::TRIE_SORTED:
		if(!smtype.empty() && smtype != "trie-sorted")
			LOG(logger, error, "Incorrect LM type in configuration for file " << file);
		return new GappyLanguageModel<lm::ngram::TrieModel>(file,params);
	case lm::ngram::QUANT_ARRAY_TRIE:
		if(!smtype.empty() && smtype != "quant-trie-sorted")
			LOG(logger, error, "Incorrect LM type in configuration for file " << file);
		return new GappyLanguageModel<lm::ngram::QuantArrayTrieModel>(file,params);
	default:
		LOG(logger, error, "Unsupported LM type for file " << file);
		BOOST_THROW_EXCEPTION(FileFormatException());
	}
}


template<class Model>
GappyLanguageModel<Model>::GappyLanguageModel(
	const std::string &file,
	const Parameters &params
) :	logger_("GappyLanguageModel")
{
	model_ = new Model(file.c_str());

	// selectedAnnotation = params.get<std::string>("selected-annotation","");
	// selectedLabel = params.get<std::string>("selected-label","");

	// parse source matching conditions ................................

	LOG(logger_, debug, "-------------- match source ----------------");
	std::string cond = params.get<std::string>("source-match","");
	if(cond.length() > 0) {
		std::vector<std::string> disj;
		boost::split(disj, cond, boost::is_any_of("|"));
		for(uint i = 0; i < disj.size(); ++i) {
			std::vector<std::string> conj;
			boost::split(conj, disj[i], boost::is_any_of(" "));
			SrcMatchCond_ srcMatch;
			for(uint j = 0; j < conj.size(); ++j) {
				std::vector<std::string> keyval;
				boost::split(keyval, conj[j], boost::is_any_of("="));
				if(keyval.size() != 2) {
					LOG(logger_, error, "Wrong format for matching condition " << conj[j]);
				} else {
					srcMatch[keyval[0]] = keyval[1];
					requiredAnnotation.insert(keyval[0]);
					LOG(logger_, debug, "add matching condition " << keyval[0] << " = " << keyval[1]);
				}
			}
			srcMatchConditions.push_back(srcMatch);
			LOG(logger_, debug, "-------------- or ----------------");
		}
	}

	// parse target matching conditions ................................

	LOG(logger_, debug, "-------------- match target ----------------");
	cond = params.get<std::string>("target-match","");
	if(cond.length() > 0) {
		std::vector<std::string> disj;
		boost::split(disj, cond, boost::is_any_of("|"));
		for(uint i = 0; i < disj.size(); ++i) {
			std::vector<std::string> conj;
			boost::split(conj, disj[i], boost::is_any_of(" "));
			TrgMatchCond_ trgMatch;
			for(uint j = 0; j < conj.size(); ++j) {
				std::vector<std::string> keyval;
				boost::split(keyval, conj[j], boost::is_any_of("="));
				if(keyval.size() != 2) {
					LOG(logger_, error, "Wrong format for matching condition " << conj[j]);
				} else {
					uint factor = atoi(keyval[0].c_str());
					requiredFactors.insert(factor);
					trgMatch[factor] = keyval[1];
					LOG(logger_, debug, "add matching condition " << factor << " = " << keyval[1]);
				}
			}
			trgMatchConditions.push_back(trgMatch);
			LOG(logger_, debug, "-------------- or ----------------");
		}
	}

	/*
	useRegExLabel = false;
	// LOG(logger_, debug, "label " << selectedLabel);
	if(selectedLabel.empty()) {
		selectedLabel = params.get<std::string>("selected-label-regex","");
		useRegExLabel = true;
		selectedLabelRegEx = sregex::compile(selectedLabel);
		// LOG(logger_, debug, "labelRE " << selectedLabelRegEx );
	}
	*/

	withinSentence = params.get<bool>("sentence-internal-only", false);
	bilingual = params.get<bool>("bilingual", false);
	skipUnaligned = params.get<bool>("skip-unaligned", false);
	mergeAligned = params.get<bool>("merge-aligned", true);
	// LOG(logger_, debug, " bilingual " << bilingual << " withinSent " << withinSentence);
}


template<class M>
GappyLanguageModel<M>::~GappyLanguageModel()
{
	delete model_;
}

template<class M>
inline Float
GappyLanguageModel<M>::scoreNgram(
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
*GappyLanguageModel<M>::initDocument(
	const DocumentState &doc,
	Scores::iterator sbegin
) const {
	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();

	boost::shared_ptr<const MMAXDocument> mmax = doc.getInputDocument();
	// const MarkableLevel &annotationLevel = mmax->getMarkableLevel(selectedAnnotation);

	uint nsents = segs.size();
	GappyLanguageModelState *s = new GappyLanguageModelState(nsents);

	// store all necessary token-level annotation in the document state
	for(std::set<std::string>::iterator
		it = requiredAnnotation.begin();
		it != requiredAnnotation.end();
		++it
	) {
		LOG(logger_, debug, "annotation " << *it);
		const MarkableLevel &annotationLevel = mmax->getMarkableLevel(*it);
		s->selectedLabels[*it].resize(nsents);
		BOOST_FOREACH(const Markable &m, annotationLevel) {
			uint snt = m.getSentence();
			const std::string &label = m.getAttribute("tag");
			const CoverageBitmap &cov = m.getCoverage();
			uint wordno = cov.find_first();
			s->AddLabel(snt, wordno, *it, label);
			LOG(logger_, debug, "Label of " << wordno << " in sent " << snt << " = " << label);
		}
	}

	for(uint i = 0; i < segs.size(); i++) {
		uint phrCount=0;
		BOOST_FOREACH(const AnchoredPhrasePair &app, segs[i]) {
			s->AddWord(
				i,
				phrCount,
				app,
				srcMatchConditions,
				trgMatchConditions,
				bilingual,
				skipUnaligned,
				mergeAligned
			);
			phrCount++;
		}
		// update selected word counter for this sentence
		s->nrSelected += s->GetNrOfWords(i);
	}

	StateType_ state(model_->BeginSentenceState()), out_state;
	const VocabularyType_ &vocab = model_->GetVocabulary();

	// TODO: should we only score one aligned word per source word?
	// (we could use srcNo in selectedWord - would that be sound?)
	// --> this would make the code much more complicated, especially down in score-update functions!
	Float score = 0;
	uint sentno = 999;
	for(uint i = 0; i != s->selectedWords.size(); ++i) {
		for(uint j = 0; j != s->selectedWords[i].size(); ++j) {
			// sentence-internal models: check if we start a new sentence
			if(withinSentence) {
				if(sentno != s->selectedWords[i][j].sentNo) {
					// TODO: do we need end of sentence?
					// score += model_->Score(state,vocab.EndSentence()),out_state);
					state = model_->BeginSentenceState();
					sentno = s->selectedWords[i][j].sentNo;
				}
			}
			score += model_->Score(state,vocab.Index(s->GetWord(i,j,bilingual)),out_state);

			if(logger_.loggable(debug)) {
				std::string word = s->GetWord(i,j,bilingual);
				LOG(logger_, debug, "add score for " << word
					<< " (index=" << vocab.Index(word)
					<< "):" << score
				);
			}
			state = out_state;
		}
	}

	s->currentScore = score;
	// LOG(logger_, debug, "initial doc score = " << s->currentScore);

	*sbegin = score;
	(*(sbegin + 1)) = Float(s->nrSelected);
	return s;
}


template<class M>
void GappyLanguageModel<M>::computeSentenceScores(
	const DocumentState &doc,
	uint sentno,
	Scores::iterator sbegin
) const {
	*sbegin = Float(0);
	(*(sbegin + 1)) = Float(0);
}

// TODO: do something smart to estimateScoreUpdate and move the code below to updateScore
// TODO: avoid recomputing scores if nothing actually changes (in the list of selected words)

template<class M>
FeatureFunction::StateModifications
*GappyLanguageModel<M>::estimateScoreUpdate(
	const DocumentState &doc,
	const SearchStep &step,
	const State *state,
	Scores::const_iterator psbegin,
	Scores::iterator sbegin
) const {
	const GappyLanguageModelState *prevstate =
		dynamic_cast<const GappyLanguageModelState *>(state);
	GappyLanguageModelState *s = prevstate->clone();

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
	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	std::vector<SearchStep::Modification>::const_iterator it = mods.begin();
	while(it != mods.end()) {
		PhraseSegmentation::const_iterator from_it = it->from_it;
		PhraseSegmentation::const_iterator to_it = it->to_it;

		const PhraseSegmentation &current = doc.getPhraseSegmentation(it->sentno);
		phrNo = std::distance(current.begin(), from_it);

		// reset selected word counter for this sentence
		s->nrSelected -= s->GetNrOfWords(it->sentno);

		if(firstSent || (sentNo != it->sentno)) {
			sentNo = it->sentno;
			firstSent = false;

			// LOG(logger_, debug, "word list before: " << s->GetWords(sentNo));
			phrNoDiff = 0;

			// start from scratch:
			// copy words from all phrases before the current modification
			s->ClearWords(sentNo);
			s->CopyWords(*prevstate,sentNo,0,phrNo,phrNoDiff);

			// get the LM history before the modification (initialize LM states)
			std::vector<std::string> history;
			s->GetHistory(history,sentNo,lmOrder-1,withinSentence,bilingual);

			if(history.size() < lmOrder-1) {
				StateType_ begin_state(model_->BeginSentenceState());
				add_state = begin_state;
			} else {
				StateType_ begin_state(model_->NullContextState());
				add_state = begin_state;
			}

			while(!history.empty()) {
				model_->Score(add_state,vocab.Index(history.back()),out_state);
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
		for(PhraseSegmentation::const_iterator pit=from_it; pit!=to_it; pit++) {
			uint phr = std::distance(current.begin(), pit);

			if(prevstate->selectedWords[sentNo].size() > 0) {
				for(uint i=0; i<prevstate->selectedWords[sentNo].size(); i++) {
					if(prevstate->selectedWords[sentNo][i].phrNo > phr)
						break;
					if(prevstate->selectedWords[sentNo][i].phrNo == phr) {
						Float score = model_->Score(
							remove_state,
							vocab.Index(prevstate->GetWord(sentNo,i,bilingual)),
							out_state
						);
						s->currentScore -= score;
						remove_state = out_state;
						LOG(logger_, debug,
							"(a) remove score for "
							<< prevstate->GetWord(sentNo,i,bilingual)
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
			uint added = s->AddWord(
				sentNo,
				newPhrNo,
				app,
				srcMatchConditions,
				trgMatchConditions,
				bilingual,
				skipUnaligned,
				mergeAligned
			);
			for(uint
				j = s->selectedWords[sentNo].size() - added;
				j < s->selectedWords[sentNo].size();
				++j
			) {
				Float score = model_->Score(
					add_state,
					vocab.Index(s->GetWord(sentNo,j,bilingual)),
					out_state
				);
				s->currentScore += score;
				add_state = out_state;
				LOG(logger_, debug, "(b) add score for "
				<< s->GetWord(sentNo,j,bilingual) << " (" << score << ")");
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

		if(it != mods.end()) {
			const PhraseSegmentation &next =
				doc.getPhraseSegmentation(it->sentno);
			nextModSentNo = it->sentno;
			nextModFrom = std::distance(next.begin(), it->from_it);
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
		// LOG(logger_, debug, " word list after: " << s->GetWords(currentSentNo));

		// run through more context words as needed by LM until next modification point or end of document

		std::vector<std::string> future;
		s->GetFuture(
			future,
			sentNo,
			wordIdx,
			lmOrder-1,
			nextModSentNo,
			nextModFrom,
			withinSentence,
			bilingual
		);

		for (std::vector<std::string>::const_iterator
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

			if(logger_.loggable(debug)) {
				if(newScore != oldScore) {
					LOG(logger_, debug,
						"(c) change score for " <<	*it
						<< " (old: " << oldScore
						<< ", new: " << newScore << ")"
					);
				}
			}
		}

		// TODO: Do we need end-of-sentence?
		/*
		if( future.size() < lmOrder-1) {
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

		// update selected word counter for this sentence
		s->nrSelected += s->GetNrOfWords(sentNo);
	}

	////////////////////////////////////////////////////////
	// DEBUG: compare with computing the score from scratch
	////////////////////////////////////////////////////////
	if(logger_.loggable(debug)) {
		StateType_ in_state(model_->BeginSentenceState()), out_state;
		const VocabularyType_ &vocab = model_->GetVocabulary();

		Float score=0;
		uint sentno = 999;
		for(uint i = 0; i < s->selectedWords.size(); ++i) {
			for(uint j = 0; j < s->selectedWords[i].size(); ++j) {
				if(withinSentence) {
					if (sentno != s->selectedWords[i][j].sentNo) {
						// TODO: do we need end of sentence?
						// score += model_->Score(state,vocab.EndSentence()),out_state);
						in_state = model_->BeginSentenceState();
						sentno = s->selectedWords[i][j].sentNo;
					}
				}
				score += model_->Score(
					in_state,
					vocab.Index(s->GetWord(i,j,bilingual)),
					out_state
				);
				in_state = out_state;
			}
		}
		if(score < s->currentScore-0.001 || score > s->currentScore+0.001) {
			LOG(logger_, debug, "score difference: " << s->currentScore << " != " << score);
		}
	}
	////////////////////////////////////////////////////////

	if(logger_.loggable(debug)) {
		if(prevstate->currentScore != s->currentScore) {
			if(prevstate->currentScore < s->currentScore) {
				LOG(logger_, debug,
					"higher LM score " << s->currentScore
					<< " (" << prevstate->currentScore << ")"
				);
			} else {
				LOG(logger_, debug,
					"lower LM score " << s->currentScore
					<< " (" << prevstate->currentScore << ")"
				);
			}
		}
	}

	*sbegin = s->score();
	(*(sbegin + 1)) = Float(s->nrSelected);
	return s;
}


template<class M>
FeatureFunction::StateModifications
*GappyLanguageModel<M>::updateScore(
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
*GappyLanguageModel<M>::applyStateModifications(
	FeatureFunction::State *oldState,
	FeatureFunction::StateModifications *modif
) const {
	GappyLanguageModelState *os = dynamic_cast<GappyLanguageModelState *>(oldState);
	GappyLanguageModelState *ms = dynamic_cast<GappyLanguageModelState *>(modif);

	os->currentScore = ms->currentScore;
	os->selectedWords.swap(ms->selectedWords);
	return oldState;
}
