/*
 *  SimulatedAnnealing.cpp
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

#include "SimulatedAnnealing.h"

#include "CoolingSchedule.h"
#include "NbestStorage.h"
#include "Random.h"
#include "SearchStep.h"
#include "StateGenerator.h"

#include <boost/make_shared.hpp>

#include <limits>

struct SimulatedAnnealingSearchState
:	public SearchState
{
	boost::shared_ptr<DocumentState> document;
	CoolingSchedule *schedule;
	uint nsteps;
	bool aborted;

	SimulatedAnnealingSearchState(
		boost::shared_ptr<DocumentState> doc,
		const Parameters &params
	) :	document(doc),
		nsteps(0),
		aborted(false)
	{
		schedule = CoolingSchedule::createCoolingSchedule(params);
	}

	~SimulatedAnnealingSearchState() {
		delete schedule;
	}

	const boost::shared_ptr<DocumentState>& getLastDocumentState() {
		return document;
	}
};

SimulatedAnnealing::SimulatedAnnealing(
	const DecoderConfiguration &config,
	const Parameters &params
) :	logger_("SimulatedAnnealing"),
	random_(config.getRandom()),
	generator_(config.getStateGenerator()),
	parameters_(params)
{
	totalMaxSteps_ = params.get<uint>("max-steps");
	targetScore_ = params.get<Float>("target-score", std::numeric_limits<Float>::infinity());
}

SearchState *SimulatedAnnealing::createState(boost::shared_ptr<DocumentState> doc) const {
	return new SimulatedAnnealingSearchState(doc, parameters_);
}

void SimulatedAnnealing::search(
	SearchState *sstate,
	NbestStorage &nbest,
	uint maxSteps,
	uint maxAccepted
) const {
	SimulatedAnnealingSearchState
		&state = dynamic_cast<SimulatedAnnealingSearchState &>(*sstate);

	LOG(logger_, debug, *state.document);

	nbest.offer(state.document);

	uint accepted = 0;
	uint i = 0;
	while(
		!state.aborted
		&& !state.schedule->isDone()
		&& i < maxSteps
		&& state.nsteps < totalMaxSteps_
		&& accepted < maxAccepted
		&& nbest.getBestScore() < targetScore_
	) {
		AcceptanceDecision accept(
			random_,
			state.schedule->getTemperature(),
			state.document->getScore()
		);
		SearchStep *step = generator_.createSearchStep(*state.document);
		if(step == NULL) {
			state.aborted = true;
			break;
		}
		state.document->registerAttemptedMove(step);
		if(step->isProvisionallyAcceptable(accept)) {
			if(accept(step->getScore())) {
				LOG(logger_, debug, "Accepting.");
				state.schedule->step(step->getScore(), true);
				state.document->applyModifications(step);
				LOG(logger_, debug, *state.document);
				nbest.offer(state.document);
				accepted++;
			} else {
				state.schedule->step(step->getScore(), false);
				LOG(logger_, debug, "Discarding.");
				delete step;
			}
		} else {
			state.schedule->step(step->getScoreEstimate(), false);
			LOG(logger_, debug, "Discarding.");
			delete step;
		}
		i++;
		state.nsteps++;
	}

	if(state.aborted)
		LOG(logger_, normal, "Document search aborted.");

	if(state.schedule->isDone())
		LOG(logger_, normal, "End of cooling schedule reached.");

	if(accepted >= maxAccepted)
		LOG(logger_, normal, "Maximum number of accepted steps (" << maxAccepted << ") reached.");

	if(i >= maxSteps)
		LOG(logger_, normal, "Interrupting search.");

	if(state.nsteps >= totalMaxSteps_)
		LOG(logger_, normal, "Maximum number of steps (" << totalMaxSteps_ << ") reached.");

	if(nbest.getBestScore() > targetScore_)
		LOG(logger_, normal, "Found solution with better than target score.");

	DocumentState::MoveCounts::const_iterator it = state.document->getMoveCounts().begin();
	while(it != state.document->getMoveCounts().end()) {
		LOG(logger_, normal,
			   it->second.first << '\t'
			<< it->second.second << '\t'
			<< it->first->getDescription()
		);
		++it;
	}
}
