/*
 *  Random.cpp
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

#include "Random.h"

#include <cstdio>

void Random::seed() {
	FILE *urandom = std::fopen("/dev/urandom", "rb");
	if(!urandom)
		goto time_seed;

	uint seed;
	
	if(std::fread(&seed, sizeof(uint), 1, urandom) != 1)
		goto time_seed;
	
	std::fclose(urandom);
	impl_->seed(seed);
	return;

time_seed:
	if(urandom)
		std::fclose(urandom);
	
	impl_->seed(static_cast<uint>(std::time(0)));
}

void Random::seed(uint seed) {
	impl_->seed(seed);
}

RandomImplementation::RandomImplementation() :
	logger_("RandomImplementation"),
	generator_(), uintGenerator_(generator_, boost::uniform_int<uint>()) {}
	
void RandomImplementation::seed(uint seed) {
	generator_.seed(seed);
	LOG(logger_, normal, "Random number generator seed: " << seed);
}

