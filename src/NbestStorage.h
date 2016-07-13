/*
 *  NbestStorage.h
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

#ifndef docent_NbestStorage_h
#define docent_NbestStorage_h

#include "Docent.h"
#include "DocumentState.h"

#include <limits>
#include <vector>

#include <boost/functional/hash.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_set.hpp>

class NbestStorage {
private:
	template<class T>
	struct SmartPointerHash : public std::unary_function<T,std::size_t> {
		std::size_t operator()(T a) const {
			boost::hash<typename T::element_type> hash;
			return hash(*a);
		}
	};

	uint maxSize_;
	std::vector<boost::shared_ptr<DocumentState> > nbest_;
	boost::unordered_set<boost::shared_ptr<const DocumentState>,
		SmartPointerHash<boost::shared_ptr<const DocumentState> >,PointerEqualsTo<boost::shared_ptr<const DocumentState> > > nbestHash_;
	Float bestScore_;
	
	static bool compareScores(boost::shared_ptr<const DocumentState> a, boost::shared_ptr<const DocumentState> b);

public:
	NbestStorage(uint size);

	bool offer(const boost::shared_ptr<const DocumentState> &doc);
	void copyNbestList(std::vector<boost::shared_ptr<const DocumentState> > &outvec) const;
	
	Float getBestScore() const {
		return bestScore_;
	}

	Float getLowestScore() const {
		if(nbest_.empty())
			return -std::numeric_limits<Float>::infinity();
		else
			return nbest_[0]->getScore();
	}

	boost::shared_ptr<DocumentState> pickRandom(Random rnd) const {
		assert(!nbest_.empty());
		uint n = rnd.drawFromRange(nbest_.size());
		return nbest_[n];
	}

	const boost::shared_ptr<DocumentState>& getBestDocumentState() const {
		uint best_index = 0;
		for(uint i=1; i<nbest_.size(); ++i) {
			if (nbest_[i]->getScore() > nbest_[best_index]->getScore()) {
				best_index = i;
			}
		}
		return nbest_[best_index];
	}

	typedef std::vector<boost::shared_ptr<DocumentState> >::const_iterator const_iterator;

	const_iterator begin() const {
		return nbest_.begin();
	}

	const_iterator end() const {
		return nbest_.end();
	}
};

#endif


