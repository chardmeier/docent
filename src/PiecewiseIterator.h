/*
 *  PiecewiseIterator.h
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

#ifndef docent_PiecewiseIterator_h
#define docent_PiecewiseIterator_h

#include "Docent.h"

#include <vector>

#include <boost/iterator_adaptors.hpp>
#include <boost/utility.hpp>

template<class PieceIterator>
class PiecewiseIterator : public boost::iterator_adaptor<PiecewiseIterator<PieceIterator>,typename std::iterator_traits<PieceIterator>::value_type> {
	friend class boost::iterator_core_access;

private:
	typedef typename std::iterator_traits<PieceIterator>::value_type BaseIterator;

	Logger logger_;

	PieceIterator piecesBegin_;
	PieceIterator piecesEnd_;
	PieceIterator currentPiece_;
	bool goingForward_;

	void init(PieceIterator begin, PieceIterator startPiece, PieceIterator end, BaseIterator initIterator) {
		piecesBegin_ = begin;
		piecesEnd_ = end;
		currentPiece_ = ++startPiece;
		goingForward_ = true;

		LOG_DEBUGBUILD(logger_, debug, "Initialising PiecewiseIterator.");

		// normalise iterator if it's the end iterator of a piece or the adjacent pieces are empty
		if(initIterator == *currentPiece_) {
			LOG_DEBUGBUILD(logger_, debug, "Normalising.");
			++currentPiece_;
			while(currentPiece_ != piecesEnd_ && *currentPiece_ == *boost::next(currentPiece_)) {
				LOG_DEBUGBUILD(logger_, debug, "Skipping empty piece.");
				incrementPiece();
				incrementPiece();
			}
			if(currentPiece_ == piecesEnd_) {
				LOG_DEBUGBUILD(logger_, debug, "at end");
				decrementPiece();
				this->base_reference() = *currentPiece_;
			} else {
				this->base_reference() = *currentPiece_;
				incrementPiece();
			}
		}
		LOG_DEBUGBUILD(logger_, debug, "now at "
			<< std::distance(piecesBegin_, currentPiece_) << " of "
			<< std::distance(piecesBegin_, piecesEnd_) << ".");
	}

public:
	PiecewiseIterator(PieceIterator begin, PieceIterator end) :
			PiecewiseIterator::iterator_adaptor_(*begin),
			logger_("PiecewiseIterator") {
		init(begin, begin, end, *begin);
	}
	
	PiecewiseIterator(PieceIterator begin, PieceIterator startPiece, PieceIterator end,
				BaseIterator initIterator) :
			PiecewiseIterator::iterator_adaptor_(initIterator),
			logger_("PiecewiseIterator") {
		init(begin, startPiece, end, initIterator);
	}

private:
	void incrementPiece() {
		++currentPiece_;
		LOG_DEBUGBUILD(logger_, debug, "Incremented piece iterator, now at "
			<< std::distance(piecesBegin_, currentPiece_) << " of "
			<< std::distance(piecesBegin_, piecesEnd_) << ".");
	}

	void decrementPiece() {
		--currentPiece_;
		LOG_DEBUGBUILD(logger_, debug, "Decremented piece iterator, now at "
			<< std::distance(piecesBegin_, currentPiece_) << " of "
			<< std::distance(piecesBegin_, piecesEnd_) << ".");
	}

	void increment() {
		assert(currentPiece_ != piecesEnd_);
		LOG_DEBUGBUILD(logger_, debug, "Incrementing PiecewiseIterator.");

		if(!goingForward_) {
			LOG_DEBUGBUILD(logger_, debug, "Going forward.");
			incrementPiece();
			goingForward_ = true;
		}

		++this->base_reference();
		if(this->base() == *currentPiece_) {
			LOG_DEBUGBUILD(logger_, debug, "Reached end of piece.");
			LOG(logger_, debug, "Going backward.");
			incrementPiece();
			goingForward_ = false;
			while(currentPiece_ != piecesEnd_) {
				assert(boost::next(currentPiece_) != piecesEnd_);
				if( *currentPiece_ != *boost::next(currentPiece_)) break;
				LOG_DEBUGBUILD(logger_, debug, "Skipping empty piece.");
				incrementPiece();
				incrementPiece();
			}
			if(currentPiece_ == piecesEnd_) {
				LOG_DEBUGBUILD(logger_, debug, "at end, going forward");
				decrementPiece();
				goingForward_ = true;
			}
			this->base_reference() = *currentPiece_;
		}
	}
	
	void decrement() {
		LOG_DEBUGBUILD(logger_, debug, "Decrementing PiecewiseIterator.");

		if(goingForward_) {
			LOG_DEBUGBUILD(logger_, debug, "Going backward.");
			decrementPiece();
			goingForward_ = false;
		}
		
		if(this->base() == *currentPiece_) {
			LOG_DEBUGBUILD(logger_, debug, "Reached beginning of piece.");
			assert(currentPiece_ != piecesBegin_);
			LOG_DEBUGBUILD(logger_, debug, "Going forward.");
			decrementPiece();
			goingForward_ = true;
			while(currentPiece_ != piecesBegin_ && *currentPiece_ == *boost::prior(currentPiece_)) {
				LOG_DEBUGBUILD(logger_, debug, "Skipping empty piece.");
				decrementPiece();
				decrementPiece();
			}
			this->base_reference() = *currentPiece_;
		}

		--this->base_reference();
	}
};

#endif

