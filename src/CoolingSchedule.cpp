/*
 *  CoolingSchedule.cpp
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

#include "CoolingSchedule.h"

#include <limits>
#include <numeric>

#include <boost/lambda/lambda.hpp>

CoolingSchedule *CoolingSchedule::createCoolingSchedule(const Parameters &params) {
	std::string s_schedule = params.get<std::string>("schedule");

	CoolingSchedule *schedule;

	if(s_schedule == "geometric-decay")
		schedule = new GeometricDecaySchedule(params);
	else if(s_schedule == "aarts-laarhoven")
		schedule = new AartsLaarhovenSchedule(params);
	else if(s_schedule == "hill-climbing")
		schedule = new HillclimbingSchedule(params);
	else
		BOOST_THROW_EXCEPTION(ConfigurationException());

	return schedule;
}

AartsLaarhovenSchedule::AartsLaarhovenSchedule(const Parameters &params)
	: logger_("AartsLaarhovenSchedule"),
	  m1_(0), m2_(0), scoreDecrease_(.0), stepsInChain_(0),
	  lastScore_(-std::numeric_limits<Float>::infinity()),
	  temperature_(50) {
	delta_ = params.get<Float>("aarts-laarhoven:delta", .1);
	epsilon_ = params.get<Float>("aarts-laarhoven:epsilon", 1e-3);
	initialAcceptanceRatio_ = params.get<Float>("aarts-laarhoven:initial-acceptance-ratio", .95);
	chainLength_ = params.get<uint>("aarts-laarhoven:chain-length", 200);
	initSteps_ = params.get<uint>("aarts-laarhoven:init-steps", 30);

	uint movingAvgWindow = params.get<uint>("aarts-laarhoven:moving-avg-window", 15);
	muBuffer_.set_capacity(movingAvgWindow + 1);

	chainCosts_.reserve(chainLength_);
}

Float AartsLaarhovenSchedule::getTemperature() const {
	return temperature_;
}

bool AartsLaarhovenSchedule::isDone() const {
	if(!muBuffer_.full())
		return false;

	LOG(logger_, debug, "isDone: T = " << temperature_ <<
		"; mu1 = " << mu1_ << "; Tlast = " << lastTemperature_);

	if(logger_.loggable(debug))
		std::copy(muBuffer_.begin(), muBuffer_.end(),
			std::ostream_iterator<Float>(logger_.getLogStream(), " "));

	Float q = temperature_ / mu1_ * ((muBuffer_.front() - muBuffer_.back()) / (muBuffer_.size() - 1)) / (lastTemperature_ - temperature_);
	LOG(logger_, debug, "q = " << q);
	return q < epsilon_;
}

void AartsLaarhovenSchedule::step(Float score, bool accept) {
	if(initSteps_ > 0)
		adaptInitialTemperature(score);
	else {
		if(accept)
			chainCosts_.push_back(-score);
		if(++stepsInChain_ == chainLength_)
			startNextChain();
	}
	LOG(logger_, debug, "T:  " << temperature_);
}

void AartsLaarhovenSchedule::adaptInitialTemperature(Float score) {
	if(score <= IMPOSSIBLE_SCORE)
		return;

	if(score > lastScore_)
		m1_++;
	else {
		m2_++;
		scoreDecrease_ += lastScore_ - score;
	}

	lastScore_ = score;

	Float logdenom = m2_ * initialAcceptanceRatio_ - m1_ * (1.0 - initialAcceptanceRatio_);
	if(logdenom > 0) {
		temperature_ = (scoreDecrease_ / m2_) / log(m2_ / logdenom);
		initSteps_--;
	} else {
		LOG(logger_, debug,
			"Hardwiring temperature to 100 (m1 = " << m1_ << ", m2 = " << m2_ << ")"
		);
		temperature_ = 100;
	}

	LOG(logger_, debug,
		"adaptInitialTemperature: m1: " << m1_ << "; m2: " << m2_
	);
}

void AartsLaarhovenSchedule::startNextChain() {
	using namespace boost::lambda;
	LOG(logger_, debug, "chainScores:");
	if(logger_.loggable(debug))
		std::copy(chainCosts_.begin(), chainCosts_.end(),
			std::ostream_iterator<Float>(logger_.getLogStream(), " "));

	Float mu = std::accumulate(
			chainCosts_.begin(),
			chainCosts_.end(),
			static_cast<Float>(0)
		) / chainCosts_.size();
	Float sigma_sq = std::accumulate(
			chainCosts_.begin(),
			chainCosts_.end(),
			static_cast<Float>(0),
			_1 + (_2 - mu) * (_2 - mu)
		) / chainCosts_.size();
	lastTemperature_ = temperature_;
	temperature_ /= 1 + temperature_ * log(1 + delta_) / (3 * sqrt(sigma_sq));
	chainCosts_.front() = chainCosts_.back();
	chainCosts_.resize(1);
	if(muBuffer_.empty())
		mu1_ = mu;
	muBuffer_.push_back(mu);
	stepsInChain_ = 0;
}
