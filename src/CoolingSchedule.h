/*
 *  CoolingSchedule.h
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

#ifndef docent_CoolingSchedule_h
#define docent_CoolingSchedule_h

#include "Docent.h"
#include "DecoderConfiguration.h"

#include <boost/circular_buffer.hpp>

class CoolingSchedule {
public:
	virtual ~CoolingSchedule() {}

	virtual Float getTemperature() const = 0;
	virtual bool isDone() const = 0;
	virtual void step(Float score, bool accept) = 0;

	static CoolingSchedule *createCoolingSchedule(const Parameters &type);
};

class HillclimbingSchedule : public CoolingSchedule {
private:
	uint maxRejected_;
	uint rejectionCounter_;
	
public:
	HillclimbingSchedule(const Parameters &params) : rejectionCounter_(0) {
		maxRejected_ = params.get<uint>("hill-climbing:max-rejected", 1000);
	}

	virtual Float getTemperature() const {
		return 1e-10;
	}
	
	virtual bool isDone() const {
		return rejectionCounter_ > maxRejected_;
	}
	
	virtual void step(Float score, bool accept) {
		if(accept)
			rejectionCounter_ = 0;
		else
			rejectionCounter_++;
	}
};

class GeometricDecaySchedule : public CoolingSchedule {
private:
	uint step_;
	Float logStartTemperature_;
	Float logDecayFactor_;
	bool stepOnAcceptance_;

public:
	GeometricDecaySchedule(const Parameters &params) : step_(0) {
		logStartTemperature_ = log(params.get<Float>("geometric-decay:start-temperature"));
		logDecayFactor_ = log(params.get<Float>("geometric-decay:decay-factor"));
		stepOnAcceptance_ = params.get<bool>("geometric-decay:step-on-acceptance", false);
	}

	virtual Float getTemperature() const {
		return exp(logStartTemperature_ + step_ * logDecayFactor_);
	}

	virtual bool isDone() const {
		return logStartTemperature_ + step_ * logDecayFactor_ < -30;
	}
	
	virtual void step(Float score, bool accept) {
		if(accept || !stepOnAcceptance_)
			step_++;
	}
};

class AartsLaarhovenSchedule : public CoolingSchedule {
private:
	mutable Logger logger_;

	Float delta_;
	Float epsilon_;
	Float initialAcceptanceRatio_;
	uint chainLength_;
	uint initSteps_;
	
	boost::circular_buffer<Float> muBuffer_;
	Float mu1_;
	uint m1_, m2_;
	Float scoreDecrease_;
	uint stepsInChain_;
	Float lastScore_;
	std::vector<Float> chainCosts_;
	
	Float lastTemperature_;
	Float temperature_;

	void adaptInitialTemperature(Float score);
	void startNextChain();

public:
	AartsLaarhovenSchedule(const Parameters &params);

	virtual Float getTemperature() const;
	virtual bool isDone() const;
	virtual void step(Float score, bool accept);
};

#endif

