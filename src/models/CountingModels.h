/*
 *  CountingModels.h
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

#ifndef docent_CountingModels_h
#define docent_CountingModels_h

#include "DocumentState.h"
#include "FeatureFunction.h"
#include "SearchStep.h"

#include <algorithm>
#include <vector>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

template<class CountingFunction>
class CountingFeatureFunction : public FeatureFunction {
private:
	CountingFunction countingFunction_;
public:
	CountingFeatureFunction(
		CountingFunction countingFunction
	) :	countingFunction_(countingFunction) {}

	static FeatureFunction *createWordPenaltyFeatureFunction();
	static FeatureFunction *createOOVPenaltyFeatureFunction();

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
		Scores::const_iterator psbegin,
		Scores::iterator estbegin
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


template<class F>
CountingFeatureFunction<F> *createCountingFeatureFunction(
	F countingFunction
) {
	return new CountingFeatureFunction<F>(countingFunction);
}


struct PhrasePenaltyCounter
: public std::unary_function<const AnchoredPhrasePair &,Float> {
	Float operator()(const AnchoredPhrasePair &ppair) const {
		return Float(1);
	};
};

struct WordPenaltyCounter
: public std::unary_function<const AnchoredPhrasePair &,Float> {
	Float operator()(const AnchoredPhrasePair &ppair) const {
		return -Float(ppair.second.get().getTargetPhrase().get().size());
	};
};

struct OOVPenaltyCounter
: public std::unary_function<const AnchoredPhrasePair &,Float> {
	Float operator()(const AnchoredPhrasePair &ppair) const {
		return ppair.second.get().isOOV() ? Float(-1) : Float(0);
	}
};

struct LongWordCounter
: public std::unary_function<const AnchoredPhrasePair &,Float>
{
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
FeatureFunction::State
*CountingFeatureFunction<F>::initDocument(
	const DocumentState &doc,
	Scores::iterator sbegin
) const {
	using namespace boost::lambda;
	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();
	Float &s = *sbegin;
	s = Float(0);
	for(uint i = 0; i < segs.size(); i++)
		std::for_each(
			segs[i].begin(), segs[i].end(),
			s += bind(countingFunction_, _1)
		);
	return NULL;
}

template<class F>
void CountingFeatureFunction<F>::computeSentenceScores(
	const DocumentState &doc,
	uint sentno,
	Scores::iterator sbegin
) const {
	using namespace boost::lambda;
	Float &s = *sbegin;
	s = Float(0);
	const PhraseSegmentation &snt = doc.getPhraseSegmentation(sentno);
	std::for_each(snt.begin(), snt.end(), s += bind(countingFunction_, _1));
}

template<class F>
FeatureFunction::StateModifications
*CountingFeatureFunction<F>::estimateScoreUpdate(
	const DocumentState &doc,
	const SearchStep &step,
	const State *state,
	Scores::const_iterator psbegin,
	Scores::iterator sbegin
) const {
	using namespace boost::lambda;
	Float &s = *sbegin;
	s = *psbegin;
	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	for(std::vector<SearchStep::Modification>::const_iterator
		it = mods.begin();
		it != mods.end();
		++it
	) {
		std::for_each(
			it->from_it, it->to_it,
			s -= bind(countingFunction_, _1)
		);
		std::for_each(
			it->proposal.begin(), it->proposal.end(),
			s += bind(countingFunction_, _1)
		);
	}
	return NULL;
}

template<class F>
FeatureFunction::StateModifications
*CountingFeatureFunction<F>::updateScore(
	const DocumentState &doc,
	const SearchStep &step,
	const State *state,
	FeatureFunction::StateModifications *estmods,
	Scores::const_iterator psbegin,
	Scores::iterator sbegin
) const {
	return estmods;
}

#endif
