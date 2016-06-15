/*
 *  NbestStorage.cpp
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

#include "NbestStorage.h"

#include <algorithm>
#include <limits>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>
#include <boost/shared_ptr.hpp>

NbestStorage::NbestStorage(uint size)
		: maxSize_(size), bestScore_(-std::numeric_limits<Float>::infinity()) {
	nbest_.reserve(size + 1);
}

inline bool NbestStorage::compareScores(boost::shared_ptr<const DocumentState> a, boost::shared_ptr<const DocumentState> b) {
	return a->getScore() > b->getScore();
}

bool NbestStorage::offer(const boost::shared_ptr<const DocumentState> &e) {
	Float newScore = e->getScore();
	if(!nbest_.empty() && newScore <= nbest_[0]->getScore())
		return false;

	if(nbestHash_.find(e) != nbestHash_.end())
		return false;

	if(newScore > bestScore_)
		bestScore_ = newScore;

	boost::shared_ptr<DocumentState> clone(new DocumentState(*e));
	nbestHash_.insert(clone);
	nbest_.push_back(clone);
	std::push_heap(nbest_.begin(), nbest_.end(), compareScores);

	while(nbest_.size() > maxSize_) {
		nbestHash_.erase(nbest_[0]);
		std::pop_heap(nbest_.begin(), nbest_.end(), compareScores);
		nbest_.resize(nbest_.size() - 1);
	}

	return true;
}

void NbestStorage::copyNbestList(std::vector<boost::shared_ptr<const DocumentState> > &outvec) const {
	outvec.resize(nbest_.size());
	std::copy(nbest_.begin(), nbest_.end(), outvec.begin());
	std::sort_heap(outvec.begin(), outvec.end(), compareScores);
}
