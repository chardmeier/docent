/*
 *  LocalBeamSearch.cpp
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

#include "NbestStorage.h"
#include "Random.h"
#include "SearchStep.h"
#include "LocalBeamSearch.h"
#include "StateGenerator.h"

#include <algorithm>
#include <limits>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/foreach.hpp>

struct LocalBeamSearchState : public SearchState {
	NbestStorage beam;
	uint rejected;
	uint nsteps;

	LocalBeamSearchState(boost::shared_ptr<DocumentState> doc, uint beamSize)
			: beam(beamSize), rejected(0), nsteps(0) {
		beam.offer(doc);
	}
};

LocalBeamSearch::LocalBeamSearch(const DecoderConfiguration &config, const Parameters &params)
		: logger_(logkw::channel = "LocalBeamSearch"), random_(config.getRandom()),
		  generator_(config.getStateGenerator()) {
	totalMaxSteps_ = params.get<uint>("max-steps");
	maxRejected_ = params.get<uint>("max-rejected");
	targetScore_ = params.get<Float>("target-score", std::numeric_limits<Float>::infinity());
	beamSize_ = params.get<uint>("beam-size");
}

SearchState *LocalBeamSearch::createState(boost::shared_ptr<DocumentState> doc) const {
	return new LocalBeamSearchState(doc, beamSize_);
}

void LocalBeamSearch::search(SearchState *sstate, NbestStorage &nbest, uint maxSteps, uint maxAccepted) const {
	LocalBeamSearchState &state = dynamic_cast<LocalBeamSearchState &>(*sstate);

	using namespace boost::lambda;
	std::for_each(state.beam.begin(), state.beam.end(), bind(&NbestStorage::offer, &nbest, _1));

	uint accepted = 0;
	uint i = 0;
	while(state.rejected < maxRejected_ && i < maxSteps && state.nsteps < totalMaxSteps_ &&
			accepted < maxAccepted && nbest.getBestScore() < targetScore_) {
		AcceptanceDecision accept(state.beam.getLowestScore());
		boost::shared_ptr<DocumentState> doc = state.beam.pickRandom(random_);
		SearchStep *step = generator_.createSearchStep(*doc);
		doc->registerAttemptedMove(step);
		if(step->isProvisionallyAcceptable(accept)) {
			if(accept(step->getScore())) {
				BOOST_LOG_SEV(logger_, debug) << "Accepting.";
				boost::shared_ptr<DocumentState> clone =
					boost::make_shared<DocumentState>(*doc);
				doc->applyModifications(step);
				BOOST_LOG_SEV(logger_, debug) << *doc;
				state.beam.offer(doc);
				nbest.offer(doc);
				accepted++;
			} else {
				BOOST_LOG_SEV(logger_, debug) << "Discarding.";
				state.rejected++;
				delete step;
			}
		} else {
			BOOST_LOG_SEV(logger_, debug) << "Discarding.";
			state.rejected++;
			delete step;
		}
		i++;
		state.nsteps++;
	}
	
	if(state.rejected >= maxRejected_)
		BOOST_LOG_SEV(logger_, normal) << "Maximum number of rejections ("
			<< maxRejected_ << ") reached.";
	
	if(i > maxSteps)
		BOOST_LOG_SEV(logger_, normal) << "Search interrupted.";

	if(accepted >= maxAccepted)
		BOOST_LOG_SEV(logger_, normal) << "Maximum number of accepted steps (" << maxAccepted << ") reached.";

	if(state.nsteps > totalMaxSteps_)
		BOOST_LOG_SEV(logger_, normal) << "Maximum number of steps (" << totalMaxSteps_ << ") reached.";
	
	if(nbest.getBestScore() > targetScore_)
		BOOST_LOG_SEV(logger_, normal) << "Found solution with better than target score.";

	for(NbestStorage::const_iterator beamit = state.beam.begin(); beamit != state.beam.end(); ++beamit) {
		const boost::shared_ptr<DocumentState> &doc = *beamit;
		DocumentState::MoveCounts::const_iterator it = doc->getMoveCounts().begin();
		while(it != doc->getMoveCounts().end()) {
			BOOST_LOG_SEV(logger_, normal) << it->second.first << '\t' << it->second.second << '\t'
				<< it->first->getDescription();
			++it;
		}
	}
}
