/*
 *  PhrasePairCollection.h
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

#ifndef docent_PhrasePairCollection_h
#define docent_PhrasePairCollection_h

#include "Docent.h"
#include "PhrasePair.h"
#include "Random.h"

#include <iterator>
#include <list>

class PhrasePairCollection {
	friend class PhraseTable;

private:
	Logger logger_;

	const PhraseTable &phraseTable_;
	Random random_;

	uint sentenceLength_;
	typedef std::list<AnchoredPhrasePair> PhrasePairList_;
	PhrasePairList_ phrasePairList_;

	PhrasePairCollection(const PhraseTable &phraseTable, uint sentenceLength, Random random);
	void addPhrasePair(CoverageBitmap cov, PhrasePair phrasePair);

	bool proposeSegmentationLeftRight(const CoverageBitmap &range,
		std::vector<AnchoredPhrasePair>::const_iterator startit, std::vector<AnchoredPhrasePair>::const_iterator endit,
		PhraseSegmentation &seg) const;
	bool proposeSegmentationRandomChoice(CoverageBitmap range, const PhrasePairList_ &list, PhraseSegmentation &seg) const;

public:
	uint getSentenceLength() const {
		return sentenceLength_;
	}

	template<class Iterator>
	void copyPhrasePairs(Iterator to_it) const {
		std::copy(phrasePairList_.begin(), phrasePairList_.end(), to_it);
	}

	const PhraseTable &getPhraseTable() const {
		return phraseTable_;
	}

	PhraseSegmentation proposeSegmentation() const;
	PhraseSegmentation proposeSegmentation(const CoverageBitmap &range) const;
	const AnchoredPhrasePair &proposeAlternativeTranslation(const AnchoredPhrasePair &old) const;
	bool phrasesExist(const PhraseSegmentation& phraseSegmentation) const;
};

#endif

