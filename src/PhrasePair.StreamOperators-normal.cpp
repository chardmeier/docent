/*
 *  PhrasePair.cpp
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

#include <iostream>
#include <iterator>

#include <boost/foreach.hpp>

#include "Docent.h"
#include "PhrasePair.h"

std::ostream &operator<<(std::ostream &os, const std::vector<Word> &phrase)
{
	bool first = true;
	BOOST_FOREACH(const Word &w, phrase) {
		if(first)
			first = false;
		else
			os << ' ';
		os << w;
	}
	return os;
}

std::ostream &operator<<(std::ostream &os, const PhraseSegmentation &seg)
{
	std::copy(seg.begin(), seg.end(),
		std::ostream_iterator<AnchoredPhrasePair>(os, "\n")
	);
	return os;
}

std::ostream &operator<<(std::ostream &os, const AnchoredPhrasePair &ppair)
{
	os	<< ppair.first << "\t["
		<< ppair.second.get().getSourcePhrase().get() << "] -\t["
		<< ppair.second.get().getTargetPhrase().get() << ']';
	return os;
}
