/*
 *  Random.h
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

#ifndef docent_Random_h
#define docent_Random_h

#include "Docent.h"

#include <algorithm>
#include <limits>
#include <numeric>
#include <vector>

#include <boost/random/geometric_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/shared_ptr.hpp>

class RandomImplementation {
	friend class Random;

private:
	typedef boost::mt19937 RandomGenerator_;

public:
	typedef boost::variate_generator<RandomGenerator_ &,boost::uniform_int<uint> > UintGenerator;
	
private:
	Logger logger_;

	// We don't consider the state change induced by drawing a random number a modification,
	// so the random generator is declared mutable.
	mutable RandomGenerator_ generator_;
	UintGenerator uintGenerator_;

	RandomImplementation(const RandomImplementation &o);
	RandomImplementation &operator=(const RandomImplementation &);
	
	RandomImplementation();

public:
	void seed(uint seed);

	inline uint drawFromRange(uint noptions) const;
	
	inline uint drawFromCumulativeDistribution(const std::vector<Float> &distribution) const;
	inline uint drawFromDiscreteDistribution(const std::vector<Float> &distribution) const;
	
	inline uint drawFromGeometricDistribution(Float decay, uint cap = std::numeric_limits<uint>::max()) const;
	
	inline Float draw01() const;
	inline bool flipCoin(Float p = .5) const;

	UintGenerator &getUintGenerator() const {
		return const_cast<UintGenerator &>(uintGenerator_);
	}
};

class Random {
private:
	boost::shared_ptr<RandomImplementation> impl_;
	explicit Random(RandomImplementation *impl) : impl_(impl) {}

	// This will create the object in an invalid state. You need to
	// call setup() in order to make it valid.
	// The default constructor is private to make sure we don't inadvertently
	// create an unseeded random generator. Using the copy constructor is ok.
	Random() : impl_(new RandomImplementation()) {}

public:
	typedef RandomImplementation::UintGenerator UintGenerator;

	// default copy constructor

	static Random create() {
		return Random();
	}

	void seed();
	void seed(uint seed);
	
	uint drawFromRange(uint noptions) const {
		return impl_->drawFromRange(noptions);
	}
	
	uint drawFromCumulativeDistribution(const std::vector<Float> &distribution) const {
		return impl_->drawFromCumulativeDistribution(distribution);
	}

	uint drawFromDiscreteDistribution(const std::vector<Float> &distribution) const {
		return impl_->drawFromDiscreteDistribution(distribution);
	}
	
	uint drawFromGeometricDistribution(Float decay, uint cap = std::numeric_limits<uint>::max()) const {
		return impl_->drawFromGeometricDistribution(decay, cap);
	}
	
	Float draw01() const {
		return impl_->draw01();
	}

	bool flipCoin(Float p = .5) const {
		return impl_->flipCoin(p);
	}

	UintGenerator &getUintGenerator() const {
		return impl_->getUintGenerator();
	}
};

uint RandomImplementation::drawFromRange(uint noptions) const {
	assert(noptions > 0);
	boost::uniform_int<uint> distr(0, noptions-1);
	return distr(generator_);
}

uint RandomImplementation::drawFromCumulativeDistribution(const std::vector<Float> &cumulative) const {
    boost::uniform_real<Float> dist(0, cumulative.back());
    return std::lower_bound(cumulative.begin(), cumulative.end(), dist(generator_)) - cumulative.begin();
}

uint RandomImplementation::drawFromDiscreteDistribution(const std::vector<Float> &distribution) const {
	std::vector<Float> cumulative;
	std::partial_sum(distribution.begin(), distribution.end(),
                     std::back_inserter(cumulative));
	return drawFromCumulativeDistribution(cumulative);
}

uint RandomImplementation::drawFromGeometricDistribution(Float decay, uint cap) const {
	boost::geometric_distribution<uint,Float> dist(decay);
	return std::min(dist(generator_), cap);
}

Float RandomImplementation::draw01() const {
	boost::uniform_01<Float> dist;
	return dist(generator_);
}

bool RandomImplementation::flipCoin(Float p) const {
	return draw01() <= p;
}

#endif

