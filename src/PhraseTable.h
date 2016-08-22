/*
 *  PhraseTable.h
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

#ifndef docent_PhraseTable_h
#define docent_PhraseTable_h

#include "Docent.h"

#include "FeatureFunction.h"
#include "PhrasePair.h"

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

class PhrasePairCollection;
class QueryEngine; // from ProbingPT

class PhraseTable : public FeatureFunction, boost::noncopyable {
private:
	Logger logger_;
	Random random_;
	std::string filename_;
	uint nscores_;
	uint maxPhraseLength_;
	uint annotationCount_;
	uint filterLimit_;
	uint filterScoreIndex_;
	QueryEngine *backend_;
	bool loadAlignments_;

	Scores scorePhraseSegmentation(const PhraseSegmentation &ps) const;

public:
	PhraseTable(const Parameters &params, Random random);
	virtual ~PhraseTable();

	virtual FeatureFunction::State *initDocument(
		const DocumentState &doc,
		Scores::iterator sbegin
	) const;

	virtual StateModifications *estimateScoreUpdate(
		const DocumentState &doc,
		const SearchStep &step,
		const State *state,
		Scores::const_iterator psbegin,
		Scores::iterator sbegin
	) const;
	virtual StateModifications *updateScore(
		const DocumentState &doc,
		const SearchStep &step,
		const State *state,
		StateModifications *estmods,
		Scores::const_iterator psbegin,
		Scores::iterator estbegin
	) const;

	virtual uint getNumberOfScores() const {
		return nscores_;
	}

	virtual void computeSentenceScores(
		const DocumentState &doc,
		uint sentno,
		Scores::iterator sbegin
	) const;

	boost::shared_ptr<const PhrasePairCollection> getPhrasesForSentence(
		const std::vector<Word> &sentence
	) const;

	bool operator==(const PhraseTable &o) const {
		return filename_ == o.filename_;
	}

	friend std::size_t hash_value(const PhraseTable &p) {
		boost::hash<std::string> hasher;
		return hasher(p.filename_);
	}
};

#endif
