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

#ifndef docent_SearchAlgorithm_h
#define docent_SearchAlgorithm_h

#include "Docent.h"
#include "NbestStorage.h"
#include "Random.h"

#include <functional>
#include <limits>

#include <boost/shared_ptr.hpp>

class DecoderConfiguration;
class DocumentState;
class Parameters;

class AcceptanceDecision : public std::unary_function<Float,bool> {
private:
	Logger logger_;

	Float threshold_;

	Float d_, T_, oldScore_;

public:
	AcceptanceDecision(
		Float threshold
	) :	logger_("AcceptanceDecision"),
		threshold_(threshold),
		d_(0),
		T_(0),
		oldScore_(0)
	{}

	AcceptanceDecision(
		Random rnd,
		Float T,
		Float oldScore
	) :	logger_("AcceptanceDecision")
	{
		// compute acceptance threshold for acceptance with probability exp((old - new) / T)
		Float d = rnd.draw01();
		threshold_ = T * log(d) + oldScore;

		d_ = d;
		T_ = T;
		oldScore_ = oldScore;
	}

	bool operator()(Float newScore) const {
		LOG(logger_, debug, "new:                  " << newScore);
		LOG(logger_, debug, "old:                  " << oldScore_);
		LOG(logger_, debug, "threshold:            " << threshold_);
		LOG(logger_, debug, "T:                    " << T_);
		LOG(logger_, debug, "d:                    " << d_);
		LOG(logger_, debug, "exp((new - old) / T): " << exp(-(oldScore_ - newScore) / T_));
		LOG(logger_, debug, "should accept:        " << (exp(-(oldScore_ - newScore) / T_) >= d_));
		LOG(logger_, debug, "will accept:          " << (newScore > threshold_));
		return newScore > threshold_;
	}
};

struct SearchState {
	virtual ~SearchState() {}
	virtual const boost::shared_ptr<DocumentState>
	&getLastDocumentState() = 0;
};

struct SearchAlgorithm {
	static SearchAlgorithm
	*createSearchAlgorithm(
		const std::string &algo,
		const DecoderConfiguration &config,
		const Parameters &params
	);

	virtual ~SearchAlgorithm() {}

	virtual SearchState
	*createState(
		boost::shared_ptr<DocumentState> doc
	) const = 0;

	virtual void search(
		SearchState *sstate,
		NbestStorage &nbest,
		uint maxSteps,
		uint maxAccepted
	) const = 0;

	void search(
		SearchState *sstate,
		NbestStorage &nbest
	) const {
		search(
			sstate,
			nbest,
			std::numeric_limits<uint>::max(),
			std::numeric_limits<uint>::max()
		);
	}

	void search(
		boost::shared_ptr<DocumentState> doc,
		NbestStorage &nbest
	) const {
		SearchState *state = createState(doc);
		search(state, nbest);
		delete state;
	}
};

#endif
