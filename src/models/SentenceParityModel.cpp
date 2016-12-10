/*
 *  SentenceParityModel.cpp
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

#include "models/SentenceParityModel.h"

#include "DocumentState.h"
#include "Counters.h"
#include "SearchStep.h"

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

struct SentenceParityModelState
:	public FeatureFunction::State,
	public FeatureFunction::StateModifications
{
	SentenceParityModelState(uint nsents) : outputLength(nsents) {}

	std::vector<uint> outputLength;

	Float score() const {
		uint neven = 0;
		uint nodd = 0;
		for(uint i = 0; i < outputLength.size(); i++)
			if(outputLength[i] % 2 == 0)
				neven++;
			else
				nodd++;

		if(neven > nodd)
			return -Float(nodd);
		else
			return -Float(neven);
	}

	virtual SentenceParityModelState *clone() const {
		return new SentenceParityModelState(*this);
	}
};


FeatureFunction::State *SentenceParityModel::initDocument(
	const DocumentState &doc,
	Scores::iterator sbegin
) const {
	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();

	SentenceParityModelState *s = new SentenceParityModelState(segs.size());
	for(uint i = 0; i < segs.size(); i++)
		s->outputLength[i] = countTargetWords(segs[i]);

	*sbegin = s->score();
	return s;
}

FeatureFunction::StateModifications *SentenceParityModel::estimateScoreUpdate(
	const DocumentState &doc,
	const SearchStep &step,
	const State *state,
	Scores::const_iterator psbegin,
	Scores::iterator sbegin
) const {
	const SentenceParityModelState *prevstate = dynamic_cast<const SentenceParityModelState *>(state);
	SentenceParityModelState *s = prevstate->clone();

	WordPenaltyCounter counter;

	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	for(std::vector<SearchStep::Modification>::const_iterator it = mods.begin(); it != mods.end(); ++it) {
		uint sentno = it->sentno;
		PhraseSegmentation::const_iterator from_it = it->from_it;
		PhraseSegmentation::const_iterator to_it = it->to_it;
		const PhraseSegmentation &proposal = it->proposal;

		using namespace boost::lambda;
		std::for_each(
			from_it, to_it,
			s->outputLength[sentno] -= boost::lambda::bind(counter, _1)
		);
		std::for_each(
			proposal.begin(), proposal.end(),
			s->outputLength[sentno] += boost::lambda::bind(counter, _1)
		);
	}

	*sbegin = s->score();
	return s;
}

FeatureFunction::StateModifications *SentenceParityModel::updateScore(
	const DocumentState &doc,
	const SearchStep &step,
	const State *state,
	FeatureFunction::StateModifications *estmods,
	Scores::const_iterator psbegin,
	Scores::iterator sbegin
) const {
	return estmods;
}

FeatureFunction::State *SentenceParityModel::applyStateModifications(
	FeatureFunction::State *oldState,
	FeatureFunction::StateModifications *modif
) const {
	SentenceParityModelState *os = dynamic_cast<SentenceParityModelState *>(oldState);
	SentenceParityModelState *ms = dynamic_cast<SentenceParityModelState *>(modif);
	os->outputLength.swap(ms->outputLength);
	return oldState;
}

void SentenceParityModel::computeSentenceScores(
	const DocumentState &doc,
	uint sentno,
	Scores::iterator sbegin
) const {
	*sbegin = Float(0);
}
