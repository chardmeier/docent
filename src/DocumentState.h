/*
 *  DocumentState.h
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

#ifndef docent_DocumentState_h
#define docent_DocumentState_h

#include "Docent.h"
#include "DecoderConfiguration.h"
#include "FeatureFunction.h"
#include "PhrasePair.h"
#include "PlainTextDocument.h"
#include "Random.h"

#include <map>
#include <iosfwd>
#include <numeric>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/functional/hash.hpp>

class MMAXDocument;
class NistXmlDocument;
class PhrasePairCollection;
class PhraseTable;
class SearchStep;
class StateOperation;

typedef unsigned long DocumentGeneration;

class DocumentState {
	friend class StateOperation;
	friend std::ostream &operator<<(std::ostream &os, const DocumentState &doc);
	friend std::size_t hash_value(const DocumentState &state);

public:
	// the pair is (attempted moves, accepted moves)
	typedef std::map<const StateOperation *,std::pair<DocumentGeneration,DocumentGeneration> > MoveCounts;

private:
	Logger logger_;
	const DecoderConfiguration *configuration_;

	uint docNumber_;
	
	boost::shared_ptr<const MMAXDocument> inputdoc_;
	std::vector<PhraseSegmentation> sentences_;
	std::vector<boost::shared_ptr<const PhrasePairCollection> > phraseTranslations_;
	boost::shared_ptr<const std::vector<Float> > cumulativeSentenceLength_;
	Scores scores_;
	std::vector<FeatureFunction::State *> featureStates_;

	MoveCounts moveCount_;
	DocumentGeneration generation_;

	void init();
	void debugSentenceCoverage(const PhraseSegmentation &seg) const;

public:
	DocumentState(const DecoderConfiguration &config, const boost::shared_ptr<const MMAXDocument> &text, int docNumber);
	DocumentState(const DecoderConfiguration &config, const boost::shared_ptr<const NistXmlDocument> &text, int docNumber);
	DocumentState(const DocumentState &o);
	~DocumentState();
	DocumentState &operator=(const DocumentState &o);
	
	bool operator==(const DocumentState &o) const {
		return configuration_ == o.configuration_ && sentences_ == o.sentences_;
	}

	boost::shared_ptr<const MMAXDocument> getInputDocument() const {
		return inputdoc_;
	}

	uint getInputWordCount() const {
		return cumulativeSentenceLength_->back();
	}

	Float getInputSentenceLength(uint i) const {
		if(i == 0)
			return (*cumulativeSentenceLength_)[0];
		else
			return (*cumulativeSentenceLength_)[i] - (*cumulativeSentenceLength_)[i-1];
	}

	PlainTextDocument asPlainTextDocument() const;

	uint drawSentence(Random rnd) const {
		return rnd.drawFromCumulativeDistribution(*cumulativeSentenceLength_);
	}

	SearchStep *proposeSearchStep() const;
	void applyModifications(SearchStep *step);
	
	const std::vector<PhraseSegmentation> &getPhraseSegmentations() const {
		return sentences_;
	}
	
	const PhraseSegmentation &getPhraseSegmentation(uint sentno) const {
		return sentences_[sentno];
	}
	
	Scores computeSentenceScores(uint sentno) const; // debugging only!

	const Scores &getScores() const {
		return scores_;
	}
	
	Float getScore() const {
		return std::inner_product(scores_.begin(), scores_.end(),
			configuration_->getFeatureWeights().begin(), Float(0));
	}
	
	DocumentGeneration getGeneration() const {
		return generation_;
	}
	
	const DecoderConfiguration *getDecoderConfiguration() const {
		return configuration_;
	}

	void registerAttemptedMove(const SearchStep *step);
	const MoveCounts &getMoveCounts() const {
		return moveCount_;
	}

	void dumpFeatureFunctionStates() const;
};

std::ostream &operator<<(std::ostream &os, const DocumentState &doc);

inline std::size_t hash_value(const DocumentState &state) {
	std::size_t seed = 0;
	boost::hash_combine(seed, state.configuration_);
	boost::hash_combine(seed, state.sentences_);
	return seed;
}

#endif

