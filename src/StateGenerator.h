/*
 *  StateGenerator.h
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

#ifndef docent_StateGenerator_h
#define docent_StateGenerator_h

#include "Docent.h"
#include "DocumentState.h"
#include "StateOperation.h"

#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

class PhrasePairCollection;
class SearchStep;

class StateGenerator {
private:
	Logger logger_;
	Random random_;
	boost::ptr_vector<StateOperation> operations_;
	std::vector<Float> cumulativeOperationDistribution_;
	StateInitialiser *initialiser_;

public:
	StateGenerator(
		const std::string &initMethod,
		const Parameters &params,
		Random random
	);
	~StateGenerator();
	void addOperation(
		Float weight,
		const std::string &type,
		const Parameters &params
	);

	PhraseSegmentation initSegmentation(
		boost::shared_ptr<const PhrasePairCollection> phraseTranslations,
		const std::vector<Word> &sentence,
		int documentNumber,
		int sentenceNumber
	) const {
		return initialiser_->initSegmentation(
			phraseTranslations,
			sentence,
			documentNumber,
			sentenceNumber
		);
	}

	/**
	 * Returns NULL if 100 consecutive operations have returned NULL.
	 * In that case the document should be considered unchangeable by the available operations,
	 * and the search algorithm should pass over the document in coming iterations.
	 */
	SearchStep *createSearchStep(
		const DocumentState &doc
	) const;
};

#endif
