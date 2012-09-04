/*
 *  SearchAlgorithm.cpp
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
#include "LocalBeamSearch.h"
//#include "MetropolisHastingsSampler.h"
#include "SearchAlgorithm.h"
#include "SimulatedAnnealing.h"

SearchAlgorithm *SearchAlgorithm::createSearchAlgorithm(const std::string &algo,
		const DecoderConfiguration &config, const Parameters &params) {
	if(algo == "simulated-annealing")
		return new SimulatedAnnealing(config, params);
	else if(algo == "local-beam-search")
		return new LocalBeamSearch(config, params);
	//else if(algo == "metropolis-hastings-sampler")
	//	return new MetropolisHastingsSampler(config, params);
	else {
		Logger logger(logkw::channel = "DecoderConfiguration");
		BOOST_LOG_SEV(logger, error) << "Unknown search algorithm: " << algo;
		exit(1);
	}
}
