/*
 *  FeatureFunction.cpp
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
//#include "LexicalChainCohesionModel.h"
#include "NgramModel.h"
#include "PhraseTable.h"
//#include "PronominalAnaphoraModel.h"
//#include "ReverseNgramModel.h"
#include "SearchStep.h"
#include "SemanticSpaceLanguageModel.h"
#include "SentenceParityModel.h"
#include "SentenceInitialCharModel.h"
#include "SentenceFinalCharModel.h"
#include "SentenceFinalWordModel.h"
#include "FinalWordRhymeModel.h"
#include "InitialCharModel.h"
//#include "WordSpaceCohesionModel.h"
#include "ConsistencyQModelPhrase.h"
#include "ConsistencyQModelWord.h"
#include "OvixModel.h"
#include "TypeTokenRateModel.h"
#include "BleuModel.h"
//#include "PhraseDistortionModel.h"

#include <algorithm>
#include <limits>
#include <vector>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

template<class CountingFunction>
class CountingFeatureFunction : public FeatureFunction {
private:
	CountingFunction countingFunction_;

public:
	CountingFeatureFunction(CountingFunction countingFunction)
		: countingFunction_(countingFunction) {}

	static FeatureFunction *createWordPenaltyFeatureFunction();
	static FeatureFunction *createOOVPenaltyFeatureFunction();

	virtual State *initDocument(const DocumentState &doc, Scores::iterator sbegin) const;
	virtual StateModifications *estimateScoreUpdate(const DocumentState &doc, const SearchStep &step, const State *state,
		Scores::const_iterator psbegin, Scores::iterator sbegin) const;
	virtual StateModifications *updateScore(const DocumentState &doc, const SearchStep &step, const State *state,
		StateModifications *estmods, Scores::const_iterator psbegin, Scores::iterator estbegin) const;
	virtual uint getNumberOfScores() const {
		return 1;
	}

	virtual void computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const;
};

class GeometricDistortionModel : public FeatureFunction {
private:
	template<class Operator>
	void scoreSegment(PhraseSegmentation::const_iterator from, PhraseSegmentation::const_iterator to,
		Scores::iterator sbegin, Operator op) const;

	Float computeDistortionDistance(const CoverageBitmap &m1, const CoverageBitmap &m2) const;
	
	Float distortionLimit_;

public:
	GeometricDistortionModel(const Parameters &params);

	virtual State *initDocument(const DocumentState &doc, Scores::iterator sbegin) const;
	virtual StateModifications *estimateScoreUpdate(const DocumentState &doc, const SearchStep &step, const State *state,
		Scores::const_iterator psbegin, Scores::iterator sbegin) const;
	virtual StateModifications *updateScore(const DocumentState &doc, const SearchStep &step, const State *state,
		StateModifications *estmods, Scores::const_iterator, Scores::iterator estbegin) const;
	
	virtual uint getNumberOfScores() const {
		return distortionLimit_ == -1 ? 1 : 2;
	}

	virtual void computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const;
};

class SentenceLengthModel : public FeatureFunction {
private:
	Float identicalLogprob_;
	Float shortLogprob_;
	Float longLogprob_;

	Float shortP_;
	Float longP_;

	Float score(Float inputlen, Float outputlen) const;

public:
	SentenceLengthModel(const Parameters &params);

	virtual State *initDocument(const DocumentState &doc, Scores::iterator sbegin) const;
	virtual StateModifications *estimateScoreUpdate(const DocumentState &doc, const SearchStep &step, const State *state,
		Scores::const_iterator psbegin, Scores::iterator sbegin) const;
	virtual StateModifications *updateScore(const DocumentState &doc, const SearchStep &step, const State *state,
		StateModifications *estmods, Scores::const_iterator psbegin, Scores::iterator estbegin) const;
	virtual uint getNumberOfScores() const {
		return 1;
	}

	virtual void computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const;
};

struct WordPenaltyCounter : public std::unary_function<const AnchoredPhrasePair &,Float> {
	Float operator()(const AnchoredPhrasePair &ppair) const {
		return -Float(ppair.second.get().getTargetPhrase().get().size());
	};
};

struct OOVPenaltyCounter : public std::unary_function<const AnchoredPhrasePair &,Float> {
	Float operator()(const AnchoredPhrasePair &ppair) const {
		return ppair.second.get().isOOV() ? Float(-1) : Float(0);
	}
};

struct LongWordCounter : public std::unary_function<const AnchoredPhrasePair &,Float> {

	LongWordCounter(const Parameters &params) {
		try {
			longLimit_ = params.get<uint>("long-word-length-limit");
		}
		catch(ParameterNotFoundException()) {
			longLimit_ = 7; //default value (from LIX)
		}
	}

	Float operator()(const AnchoredPhrasePair &ppair) const {
		int numLong = 0;
		BOOST_FOREACH(const Word &w, ppair.second.get().getTargetPhrase().get()) {
			if (w.size() >= longLimit_) {
				numLong++;
			}
		}
		return Float(-numLong);
	}

private:
	uint longLimit_;
};


template<class F>
CountingFeatureFunction<F> *createCountingFeatureFunction(F countingFunction) {
	return new CountingFeatureFunction<F>(countingFunction);
}

template<class F>
FeatureFunction::State *CountingFeatureFunction<F>::initDocument(const DocumentState &doc,
		Scores::iterator sbegin) const {
	using namespace boost::lambda;
	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();
	Float &s = *sbegin;
	s = Float(0);
	for(uint i = 0; i < segs.size(); i++)
		std::for_each(segs[i].begin(), segs[i].end(), s += bind(countingFunction_, _1));
	return NULL;
}

template<class F>
void CountingFeatureFunction<F>::computeSentenceScores(const DocumentState &doc, uint sentno,
		Scores::iterator sbegin) const {
	using namespace boost::lambda;
	Float &s = *sbegin;
	s = Float(0);
	const PhraseSegmentation &snt = doc.getPhraseSegmentation(sentno);
	std::for_each(snt.begin(), snt.end(), s += bind(countingFunction_, _1));
}

template<class F>
FeatureFunction::StateModifications *CountingFeatureFunction<F>::estimateScoreUpdate(const DocumentState &doc,
		const SearchStep &step, const State *state, Scores::const_iterator psbegin,
		Scores::iterator sbegin) const {
	using namespace boost::lambda;
	Float &s = *sbegin;
	s = *psbegin;
	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	for(std::vector<SearchStep::Modification>::const_iterator it = mods.begin(); it != mods.end(); ++it) {
		std::for_each(it->from_it, it->to_it, s -= bind(countingFunction_, _1));
		std::for_each(it->proposal.begin(), it->proposal.end(), s += bind(countingFunction_, _1));
	}
	return NULL;
}

template<class F>
FeatureFunction::StateModifications *CountingFeatureFunction<F>::updateScore(const DocumentState &doc,
		const SearchStep &step, const State *state, FeatureFunction::StateModifications *estmods,
		Scores::const_iterator psbegin, Scores::iterator sbegin) const {
	return estmods;
}

GeometricDistortionModel::GeometricDistortionModel(const Parameters &params) {
	distortionLimit_ = params.get<Float>("distortion-limit", std::numeric_limits<Float>::infinity());
}

inline Float GeometricDistortionModel::computeDistortionDistance(const CoverageBitmap &m1, const CoverageBitmap &m2) const {
	uint fp1 = m1.find_first();
	CoverageBitmap::size_type fp1n;
	while((fp1n = m1.find_next(fp1)) != CoverageBitmap::npos)
		fp1 = fp1n;
	uint fp2 = m2.find_first();
	return static_cast<Float>(-abs(fp2 - (fp1 + 1)));
}

template<class Operator>
inline void GeometricDistortionModel::scoreSegment(PhraseSegmentation::const_iterator from,
		PhraseSegmentation::const_iterator to, Scores::iterator sbegin, Operator op) const {
	if(from == to)
		return;

	PhraseSegmentation::const_iterator it1 = from;
	PhraseSegmentation::const_iterator it2 = from;
	++it2;
	while(it2 != to) {
		Float dist = computeDistortionDistance(it1->first, it2->first);
		*sbegin = op(*sbegin, dist);
		if(-dist > distortionLimit_)
			(*(sbegin + 1)) = op(*(sbegin + 1), Float(-1));
		it1 = it2;
		++it2;
	}
}

FeatureFunction::State *GeometricDistortionModel::initDocument(const DocumentState &doc, Scores::iterator sbegin) const {
	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();
	std::fill_n(sbegin, getNumberOfScores(), .0);
	for(uint i = 0; i < segs.size(); i++)
		scoreSegment(segs[i].begin(), segs[i].end(), sbegin, std::plus<Float>());
	return NULL;
}

void GeometricDistortionModel::computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const {
	const PhraseSegmentation &seg = doc.getPhraseSegmentation(sentno);
	std::fill_n(sbegin, getNumberOfScores(), Float(0));
	scoreSegment(seg.begin(), seg.end(), sbegin, std::plus<Float>());
}


FeatureFunction::StateModifications *GeometricDistortionModel::estimateScoreUpdate(const DocumentState &doc, const SearchStep &step, const State *state,
		Scores::const_iterator psbegin, Scores::iterator sbegin) const {
	
	std::copy(psbegin, psbegin + getNumberOfScores(), sbegin);
	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	for(std::vector<SearchStep::Modification>::const_iterator it = mods.begin(); it != mods.end(); ++it) {
		uint sentno = it->sentno;
		PhraseSegmentation::const_iterator from_it = it->from_it;
		PhraseSegmentation::const_iterator to_it = it->to_it;
		const PhraseSegmentation &proposal = it->proposal;
		const PhraseSegmentation &oldseg = doc.getPhraseSegmentation(sentno);

		if(!proposal.empty()) {
			if(from_it != oldseg.begin()) {
				--from_it;
				Float dist = computeDistortionDistance(from_it->first, proposal.front().first);
				*sbegin += dist;
				if(-dist > distortionLimit_)
					(*(sbegin + 1))--;
			}

			if(to_it != oldseg.end()) {
				Float dist = computeDistortionDistance(proposal.back().first, to_it->first);
				*sbegin += dist;
				if(-dist > distortionLimit_)
					(*(sbegin + 1))--;
				++to_it;
			}
		} else {
			if(from_it != oldseg.begin())
				--from_it;
			if(from_it != oldseg.begin() && to_it != oldseg.end()) {
				Float dist = computeDistortionDistance(from_it->first, to_it->first);
				*sbegin += dist;
				if(-dist > distortionLimit_)
					(*(sbegin + 1))--;
			}
			if(to_it != oldseg.end())
				++to_it;
		}
		
		scoreSegment(from_it, to_it, sbegin, std::minus<Float>());
		scoreSegment(proposal.begin(), proposal.end(), sbegin, std::plus<Float>());
	}
	return NULL;
}

FeatureFunction::StateModifications *GeometricDistortionModel::updateScore(const DocumentState &doc, const SearchStep &step, const State *state,
		FeatureFunction::StateModifications *estmods, Scores::const_iterator psbegin, Scores::iterator estbegin) const {
	return estmods;
}

SentenceLengthModel::SentenceLengthModel(const Parameters &params) {
	Float identicalProb = params.get<Float>("prob-identical");
	Float shortProb = params.get<Float>("prob-short");

	identicalLogprob_ = std::log(identicalProb);
	shortLogprob_ = std::log(shortProb);
	longLogprob_ = std::log(Float(1) - identicalProb - shortProb);

	shortP_ = params.get<Float>("decay-short");
	longP_ = params.get<Float>("decay-long");
}

FeatureFunction::State *SentenceLengthModel::initDocument(const DocumentState &doc,
		Scores::iterator sbegin) const {
	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();
	Float &s = *sbegin;
	s = Float(0);
	for(uint i = 0; i < segs.size(); i++)
		s += score(doc.getInputSentenceLength(i), countTargetWords(segs[i]));
	return NULL;
}

FeatureFunction::StateModifications *SentenceLengthModel::estimateScoreUpdate(const DocumentState &doc,
		const SearchStep &step, const State *state, Scores::const_iterator psbegin,
		Scores::iterator sbegin) const {
	using namespace boost::lambda;
	Float &s = *sbegin;
	s = *psbegin;
	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	for(std::vector<SearchStep::Modification>::const_iterator it = mods.begin(); it != mods.end(); ++it) {
		uint sentno = it->sentno;
		PhraseSegmentation::const_iterator from_it = it->from_it;
		PhraseSegmentation::const_iterator to_it = it->to_it;
		const PhraseSegmentation &proposal = it->proposal;
		
		Float outlen = Float(countTargetWords(doc.getPhraseSegmentation(sentno)));
		s -= score(doc.getInputSentenceLength(sentno), outlen);

		std::for_each(from_it, to_it, outlen -= bind(WordPenaltyCounter(), _1));
		std::for_each(proposal.begin(), proposal.end(), outlen += bind(WordPenaltyCounter(), _1));
		s += score(doc.getInputSentenceLength(sentno), outlen);
	}
	return NULL;
}

FeatureFunction::StateModifications *SentenceLengthModel::updateScore(const DocumentState &doc,
		const SearchStep &step, const State *state, FeatureFunction::StateModifications *estmods,
		Scores::const_iterator psbegin, Scores::iterator sbegin) const {
	return estmods;
}

void SentenceLengthModel::computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const {
	*sbegin = score(doc.getInputSentenceLength(sentno), countTargetWords(doc.getPhraseSegmentation(sentno)));
}

Float SentenceLengthModel::score(Float inputlen, Float outputlen) const {
	const Float eps = Float(.1); // should we be dealing in uints here? ...
	Float diff = std::abs(inputlen - outputlen);
	if(diff < eps)
		return identicalLogprob_;
	else if(outputlen < inputlen)
		return shortLogprob_ + (diff - Float(1)) * std::log(Float(1) - shortP_) + std::log(shortP_);
	else
		return longLogprob_ + (diff - Float(1)) * std::log(Float(1) - longP_) + std::log(longP_);
}

boost::shared_ptr<FeatureFunction> FeatureFunctionFactory::create(const std::string &type, const Parameters &params) const {
	FeatureFunction *ff;
	
	if(type == "phrase-table")
		ff = new PhraseTable(params, random_);
	else if(type == "ngram-model")
		ff = NgramModelFactory::createNgramModel(params);
	/*
	else if(type == "reverse-ngram-model")
		ff = ReverseNgramModelFactory::createNgramModel(params);
	*/
	else if(type == "geometric-distortion-model")
		ff = new GeometricDistortionModel(params);
	else if(type == "sentence-length-model")
		ff = new SentenceLengthModel(params);
	else if(type == "word-penalty")
		ff = createCountingFeatureFunction(WordPenaltyCounter());
	else if(type == "oov-penalty")
		ff = createCountingFeatureFunction(OOVPenaltyCounter());
	else if(type == "long-word-penalty")
		ff = createCountingFeatureFunction(LongWordCounter(params));
	//else if(type == "word-space-cohesion-model")
	//	ff = WordSpaceCohesionModelFactory::createWordSpaceCohesionModel(params);
	//else if(type == "lexical-chain-cohesion-model")
	//	ff = LexicalChainCohesionModelFactory::createLexicalChainCohesionModel(params);
	else if(type == "semantic-space-language-model")
		ff = SemanticSpaceLanguageModelFactory::createSemanticSpaceLanguageModel(params);
	//else if(type == "pronominal-anaphora-model")
	//	ff = PronominalAnaphoraModelFactory::createPronominalAnaphoraModel(params);
	else if(type == "sentence-parity-model")
		ff = new SentenceParityModel(params);
	else if(type == "sentence-initial-char-model")
		ff = new SentenceInitialCharModel(params);
	else if(type == "sentence-final-char-model")
		ff = new SentenceFinalCharModel(params);
	else if(type == "sentence-final-word-model")
		ff = new SentenceFinalWordModel(params);
	else if(type == "final-word-rhyme-model")
		ff = new FinalWordRhymeModel(params);
	else if(type == "initial-char-model")
		ff = new InitialCharModel(params);
	else if(type == "ovix")
		ff = new OvixModel(params);
	else if(type == "type-token")
		ff = new TypeTokenRateModel(params);
	else if(type == "consistency-q-model-phrase")
		ff = new ConsistencyQModelPhrase(params);
	else if(type == "consistency-q-model-word")
		ff = new ConsistencyQModelWord(params);
	else if(type == "bleu-model")
		ff = new BleuModel(params);	
	else 
		BOOST_THROW_EXCEPTION(ConfigurationException());

	return boost::shared_ptr<FeatureFunction>(ff);
}

