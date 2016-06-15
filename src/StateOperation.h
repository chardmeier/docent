/*
 *  StateOperation.h
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

#ifndef docent_StateOperation_h
#define docent_StateOperation_h

#include "Docent.h"
#include "DocumentState.h"

#include <vector>

#include <boost/shared_ptr.hpp>

class FeatureFunction;
class SearchStep;

class StateOperation {
protected:
	const std::vector<boost::shared_ptr<const PhrasePairCollection> > &getPhraseTranslations(const DocumentState &doc) const {
		return doc.phraseTranslations_;
	}

	const std::vector<FeatureFunction::State *> &getFeatureStates(const DocumentState &doc) const {
		return doc.featureStates_;
	}

public:
	virtual ~StateOperation() {}
	virtual std::string getDescription() const = 0;
	virtual SearchStep *createSearchStep(const DocumentState &doc) const = 0;
};

struct StateInitialiser {
	virtual ~StateInitialiser() {}
	virtual PhraseSegmentation initSegmentation(
		boost::shared_ptr<const PhrasePairCollection> phraseTranslations,
		const std::vector<Word> &sentence, int documentNumber, int sentenceNumber) const = 0;
};

struct ChangePhraseTranslationOperation : public StateOperation {
private:
	mutable Logger logger_;

public:
	ChangePhraseTranslationOperation(
		const Parameters &params
	) : logger_("ChangePhraseTranslationOperation") {}

	virtual std::string getDescription() const {
		return "ChangePhraseTranslation";
	}

	virtual SearchStep *createSearchStep(const DocumentState &doc) const;
};


class PermutePhrasesOperation : public StateOperation {
private:
	mutable Logger logger_;
	Float phrasePermutationDecay_;

public:
	PermutePhrasesOperation(const Parameters &params);

	virtual std::string getDescription() const {
		std::ostringstream os;
		os << "PermutePhrases(decay=" << phrasePermutationDecay_ << ")";
		return os.str();
	}

	virtual SearchStep *createSearchStep(const DocumentState &doc) const;
};


class LinearisePhrasesOperation : public StateOperation {
private:
	mutable Logger logger_;
	Float phraseLinearisationDecay_;

public:
	LinearisePhrasesOperation(const Parameters &params);

	virtual std::string getDescription() const {
		std::ostringstream os;
		os << "LinearisePhrases(decay=" << phraseLinearisationDecay_ << ")";
		return os.str();
	}

	virtual SearchStep *createSearchStep(const DocumentState &doc) const;
};


class SwapPhrasesOperation : public StateOperation {
private:
	mutable Logger logger_;
	Float swapDistanceDecay_;

public:
	SwapPhrasesOperation(const Parameters &params);

	virtual std::string getDescription() const {
		std::ostringstream os;
		os << "SwapPhrases(decay=" << swapDistanceDecay_ << ")";
		return os.str();
	}

	virtual SearchStep *createSearchStep(const DocumentState &doc) const;
};


class MovePhrasesOperation : public StateOperation {
private:
	mutable Logger logger_;
	Float blockSizeDecay_;
	Float rightMovePreference_;
	Float rightDistanceDecay_;
	Float leftDistanceDecay_;

public:
	MovePhrasesOperation(const Parameters &params);

	virtual std::string getDescription() const {
		std::ostringstream os;
		os << "MovePhrases(block-size-decay=" << blockSizeDecay_ <<
			",right-move-preference=" << rightMovePreference_ <<
			",right-distance-decay=" << rightDistanceDecay_ <<
			",left-distance-decay=" << leftDistanceDecay_ << ')';
		return os.str();
	}

	virtual SearchStep *createSearchStep(const DocumentState &doc) const;
};


class ResegmentOperation : public StateOperation {
private:
	mutable Logger logger_;
	Float phraseResegmentationDecay_;

public:
	ResegmentOperation(const Parameters &params);

	virtual std::string getDescription() const {
		std::ostringstream os;
		os << "Resegment(decay=" << phraseResegmentationDecay_ << ")";
		return os.str();
	}

	virtual SearchStep *createSearchStep(const DocumentState &doc) const;
};

#endif
