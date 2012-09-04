/*
 *  SimulatedAnnealing.h
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

#ifndef docent_SimulatedAnnealing_h
#define docent_SimulatedAnnealing_h

#include "Docent.h"
#include "DecoderConfiguration.h"
#include "SearchAlgorithm.h"

class DocumentState;
class NbestStorage;
class Random;

class SimulatedAnnealing : public SearchAlgorithm {
private:
	mutable Logger logger_;
	Random random_;
	const StateGenerator &generator_;
	uint totalMaxSteps_;
	Float targetScore_;
	Parameters parameters_;

public:
	SimulatedAnnealing(const DecoderConfiguration &config, const Parameters &params);

	virtual SearchState *createState(boost::shared_ptr<DocumentState> doc) const;
	virtual void search(SearchState *sstate, NbestStorage &nbest, uint maxSteps, uint maxAccepted) const;
};

#endif

