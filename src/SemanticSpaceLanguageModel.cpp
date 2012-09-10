/*
 *  SemanticSpaceLanguageModel.cpp
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
#include "SemanticSpace.h"
#include "SemanticSpaceLanguageModel.h"
#include "PhrasePair.h"
#include "PiecewiseIterator.h"
#include "SearchStep.h"
#include "Stemmer.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>

#include <boost/foreach.hpp>
#include <boost/numeric/ublas/matrix_expression.hpp>
#include <boost/numeric/ublas/symmetric.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/unordered_map.hpp>

namespace ublas = boost::numeric::ublas;

template<class Vector>
std::string vec_to_string(const Vector &x) {
	std::ostringstream os;
	for(uint i = 0; i < x.size(); i++) {
		if(i > 0)
			os << ' ';
		os << x[i];
	}
	return os.str();
}

struct VectorScorer {
	virtual ~VectorScorer() {}
	virtual Float score(const SemanticSpace::WordVector &w, const SemanticSpace::DenseVector &h) const = 0;
};

class MultivariateNormal : public VectorScorer {
private:
	Float logNormalisationFactor_;
	Float logNormaliseTo1_; // FIXME: This can't be right, can it?
	ublas::symmetric_matrix<Float, ublas::lower> inverseCovarianceMatrix_;

public:
	MultivariateNormal(uint ndims, const std::string &file);

	virtual Float score(const SemanticSpace::WordVector &w, const SemanticSpace::DenseVector &h) const;

	template<class Vector>
	Float logpdf(Vector x) const {
		Float lscore = logNormalisationFactor_ -
			Float(.5) * ublas::inner_prod(x, ublas::prod(inverseCovarianceMatrix_, x))
			- logNormaliseTo1_;
		//std::cerr << lscore << "\t[" << vec_to_string(x) << "]" << std::endl;
		return lscore;
	}
};

class CosineSimilarity : public VectorScorer {
public:
	virtual Float score(const SemanticSpace::WordVector &w, const SemanticSpace::DenseVector &h) const;
};

class CosineProbabilityHistogram : public VectorScorer {
private:
	std::vector<Float> histogram_;

public:
	CosineProbabilityHistogram(const std::string &histfile);

	virtual Float score(const SemanticSpace::WordVector &w, const SemanticSpace::DenseVector &h) const;
};

class VectorCountModel {
private:
	std::vector<Float> distribution_;
	Float size_;

public:
	VectorCountModel(const std::string &histfile);
	Float score(uint nvecs, uint inputlen) const;
};

typedef boost::shared_ptr<SemanticSpace::DenseVector> WordVectorPointer;
typedef std::list<WordVectorPointer> SemList;

class SemanticSpaceLanguageModel : public FeatureFunction {
	friend class SemanticSpaceLanguageModelFactory;
	friend struct SSLMDocumentState;
	friend struct SSLMDocumentModifications;
	friend struct SSLMModificationState;

private:
	typedef std::pair<Float,WordVectorPointer> ScoreVectorPair_;
	typedef boost::unordered_map<Word,Float> StopList_;

	struct WordState_ {
		Float score;
		SemList::iterator semlink;

		WordState_(Float pscore, const SemList::iterator &psemlink) :
			score(pscore), semlink(psemlink) {}

		WordState_(const SemList::iterator &psemlink) :
			score(std::numeric_limits<Float>::quiet_NaN()), semlink(psemlink) {}
	};

	typedef std::vector<WordState_> SentenceState_;

	mutable Logger logger_;

	bool lowercase_;
	Stemmer *stemmer_;
	SemanticSpace *sspace_;
	std::string sourcePrefix_;
	std::string targetPrefix_;

	VectorCountModel *vectorCountModel_;
	Float stopWordLogprob_;
	Float unknownWordLogprob_;
	Float contentWordLogprob_;
	StopList_ stoplist_;
	std::vector<Float> filter_;
	const VectorScorer *jumpDistribution_;
	bool bilingualLookup_;
	bool useBiLMFormat_;
	bool normaliseByLength_;

	SemList::iterator noSemLink_;

	SemanticSpaceLanguageModel(const Parameters &params);

	ScoreVectorPair_ lookupWord(const PhrasePair &pp, uint wp) const;
	template<class Iterator>
	Float scoreWord(Iterator semlink, Iterator sembegin) const;

public:
	virtual ~SemanticSpaceLanguageModel();

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

FeatureFunction *SemanticSpaceLanguageModelFactory::createSemanticSpaceLanguageModel(const Parameters &params) {
	return new SemanticSpaceLanguageModel(params);
}

struct SSLMDocumentState : public FeatureFunction::State {
	std::vector<SemanticSpaceLanguageModel::SentenceState_> wordcache;
	SemList semlist;

	uint targetWordCount;
	uint vectorCount;
	Float vectorCountScore;

	virtual FeatureFunction::State *clone() const {
		return new SSLMDocumentState(*this);
	}
};

struct SSLMModificationState {
	SemanticSpaceLanguageModel::SentenceState_ wordcache;

	uint old_from_word;
	uint old_to_word;

	SemList::const_iterator oldsem_start;
	SemList::const_iterator oldsem_end;
};

struct SemListModification {
	SemList::const_iterator from;
	SemList::const_iterator to;
	SemList replacement;
};

struct SSLMDocumentModifications : public FeatureFunction::StateModifications {
	std::vector<std::pair<uint,SemanticSpaceLanguageModel::SentenceState_> > wordcacheMods;
	std::vector<SemListModification> semlistMods;
	uint targetWordCount;
	uint vectorCount;
	Float vectorCountScore;
};

SemanticSpaceLanguageModel::SemanticSpaceLanguageModel(const Parameters &params) :
		logger_(logkw::channel = "SemanticSpaceLanguageModel") {
	sspace_ = SemanticSpace::load(params.get<std::string>("sspace-file"));

	std::string ftype = params.get<std::string>("filter-type", "moving-avg");
	if(ftype != "moving-avg") {
		BOOST_LOG_SEV(logger_, error) << "Unimplemented filter type: " << ftype;
		BOOST_THROW_EXCEPTION(ConfigurationException());
	}
	uint order = params.get<uint>("moving-avg:order");
	if(order == 0) {
		BOOST_LOG_SEV(logger_, error) << "Moving average order must be positive.";
		BOOST_THROW_EXCEPTION(ConfigurationException());
	}
	filter_.insert(filter_.end(), order, Float(1) / Float(order));

	std::string scorertype = params.get<std::string>("scorer-type", "mvn");
	if(scorertype == "mvn") {
		std::string mvnfile = params.get<std::string>("mvn-parameters");
		jumpDistribution_ = new MultivariateNormal(sspace_->getDimensionality(), mvnfile);
	} else if(scorertype == "cosprob")
		jumpDistribution_ = new CosineSimilarity();
	else if(scorertype == "coshisto") {
		std::string histfile = params.get<std::string>("coshisto:histogram-file");
		jumpDistribution_ = new CosineProbabilityHistogram(histfile);
	} else {
		BOOST_LOG_SEV(logger_, error) << "Unknown scorer type: " << scorertype;
		BOOST_THROW_EXCEPTION(ConfigurationException());
	}

	std::string stopfile = params.get<std::string>("stop-word-model");
	std::ifstream stopstr(stopfile.c_str());
	std::string line;
	while(getline(stopstr, line)) {
		std::istringstream ss(line);
		std::string word;
		Float prob;
		ss >> word >> prob;
		stoplist_.insert(std::make_pair(word, std::log(prob)));
	}

	std::string vcountfile = params.get<std::string>("vector-count-model", "");
	if(!vcountfile.empty())
		vectorCountModel_ = new VectorCountModel(vcountfile);
	else
		vectorCountModel_ = NULL;

	unknownWordLogprob_ = std::log(params.get<Float>("unknown-word-probability"));
	contentWordLogprob_ = std::log(params.get<Float>("content-word-probability", Float(1)));

	if(vectorCountModel_ == NULL)
		stopWordLogprob_ = std::log(params.get<Float>("stop-word-probability", Float(0)));
	else
		stopWordLogprob_ = std::numeric_limits<Float>::quiet_NaN();

	normaliseByLength_ = params.get<bool>("normalise-by-length", false);

	useBiLMFormat_ = params.get<bool>("bilm-format", false);

	lowercase_ = params.get<bool>("lowercase", false);

	std::string sstemmer = params.get<std::string>("stemmer", "");
	if(!sstemmer.empty()) {
		std::string encoding = params.get<std::string>("encoding", "UTF_8");
		stemmer_ = new Stemmer(sstemmer, encoding);
	} else
		stemmer_ = NULL;

	bilingualLookup_ = params.get<bool>("bilingual-lookup", false);
	if(bilingualLookup_) {
		if(useBiLMFormat_) {
			BOOST_LOG_SEV(logger_, error) << "bilm-format and bilingual-lookup are mutually exclusive.";
			BOOST_THROW_EXCEPTION(ConfigurationException());
		}
		if(stemmer_) {
			BOOST_LOG_SEV(logger_, error) << "Stemming not currently implemented for bilingual-lookup.";
			BOOST_THROW_EXCEPTION(ConfigurationException());
		}
		sourcePrefix_ = params.get<std::string>("source-prefix");
		targetPrefix_ = params.get<std::string>("target-prefix");
	}

	static SemList emptySemList;
	noSemLink_ = emptySemList.end();
}

SemanticSpaceLanguageModel::~SemanticSpaceLanguageModel() {
	delete jumpDistribution_;
	delete sspace_;
	if(stemmer_)
		delete stemmer_;
	if(vectorCountModel_)
		delete vectorCountModel_;
}

FeatureFunction::State *SemanticSpaceLanguageModel::initDocument(const DocumentState &doc, Scores::iterator sbegin) const {
	SSLMDocumentState *state = new SSLMDocumentState();
	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();
	state->wordcache.resize(segs.size());
	Float &s = *sbegin;
	state->vectorCount = 0;
	uint total_tgtwords = 0;
	for(uint i = 0; i < segs.size(); i++) {
		uint ntgtwords = countTargetWords(segs[i].begin(), segs[i].end());
		total_tgtwords += ntgtwords;
		BOOST_LOG_SEV(logger_, debug) << "Sentence " << i << ": " << ntgtwords << " target words.";
		state->wordcache[i].reserve(ntgtwords);
		BOOST_FOREACH(const AnchoredPhrasePair &app, segs[i]) {
			for(uint w = 0; w < app.second.get().getTargetPhrase().get().size(); w++) {
				ScoreVectorPair_ svp = lookupWord(app.second, w);
				if(svp.second == NULL) {
					if(vectorCountModel_ == NULL) {
						WordState_ ws(svp.first, noSemLink_);
						state->wordcache[i].push_back(ws);
						s += svp.first;
					} else {
						WordState_ ws(Float(0), noSemLink_);
						state->wordcache[i].push_back(ws);
					}
				} else {
					state->vectorCount++;
					SemList::iterator semit =
						state->semlist.insert(state->semlist.end(), svp.second);
					WordState_ ws(svp.first, semit);
					Float lscore = scoreWord(semit, state->semlist.begin());
					ws.score = lscore;
					state->wordcache[i].push_back(ws);
					s += lscore;
				}
			}
		}
		assert(state->wordcache[i].size() == ntgtwords);
	}

	if(vectorCountModel_ != NULL) {
		state->vectorCountScore = vectorCountModel_->score(state->vectorCount, doc.getInputWordCount());
		s += state->vectorCountScore;
	}

	state->targetWordCount = total_tgtwords;
	if(normaliseByLength_)
		s /= total_tgtwords;

	return state;
}

void SemanticSpaceLanguageModel::computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const {
	std::fill_n(sbegin, getNumberOfScores(), Float(0));
}

FeatureFunction::StateModifications *SemanticSpaceLanguageModel::estimateScoreUpdate(const DocumentState &doc,
		const SearchStep &step, const FeatureFunction::State *ffstate,
		Scores::const_iterator psbegin, Scores::iterator sbegin) const {
	std::fill_n(sbegin, getNumberOfScores(), Float(0));
	return NULL;
}

FeatureFunction::StateModifications *SemanticSpaceLanguageModel::updateScore(const DocumentState &doc,
		const SearchStep &step, const FeatureFunction::State *ffstate,
		StateModifications *estmods, Scores::const_iterator psbegin, Scores::iterator sbegin) const {
	BOOST_LOG_SEV(logger_, debug) << "SemanticSpaceLanguageModel::updateScore";
	const SSLMDocumentState &state = dynamic_cast<const SSLMDocumentState &>(*ffstate);
	Float &s = *sbegin;
	s = *psbegin;

	uint total_tgtwords = state.targetWordCount;
	if(normaliseByLength_)
		s *= total_tgtwords;

	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	std::vector<SSLMModificationState> modificationStates(mods.size());

	SSLMDocumentModifications *modif = new SSLMDocumentModifications();
	modif->semlistMods.resize(mods.size());

	BOOST_LOG_SEV(logger_, debug) << "Chain creation pass";
	for(uint i = 0; i < mods.size(); i++) {
		uint sentno = mods[i].sentno;
		PhraseSegmentation::const_iterator from_it = mods[i].from_it;
		PhraseSegmentation::const_iterator to_it = mods[i].to_it;
		const PhraseSegmentation &proposal = mods[i].proposal;

		const PhraseSegmentation &current = doc.getPhraseSegmentation(sentno);

		uint fromword = countTargetWords(current.begin(), from_it);
		uint toword = fromword + countTargetWords(from_it, to_it);

		// find out what to replace in the semantic vector list
		SemList::const_iterator oldsem_start;
		SemList::const_iterator oldsem_end;
		if(!state.semlist.empty()) {
			oldsem_start = oldsem_end = noSemLink_;
			// first look at the vectors in the replaced phrases
			for(uint j = fromword; j < toword; j++) {
				assert(j < state.wordcache[sentno].size());
				if(state.wordcache[sentno][j].semlink != noSemLink_) {
					if(oldsem_start == noSemLink_)
						oldsem_start = state.wordcache[sentno][j].semlink;
					oldsem_end = state.wordcache[sentno][j].semlink;
					++oldsem_end;
				}
			}
			// if there's no vector, scan the part after the replacement
			uint w = toword;
			for(uint j = sentno; oldsem_end == noSemLink_ && j < state.wordcache.size(); j++) {
				while(w < state.wordcache[j].size()) {
					SemList::const_iterator link = state.wordcache[j][w].semlink;
					if(link != noSemLink_) {
						oldsem_start = oldsem_end = link;
						break;
					}
					w++;
				}
				w = 0;
			}
			// if that fails, the part before the replacement
			w = fromword;
			uint j = sentno;
			for(;;) {
				while(w > 0) {
					w--;
					SemList::const_iterator link = state.wordcache[j][w].semlink;
					if(link != noSemLink_) {
						++link;
						oldsem_start = oldsem_end = link;
						break;
					}
				}
				if(oldsem_end != noSemLink_)
					break;
				assert(j > 0); // otherwise we should have found a vector already
				j--;
				w = state.wordcache[j].size();
			}
		} else
			oldsem_start = oldsem_end = state.semlist.end();

		modif->semlistMods[i].from = oldsem_start;
		modif->semlistMods[i].to = oldsem_end;
		SemList &newsemlist = modif->semlistMods[i].replacement;

		BOOST_FOREACH(const AnchoredPhrasePair &app, proposal) {
			for(uint w = 0; w < app.second.get().getTargetPhrase().get().size(); w++) {
				ScoreVectorPair_ svp = lookupWord(app.second, w);
				if(svp.second == NULL)
					modificationStates[i].wordcache.push_back(WordState_(svp.first, noSemLink_));
				else {
					SemList::iterator semit =
						newsemlist.insert(newsemlist.end(), svp.second);
					modificationStates[i].wordcache.push_back(WordState_(svp.first, semit));
				}
			}
		}

		modificationStates[i].old_from_word = fromword;
		modificationStates[i].old_to_word = toword;
		modificationStates[i].oldsem_start = oldsem_start;
		modificationStates[i].oldsem_end = oldsem_end;
	}

	BOOST_LOG_SEV(logger_, debug) << "Subtraction pass";
	modif->vectorCount = state.vectorCount;
	for(uint i = 0; i < mods.size(); i++) {
		SSLMModificationState modstate;

		uint sentno = mods[i].sentno;
		PhraseSegmentation::const_iterator from_it = mods[i].from_it;
		PhraseSegmentation::const_iterator to_it = mods[i].to_it;

		BOOST_LOG_SEV(logger_, debug) << "next modification, starting at target word " <<
			countTargetWords(doc.getPhraseSegmentation(sentno).begin(), from_it);

		for(uint j = modificationStates[i].old_from_word; j < modificationStates[i].old_to_word; j++) {
			BOOST_LOG_SEV(logger_, debug) << "(a) minus " << state.wordcache[sentno][j].score;
			s -= state.wordcache[sentno][j].score;
			total_tgtwords--;
			if(state.wordcache[sentno][j].semlink != noSemLink_)
				modif->vectorCount--;
		}
	}

	BOOST_LOG_SEV(logger_, debug) << "Addition pass";
	std::vector<SemListModification>::iterator semmodit = modif->semlistMods.begin();
	std::vector<SSLMModificationState>::iterator modstateit = modificationStates.begin();
	std::vector<SearchStep::Modification>::const_iterator it1 = mods.begin();

	typedef std::vector<SemList::const_iterator> PieceVector;
	typedef PiecewiseIterator<PieceVector::const_iterator> PieceIt;

	PieceVector pieces;
	pieces.reserve(2 + 4 * modif->semlistMods.size());
	pieces.push_back(state.semlist.begin());
	BOOST_FOREACH(const SemListModification &m, modif->semlistMods) {
		assert(m.from != noSemLink_);
		assert(m.to != noSemLink_);
		pieces.push_back(m.from);
		pieces.push_back(m.replacement.begin());
		pieces.push_back(m.replacement.end());
		pieces.push_back(m.to);
	}
	pieces.push_back(state.semlist.end());

	PieceIt modsemlist_begin(pieces.begin(), pieces.end());

	while(it1 != mods.end()) {
		uint sentno = it1->sentno;
		BOOST_LOG_SEV(logger_, debug) << "Starting in sentence " << sentno <<
			", which has " <<
			countTargetWords(doc.getPhraseSegmentation(sentno)) <<
			" target words.";
		std::vector<SearchStep::Modification>::const_iterator it2 = it1;
		while(++it2 != mods.end())
			if(it2->sentno != sentno)
				break;

		const PhraseSegmentation &current = doc.getPhraseSegmentation(sentno);

		if(modif->wordcacheMods.empty() || modif->wordcacheMods.back().first != sentno)
			modif->wordcacheMods.push_back(std::make_pair(sentno, SentenceState_()));

		SentenceState_ *sntstate = &modif->wordcacheMods.back().second;
		sntstate->reserve(state.wordcache[sentno].size()); // an approximation

		SentenceState_::const_iterator oldstate1 = state.wordcache[sentno].begin();
		PhraseSegmentation::const_iterator next_from_it = it1->from_it;
		SentenceState_::const_iterator oldstate2 = oldstate1 +
			countTargetWords(current.begin(), next_from_it);

		if(sntstate->empty())
			sntstate->insert(sntstate->end(), oldstate1, oldstate2);

		for(std::vector<SearchStep::Modification>::const_iterator modit = it1; modit != it2;
				++modit, ++modstateit, ++semmodit) {
			uint sentno = modit->sentno;
			PhraseSegmentation::const_iterator from_it = modit->from_it;
			PhraseSegmentation::const_iterator to_it = modit->to_it;

			BOOST_LOG_SEV(logger_, debug) << "next modification, starting at target word " <<
				countTargetWords(doc.getPhraseSegmentation(sentno).begin(), from_it);

			uint next_sentence;
			PhraseSegmentation::const_iterator local_next_from_it;
			std::vector<SearchStep::Modification>::const_iterator nextmod = modit + 1;
			if(nextmod != mods.end()) {
				next_sentence = nextmod->sentno;
				next_from_it = (modit + 1)->from_it;
				if(next_sentence == sentno)
					local_next_from_it = next_from_it;
				else
					local_next_from_it = current.end();
			} else {
				next_sentence = std::numeric_limits<uint>::max();
				local_next_from_it = current.end();
				next_from_it = doc.getPhraseSegmentations().back().end();
			}

			PieceVector::const_iterator curpiece =
				std::find(pieces.begin(), pieces.end(), semmodit->replacement.begin());
			BOOST_FOREACH(const WordState_ &ws, modstateit->wordcache) {
				total_tgtwords++;
				if(ws.semlink == noSemLink_) {
					if(vectorCountModel_ != NULL) {
						sntstate->push_back(ws);
						BOOST_LOG_SEV(logger_, debug) << "(a1) plus " << ws.score;
						s += ws.score;
					} else {
						sntstate->push_back(WordState_(Float(0), noSemLink_));
						BOOST_LOG_SEV(logger_, debug) << "(a1) skipping stop word";
					}
				} else {
					PieceIt modsemlist_current(pieces.begin(), curpiece, pieces.end(), ws.semlink);
					Float lscore = scoreWord(modsemlist_current, modsemlist_begin);
					sntstate->push_back(ws);
					sntstate->back().score = lscore;
					BOOST_LOG_SEV(logger_, debug) << "(a2) plus " << lscore;
					s += lscore;
					modif->vectorCount++;
				}
			}
			curpiece += 2;

			oldstate1 = oldstate2 + countTargetWords(from_it, to_it);
			oldstate2 = oldstate1 + countTargetWords(to_it, local_next_from_it);

			SemList::const_iterator last_semlink = state.semlist.end();
			--last_semlink;
			uint i = 0;
			while(i < filter_.size()) {
				if(oldstate1 == oldstate2) {
					if(sentno == next_sentence)
						break;

					sentno++;
					if(sentno == state.wordcache.size())
						break;

					BOOST_LOG_SEV(logger_, debug) << "now in sentence " << sentno <<
						", which has " <<
						countTargetWords(doc.getPhraseSegmentation(sentno)) <<
						" target words.";
					modif->wordcacheMods.push_back(std::make_pair(sentno, SentenceState_()));
					sntstate = &modif->wordcacheMods.back().second;
					oldstate1 = state.wordcache[sentno].begin();
					if(sentno == next_sentence)
						oldstate2 = oldstate1 +
							countTargetWords(doc.getPhraseSegmentation(sentno).begin(), next_from_it);
					else
						oldstate2 = state.wordcache[sentno].end();
				}

				if(oldstate1->semlink == noSemLink_) {
					BOOST_LOG_SEV(logger_, debug) << "copying non-content element";
					sntstate->push_back(*oldstate1);
				} else {
					PieceIt modsemlist_current(pieces.begin(), curpiece, pieces.end(),
									oldstate1->semlink);
					if(i < filter_.size()) {
						WordState_ ws(oldstate1->semlink);
						Float lscore = scoreWord(modsemlist_current, modsemlist_begin);
						ws.score = lscore;
						sntstate->push_back(ws);
						BOOST_LOG_SEV(logger_, debug) << "(b) minus " << oldstate1->score;
						BOOST_LOG_SEV(logger_, debug) << "(b) plus " << lscore;
						s += lscore - oldstate1->score;
					}
					++i;
				}
				if(oldstate1->semlink == last_semlink) {
					++oldstate1;
					break;
				} else
					++oldstate1;
			}
			sntstate->insert(sntstate->end(), oldstate1, oldstate2);

			it1 = it2;
		}
	}

	if(modif->vectorCount != state.vectorCount && vectorCountModel_ != NULL) {
		s -= state.vectorCountScore;
		modif->vectorCountScore = vectorCountModel_->score(modif->vectorCount, doc.getInputWordCount());
		s += modif->vectorCountScore;
	} else
		modif->vectorCountScore = state.vectorCountScore;

	modif->targetWordCount = total_tgtwords;
	if(normaliseByLength_)
		s /= total_tgtwords;

	return modif;
}

FeatureFunction::State *SemanticSpaceLanguageModel::applyStateModifications(FeatureFunction::State *oldState,
		FeatureFunction::StateModifications *modif) const {
	SSLMDocumentState &state = dynamic_cast<SSLMDocumentState &>(*oldState);
	SSLMDocumentModifications *mod = dynamic_cast<SSLMDocumentModifications *>(modif);
	for(std::vector<std::pair<uint,SentenceState_> >::iterator it = mod->wordcacheMods.begin();
			it != mod->wordcacheMods.end(); ++it)
		state.wordcache[it->first].swap(it->second);
	BOOST_FOREACH(SemListModification &m, mod->semlistMods) {
		// transform const_iterators into iterators
		SemList::iterator from = state.semlist.begin();
		SemList::iterator to = state.semlist.begin();
		std::advance(from, std::distance<SemList::const_iterator>(state.semlist.begin(), m.from));
		std::advance(to, std::distance<SemList::const_iterator>(state.semlist.begin(), m.to));

		state.semlist.erase(from, to);
		state.semlist.splice(to, m.replacement);
	}
	state.vectorCount = mod->vectorCount;
	state.vectorCountScore = mod->vectorCountScore;
	state.targetWordCount  = mod->targetWordCount;
	return oldState;
}

SemanticSpaceLanguageModel::ScoreVectorPair_ SemanticSpaceLanguageModel::lookupWord(
		const PhrasePair &pp, uint wp) const {
	std::string word = pp.get().getTargetPhrase().get()[wp];

	if(lowercase_ || stemmer_)
		boost::to_lower(word);

	StopList_::const_iterator it = stoplist_.find(word);
	WordVectorPointer vec;

	Float lprob;
	if(it != stoplist_.end()) {
		lprob = stopWordLogprob_ + it->second;
		BOOST_LOG_SEV(logger_, debug) << "Stop word: " << word << " (lp = " << lprob << ")";
	} else if(bilingualLookup_) {
		vec = boost::make_shared<SemanticSpace::DenseVector>(sspace_->getDimensionality());
		vec->clear();

		const SemanticSpace::WordVector *ssvec;
		uint srccnt = 0;
		const WordAlignment &wa = pp.get().getWordAlignment();
		for(WordAlignment::const_iterator wit = wa.begin_for_target(wp);
				wit != wa.end_for_target(wp); ++wit) {
			std::string s = pp.get().getSourcePhrase().get()[*wit];
			if(lowercase_)
				boost::to_lower(s);
			ssvec = sspace_->lookup(sourcePrefix_ + s);
			if(ssvec != NULL) {
				*vec += *ssvec;
				srccnt++;
			}
		}
		if(srccnt > 1)
			*vec /= srccnt;

		ssvec = sspace_->lookup(targetPrefix_ + word);
		if(ssvec != NULL) {
			*vec += *ssvec;
			if(srccnt > 0)
				*vec *= Float(.5);
		}

		if(ssvec != NULL || srccnt > 0)
			lprob = std::numeric_limits<Float>::quiet_NaN();
		else {
			lprob = unknownWordLogprob_;
			BOOST_LOG_SEV(logger_, debug) << "No vectors found for " << word << " (lp = " << lprob << ")";
		}
	} else {
		if(stemmer_)
			word = (*stemmer_)(word);

		if(useBiLMFormat_) {
			std::ostringstream os;
			os << word;
			const WordAlignment &wa = pp.get().getWordAlignment();
			std::vector<Word> srcww;
			for(WordAlignment::const_iterator wit = wa.begin_for_target(wp);
					wit != wa.end_for_target(wp); ++wit)
				srcww.push_back(pp.get().getSourcePhrase().get()[*wit]);
			if(lowercase_) {
				BOOST_FOREACH(std::string &s, srcww)
					boost::to_lower(s);
			}
			std::sort(srcww.begin(), srcww.end());
			std::vector<Word>::iterator eit = std::unique(srcww.begin(), srcww.end());
			for(std::vector<Word>::iterator i = srcww.begin(); i != eit; ++i)
				os << "__" << *i;
			word = os.str();
		}

		BOOST_LOG_SEV(logger_, debug) << "Looking up " << word;
		const SemanticSpace::WordVector *ssvec = NULL;
		ssvec = sspace_->lookup(word);
		if(ssvec == NULL) {
			lprob = unknownWordLogprob_;
			BOOST_LOG_SEV(logger_, debug) << "Unknown word: " << word << " (lp = " << lprob << ")";
		} else {
			lprob = std::numeric_limits<Float>::quiet_NaN();
			vec = boost::make_shared<SemanticSpace::DenseVector>(*ssvec);
		}
	}

	return ScoreVectorPair_(lprob, vec);
}

template<class Iterator>
Float SemanticSpaceLanguageModel::scoreWord(Iterator semlink, Iterator sembegin) const {
	if(semlink == sembegin) {
		return contentWordLogprob_; // the initial jump is free
	} else {
		SemanticSpace::DenseVector h(sspace_->getDimensionality());
		h.clear();
		Iterator semit = semlink;
		for(uint i = 0; i < filter_.size(); i++) {
			--semit;
			h += filter_[i] * **semit;
			if(semit == sembegin)
				break;
		}
		return contentWordLogprob_ + jumpDistribution_->score(**semlink, h);
	}
}

Float CosineSimilarity::score(const SemanticSpace::WordVector &w, const SemanticSpace::DenseVector &h) const {
	return std::log(ublas::inner_prod(w, h)) - (std::log(ublas::norm_2(w)) + std::log(ublas::norm_2(h)));
}

CosineProbabilityHistogram::CosineProbabilityHistogram(const std::string &histfile) {
	std::ifstream istr(histfile.c_str());
	Float p;
	while(istr >> p)
		histogram_.push_back(p);
	istr.close();
	std::sort(histogram_.begin(), histogram_.end());
}

Float CosineProbabilityHistogram::score(const SemanticSpace::WordVector &w, const SemanticSpace::DenseVector &h) const {
	Float sim = ublas::inner_prod(w, h) / (ublas::norm_2(w) * ublas::norm_2(h));
	Float pos = Float(std::lower_bound(histogram_.begin(), histogram_.end(), sim) - histogram_.begin());
	return std::log(pos / Float(histogram_.size()));
}

MultivariateNormal::MultivariateNormal(uint ndims, const std::string &file)
		: inverseCovarianceMatrix_(ndims) {
	std::ifstream str(file.c_str());
	std::string line;

	if(!getline(str, line))
		BOOST_THROW_EXCEPTION(FileFormatException() << err_info::Filename(file));
	std::istringstream is(line);
	is >> logNormalisationFactor_;

	for(uint i = 0; i < ndims; i++) {
		if(!getline(str, line))
			BOOST_THROW_EXCEPTION(FileFormatException() << err_info::Filename(file));

		is.clear();
		is.str(line);
		for(uint j = 0; j <= i; j++) {
			Float f;
			is >> f;
			inverseCovarianceMatrix_(i, j) = f;
		}
	}

	SemanticSpace::DenseVector zero(ndims);
	zero.clear();
	logNormaliseTo1_ = Float(0); // must be initialised when calling logpdf
	logNormaliseTo1_ = logpdf(zero);
}

Float MultivariateNormal::score(const SemanticSpace::WordVector &w, const SemanticSpace::DenseVector &h) const {
	return logpdf(w - h);
}

VectorCountModel::VectorCountModel(const std::string &histfile) {
	std::ifstream istr(histfile.c_str());
	distribution_.insert(distribution_.end(), std::istream_iterator<Float>(istr), std::istream_iterator<Float>());
	istr.close();
	std::sort(distribution_.begin(), distribution_.end());
	size_ = Float(distribution_.size());
}

Float VectorCountModel::score(uint nvecs, uint inputlen) const {
	Float ratio = Float(nvecs) / Float(inputlen);
	Float pos = Float(std::lower_bound(distribution_.begin(), distribution_.end(), ratio) - distribution_.begin());
	if(pos + pos > size_)
		pos = size_ - pos;
	pos = std::max(Float(.5), pos);
	return std::log(Float(2) * pos / size_);
}
