/*
 *  SearchStep.cpp
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
#include "DecoderConfiguration.h"
#include "Random.h"
#include "SearchStep.h"
#include "SimulatedAnnealing.h"

#include <algorithm>
#include <limits>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>
#include <boost/tuple/tuple_comparison.hpp>

SearchStep::SearchStep(
	const StateOperation *op,
	const DocumentState &doc,
	const std::vector<FeatureFunction::State *> &featureStates
) :	logger_("SearchStep"),
	document_(doc),
	generation_(doc.getGeneration()),
	featureStates_(featureStates),
	configuration_(*doc.getDecoderConfiguration()),
	stateModifications_(configuration_.getFeatureFunctions().size()),
	operation_(op),
	modificationsConsolidated_(true),
	scores_(doc.getScores().size()),
	scoreState_(NoScores)
{}

SearchStep::~SearchStep()
{
	using namespace boost::lambda;
	std::for_each(
		stateModifications_.begin(),
		stateModifications_.end(),
		bind(delete_ptr(), _1)
	);
}

void SearchStep::consolidateModifications()
const {
	if(modifications_.empty())
		return;

	std::sort(modifications_.begin(), modifications_.end(), compareModifications);

	uint removed = 0;
	std::vector<Modification>::iterator prev = modifications_.begin();
	for(std::vector<Modification>::iterator
		it = prev + 1;
		it != modifications_.end();
		prev = it, ++it
	) {
		if(prev->sentno == it->sentno && prev->to == it->from) {
			it->from = prev->from;
			it->from_it = prev->from_it;
			it->proposal.splice(it->proposal.begin(), prev->proposal);

			prev->sentno = std::numeric_limits<uint>::max();
			removed++;
		}
	}

	if(removed > 0) {
		std::sort(modifications_.begin(), modifications_.end(), compareModifications);
		modifications_.erase(modifications_.end() - removed, modifications_.end());
	}

	modificationsConsolidated_ = true;
}

bool SearchStep::compareModifications(
	const Modification &a,
	const Modification &b
) {
	namespace t = boost::tuples;
	return t::make_tuple(a.sentno, a.from, a.to)
		<  t::make_tuple(b.sentno, b.from, b.to);
}

void SearchStep::estimateScores()
const {
	if(scoreState_ != NoScores)
		return;

	Scores::const_iterator oldscoreit = document_.getScores().begin();
	Scores::iterator scoreit = scores_.begin();
	const DecoderConfiguration::FeatureFunctionList &ff = configuration_.getFeatureFunctions();
	for(uint
		i = 0;
		i < ff.size();
		scoreit += ff[i].getNumberOfScores(), oldscoreit += ff[i].getNumberOfScores(), i++
	) {
		stateModifications_[i] = ff[i].estimateScoreUpdate(
			document_,
			*this,
			featureStates_[i],
			oldscoreit,
			scoreit
		);
	}

	scoreState_ = ScoresEstimated;
}

void SearchStep::computeScores()
const {
	switch(scoreState_) {
	case NoScores:
		estimateScores();
	case ScoresEstimated:
		break;
	case ScoresComputed:
		return;
	}

	Scores::const_iterator oldscoreit = document_.getScores().begin();
	Scores::iterator scoreit = scores_.begin();
	const DecoderConfiguration::FeatureFunctionList &ff = configuration_.getFeatureFunctions();
	for(uint
		i = 0;
		i < ff.size();
		scoreit += ff[i].getNumberOfScores(), oldscoreit += ff[i].getNumberOfScores(), i++
	) {
		stateModifications_[i] = ff[i].updateScore(
			document_,
			*this,
			featureStates_[i],
			stateModifications_[i],
			oldscoreit,
			scoreit
		);
	}
	scoreState_ = ScoresComputed;
}

bool SearchStep::isProvisionallyAcceptable(
	const AcceptanceDecision &accept
) const {
	estimateScores();
	Float estScore = std::inner_product(
		scores_.begin(), scores_.end(),
		configuration_.getFeatureWeights().begin(),
		static_cast<Float>(0)
	);
	return accept(estScore);
}
