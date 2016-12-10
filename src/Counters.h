/*
 *  Counters.h
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

#ifndef docent_Counters_h
#define docent_Counters_h

#include "DecoderConfiguration.h"
#include "PhrasePair.h"

#include <functional>

#include <boost/foreach.hpp>

struct PhrasePenaltyCounter
: public std::unary_function<const AnchoredPhrasePair &,Float> {
	Float operator()(const AnchoredPhrasePair &ppair) const {
		return Float(1);
	};
};

struct WordPenaltyCounter
: public std::unary_function<const AnchoredPhrasePair &,Float> {
	Float operator()(const AnchoredPhrasePair &ppair) const {
		return -Float(ppair.second.get().getTargetPhrase().get().size());
	};
};

struct OOVPenaltyCounter
: public std::unary_function<const AnchoredPhrasePair &,Float> {
	Float operator()(const AnchoredPhrasePair &ppair) const {
		return ppair.second.get().isOOV() ? Float(-1) : Float(0);
	}
};

struct LongWordCounter
: public std::unary_function<const AnchoredPhrasePair &,Float>
{
	LongWordCounter(const Parameters &params) {
		try {
			longLimit_ = params.get<uint>("long-word-length-limit");
		} catch(ParameterNotFoundException()) {
			longLimit_ = 7; //default value (from LIX)
		}
	}

	Float operator()(const AnchoredPhrasePair &ppair) const {
		int numLong = 0;
		BOOST_FOREACH(const Word &w, ppair.second.get().getTargetPhrase().get()) {
			if (w.size() >= longLimit_) {
				numLong++;
			}
		}
		return Float(-numLong);
	}
private:
	uint longLimit_;
};

#endif
