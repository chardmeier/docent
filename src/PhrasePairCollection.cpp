/*
 *  PhrasePairCollection.cpp
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

#include "PhrasePairCollection.h"
#include "Random.h"

#include <algorithm>
#include <iterator>

#include <boost/function.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

PhrasePairCollection::PhrasePairCollection(const PhraseTable &phraseTable, uint sentenceLength,
		uint annotationCount, Random random)
	: logger_("PhrasePairCollection"),
	  phraseTable_(phraseTable), random_(random),
	  sentenceLength_(sentenceLength), annotationCount_(annotationCount) {}

void PhrasePairCollection::addPhrasePair(CoverageBitmap cov, PhrasePair phrasePair) {
	LOG(logger_, verbose, "addPhrasePair " << cov << " " <<
		phrasePair.get().getSourcePhrase().get() << " " << phrasePair.get().getTargetPhrase().get() <<
		" " << phrasePair.get().getScores());
	phrasePairList_.push_back(std::make_pair(cov, phrasePair));
}

PhraseSegmentation PhrasePairCollection::proposeSegmentation() const {
	CoverageBitmap all(sentenceLength_);
	all.set();
	return proposeSegmentation(all);
}

PhraseSegmentation PhrasePairCollection::proposeSegmentation(const CoverageBitmap &range) const {
	using namespace boost::lambda;

	assert(range.size() == sentenceLength_);
	
	PhraseSegmentation seg;
	bool success;

	//if(range.count() < 5)
	//	success = proposeSegmentationRandomChoice(range, phrasePairList_, seg);
	//else {
		std::vector<AnchoredPhrasePair> ppairs;
		std::remove_copy_if(phrasePairList_.begin(), phrasePairList_.end(), std::back_inserter(ppairs),
			!bind(&CoverageBitmap::is_subset_of, bind<const CoverageBitmap &>(&AnchoredPhrasePair::first, _1), range));
		std::sort(ppairs.begin(), ppairs.end(),
			bind(&CoverageBitmap::find_first, bind<const CoverageBitmap &>(&AnchoredPhrasePair::first, _1)) <
				bind(&CoverageBitmap::find_first, bind<const CoverageBitmap &>(&AnchoredPhrasePair::first, _2)));

		success = proposeSegmentationLeftRight(range, ppairs.begin(), ppairs.end(), seg);
	//}

	assert(success); // TODO: should throw here
	assert(!seg.empty());

	return seg;
}

bool PhrasePairCollection::proposeSegmentationLeftRight(const CoverageBitmap &range,
		std::vector<AnchoredPhrasePair>::const_iterator startit, std::vector<AnchoredPhrasePair>::const_iterator endit,
		PhraseSegmentation &seg) const {
	using namespace boost::lambda;

	LOG(logger_, verbose, "proposeSegmentation " << range);

	if(range.none()) {
		LOG(logger_, debug, "Range empty.");
		return true;
	}

	if(startit == endit) {
		LOG(logger_, debug, "No phrase pairs available.");
		return false;
	}

	LOG(logger_, debug, "                    " << startit->first);

	CoverageBitmap::size_type firstBit = range.find_first();
	std::vector<AnchoredPhrasePair>::const_iterator it1 = std::find_if(startit, endit,
		bind(&CoverageBitmap::test, bind<const CoverageBitmap &>(&AnchoredPhrasePair::first, _1), firstBit));

	if(it1 == endit) {
		LOG(logger_, debug, "firstBit = " << firstBit);
		LOG(logger_, debug, "startit->find_first() = " << startit->first.find_first());
		LOG(logger_, debug, "No matching phrase pair.");
		return false;
	}

	std::vector<AnchoredPhrasePair>::const_iterator it2 = std::find_if(it1, endit,
		bind(&CoverageBitmap::find_first, bind<const CoverageBitmap &>(&AnchoredPhrasePair::first, _1)) > firstBit);
	uint noptions = std::distance(it1, it2);
	uint choice;
	std::vector<AnchoredPhrasePair>::const_iterator ph;
	CoverageBitmap badChoices(noptions);
	bool done = false;
	do {
		if(badChoices.count() == noptions) {
			LOG(logger_, debug, "Exhausted all choices.");
			return false;
		}

		do {
			choice = random_.drawFromRange(noptions);
		} while(badChoices.test(choice));
		badChoices.set(choice);
		ph = it1 + choice;
		
		if((ph->first & range) != ph->first) { // are we covering something we shouldn't?
			LOG(logger_, debug, "Conflicting coverage:");
			LOG(logger_, debug, ph->first);
			LOG(logger_, debug, range);
			continue;
		}

		LOG(logger_, debug, "selected            " << ph->first);
		CoverageBitmap::size_type i = firstBit, next;
		LOG(logger_, debug, "First bit: " << i);
		while((next = ph->first.find_next(i)) == i + 1) // find first gap in phrase
			i = next;
		i++;
		LOG(logger_, debug, "Next unset bit: " << i);
		std::vector<AnchoredPhrasePair>::const_iterator it2new = std::find_if(it2, endit,
			bind(&CoverageBitmap::find_first, bind<const CoverageBitmap &>(&AnchoredPhrasePair::first, _1)) >= i);
		
		done = proposeSegmentationLeftRight(range - ph->first, it2new, endit, seg);
	} while(!done);

	LOG(logger_, debug, "Proposing " << *ph);
	seg.push_front(*ph);
	return true;
}

bool PhrasePairCollection::proposeSegmentationRandomChoice(CoverageBitmap range, const PhrasePairList_ &list, PhraseSegmentation &seg) const {
	using namespace boost::lambda;

	LOG(logger_, verbose, "proposeSegmentation " << range);
	
	if(range.none())
		return true;

	typedef boost::function<bool(const AnchoredPhrasePair &)> FilterPred;
	FilterPred filterPredicate = bind(&CoverageBitmap::is_subset_of, bind<const CoverageBitmap &>(&AnchoredPhrasePair::first, _1), range);
	boost::filter_iterator<FilterPred,PhraseSegmentation::const_iterator> fit1 =
		boost::make_filter_iterator(filterPredicate, list.begin(), list.end());
	boost::filter_iterator<FilterPred,PhraseSegmentation::const_iterator> fit2 =
		boost::make_filter_iterator(filterPredicate, list.end(), list.end());
	PhrasePairList_ sublist(fit1, fit2);

	while(!sublist.empty()) {
		uint phidx = random_.drawFromRange(sublist.size());
		PhrasePairList_::const_iterator ph = sublist.begin();
		while(phidx--)
			++ph;
		LOG(logger_, debug, "selected            " << ph->first);
		if(proposeSegmentationRandomChoice(range - ph->first, sublist, seg)) {
			LOG(logger_, debug, "Proposing " << *ph);
			seg.push_front(*ph);
			return true;
		}
		sublist.remove_if(bind(&AnchoredPhrasePair::first, _1) == ph->first);
	}

	return false;
}

const AnchoredPhrasePair &PhrasePairCollection::proposeAlternativeTranslation(const AnchoredPhrasePair &old) const {
	using namespace boost::lambda;
	
	typedef boost::function<bool(const AnchoredPhrasePair &)> FilterPred;
	FilterPred filterPredicate = bind<const CoverageBitmap &>(&AnchoredPhrasePair::first, _1) == old.first;
	boost::filter_iterator<FilterPred,PhraseSegmentation::const_iterator> fit1 =
		boost::make_filter_iterator(filterPredicate, phrasePairList_.begin(), phrasePairList_.end());
	boost::filter_iterator<FilterPred,PhraseSegmentation::const_iterator> fit2 =
		boost::make_filter_iterator(filterPredicate, phrasePairList_.end(), phrasePairList_.end());

	std::vector<const AnchoredPhrasePair *> sublist;
	std::transform(fit1, fit2, std::back_inserter(sublist), &_1);
	if(sublist.size() == 0)
		return old;
	
	uint phidx = random_.drawFromRange(sublist.size());
	return *sublist[phidx];
}


bool PhrasePairCollection::phrasesExist(const PhraseSegmentation& phraseSegmentation) const {
	for(PhraseSegmentation::const_iterator pit1 = phraseSegmentation.begin(); pit1 != phraseSegmentation.end(); ++pit1) {
		bool found = false;
		for (PhrasePairList_::const_iterator pit2 = phrasePairList_.begin(); pit2 != phrasePairList_.end(); ++pit2) {
			if (*pit1  == *pit2) {
				found = true;
				break;
			}
		}
		if (!found) {
			return false;
		}
	}
	return true;
}
