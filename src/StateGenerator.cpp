/*
 *  StateGenerator.cpp
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

#include "StateGenerator.h"

#include "BeamSearchAdapter.h"
#include "DecoderConfiguration.h"
#include "FeatureFunction.h"
#include "PhrasePair.h"
#include "PhrasePairCollection.h"
#include "PhraseTable.h"
#include "Random.h"
#include "SearchStep.h"

#include <algorithm>
#include <sstream>
#include <fstream>

#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/string.hpp>

struct MonotonicStateInitialiser
:	public StateInitialiser
{
	MonotonicStateInitialiser(
		const Parameters &params
	) {}
	virtual PhraseSegmentation initSegmentation(
		boost::shared_ptr<const PhrasePairCollection> phraseTranslations,
		const std::vector<Word> &sentence,
		int documentNumber,
		int sentenceNumber
	) const;
};


class BeamSearchStateInitialiser : public StateInitialiser {
private:
	Logger logger_;
	BeamSearchAdapter *beamSearchAdapter_;

public:
	BeamSearchStateInitialiser(const Parameters &params);
	virtual ~BeamSearchStateInitialiser();
	virtual PhraseSegmentation initSegmentation(
		boost::shared_ptr<const PhrasePairCollection> phraseTranslations,
		const std::vector<Word> &sentence,
		int documentNumber,
		int sentenceNumber
	) const;
};


class FileReadStateInitialiser : public StateInitialiser {
private:
	Logger logger_;
	std::vector<std::vector<PhraseSegmentation> > segmentations_;
public:
	FileReadStateInitialiser(
		const Parameters &params
	);
	virtual PhraseSegmentation initSegmentation(
		boost::shared_ptr<const PhrasePairCollection> phraseTranslations,
		const std::vector<Word> &sentence,
		int documentNumber,
		int sentenceNumber
	) const;
};


PhraseSegmentation
MonotonicStateInitialiser::initSegmentation(
	boost::shared_ptr<const PhrasePairCollection> phraseTranslations,
	const std::vector<Word> &sentence,
	int documentNumber,
	int sentenceNumber
) const {
	return phraseTranslations->proposeSegmentation();
}


BeamSearchStateInitialiser::BeamSearchStateInitialiser(
	const Parameters &params
) :	logger_("StateInitialiser")
{
	std::string filename  = params.get<std::string>("file");
	if(filename.empty()) {
		LOG(logger_, error, "Missing parameter 'file'.");
		BOOST_THROW_EXCEPTION(ConfigurationException());
	}
	beamSearchAdapter_ = new BeamSearchAdapter(filename);
}

BeamSearchStateInitialiser::~BeamSearchStateInitialiser()
{
	delete beamSearchAdapter_;
}

PhraseSegmentation
BeamSearchStateInitialiser::initSegmentation(
	boost::shared_ptr<const PhrasePairCollection> phraseTranslations,
	const std::vector<Word> &sentence,
	int documentNumber,
	int sentenceNumber
) const {
	return beamSearchAdapter_->search(
		phraseTranslations,
		sentence
	);
}


FileReadStateInitialiser::FileReadStateInitialiser(
	const Parameters &params
) :	logger_("StateInitialiser")
{
	// get file name from params
	std::string filename = params.get<std::string>("file");

	// open the archive
	std::ifstream ifs(filename.c_str());
	if (!ifs.good()) {
		LOG(logger_, error, "problem reading file "<<filename);
		BOOST_THROW_EXCEPTION(FileFormatException());
	}
	boost::archive::text_iarchive ia(ifs);

	// restore the schedule from the archive
	ia >> segmentations_;
}

PhraseSegmentation
FileReadStateInitialiser::initSegmentation(
	boost::shared_ptr<const PhrasePairCollection> phraseTranslations,
	const std::vector<Word> &sentence,
	int documentNumber,
	int sentenceNumber
) const {
	PhraseSegmentation phraseSegmentation = segmentations_[documentNumber][sentenceNumber];
	//Check that all phrases in the phraseSegmentation exist in phraseTranslations
	if (!phraseTranslations->phrasesExist(phraseSegmentation)) {
		//!checkPhrases(phraseTranslations, phraseSegmentation)) {
		LOG(logger_, error, "ERROR: A phrase from the saved state does not exist in phrase table, make sure that the same phrase table is used as when saving the state");
		BOOST_THROW_EXCEPTION(ConfigurationException());
	}

	return phraseSegmentation;
}


StateGenerator::StateGenerator(
	const std::string &initMethod,
	const Parameters &params,
	Random(random)
) :	logger_("StateGenerator"),
	random_(random)
{
	if(initMethod == "monotonic")
		initialiser_ = new MonotonicStateInitialiser(params);
	else if(initMethod == "beam-search")
		initialiser_ = new BeamSearchStateInitialiser(params);
	else if(initMethod == "saved-state")
		initialiser_ = new FileReadStateInitialiser(params);
	else {
		LOG(logger_, error, "Unknown initialisation method: " << initMethod);
		BOOST_THROW_EXCEPTION(ConfigurationException());
	}
}

StateGenerator::~StateGenerator() {
	delete initialiser_;
}

void StateGenerator::addOperation(
	Float weight,
	const std::string &type,
	const Parameters &params
) {
	if(type == "change-phrase-translation")
		operations_.push_back(new ChangePhraseTranslationOperation(params));
	else if(type == "permute-phrases")
		operations_.push_back(new PermutePhrasesOperation(params));
	else if(type == "linearise-phrases")
		operations_.push_back(new LinearisePhrasesOperation(params));
	else if(type == "swap-phrases")
		operations_.push_back(new SwapPhrasesOperation(params));
	else if(type == "move-phrases")
		operations_.push_back(new MovePhrasesOperation(params));
	else if(type == "resegment")
		operations_.push_back(new ResegmentOperation(params));
	else {
		LOG(logger_, error, "Unknown operation: " << type);
		BOOST_THROW_EXCEPTION(ConfigurationException());
	}

	if(!cumulativeOperationDistribution_.empty())
		weight += cumulativeOperationDistribution_.back();
	cumulativeOperationDistribution_.push_back(weight);
}

/**
 * Returns NULL if 100 consecutive operations have returned NULL.
 */
SearchStep
*StateGenerator::createSearchStep(
	const DocumentState &doc
) const {
	SearchStep *nextStep = NULL;
	uint failed = 0;
	for(;;) {
		uint next_op = random_.drawFromCumulativeDistribution(cumulativeOperationDistribution_);
		LOG(logger_, debug,
			"Next operation: " << operations_[next_op].getDescription()
			<< "; failed so far: " << failed
		);
		nextStep = operations_[next_op].createSearchStep(doc);

		// NULL just indicates that our operator wasn't able to produce a reasonable set of changes
		// for some reason.
		if(nextStep == NULL) {
			++failed;
			if(failed >= 100) {
				LOG(logger_, debug, "Too many failed search steps in document #" << doc.getDocNumber());
				return NULL;
			}
			continue;
		}

		if(nextStep->getModifications().empty()) {
			delete nextStep;
			continue;
		}

		return nextStep;
	}
}
