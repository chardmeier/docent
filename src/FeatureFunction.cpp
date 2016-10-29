/*
 *  FeatureFunction.cpp
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

#include "FeatureFunction.h"
#include "PhraseTable.h"
#include "SearchStep.h"
#include "models/BleuModel.h"
#include "models/BracketingModel.h"
#include "models/ConsistencyQModelPhrase.h"
#include "models/ConsistencyQModelWord.h"
#include "models/CountingModels.h"
#include "models/GappyLanguageModel.h"
#include "models/GeometricDistortionModel.h"
#include "models/NgramModel.h"
#include "models/OvixModel.h"
#include "models/SemanticSimilarityModel.h"
#include "models/SemanticSpaceLanguageModel.h"
#include "models/SentenceLengthModel.h"
#include "models/SentenceParityModel.h"
#include "models/TypeTokenRateModel.h"
#include "models/WellFormednessModel.h"

boost::shared_ptr<FeatureFunction>
FeatureFunctionFactory::create(
	const std::string &type,
	const Parameters &params
) const {
	FeatureFunction *ff;

	if(type == "phrase-table")
		ff = new PhraseTable(params, random_);
	else if(type == "ngram-model")
		ff = NgramModelFactory::createNgramModel(params);
	else if(type == "geometric-distortion-model")
		ff = new GeometricDistortionModel(params);
	else if(type == "sentence-length-model")
		ff = new SentenceLengthModel(params);
	else if(type == "phrase-penalty")
		ff = createCountingFeatureFunction(PhrasePenaltyCounter());
	else if(type == "word-penalty")
		ff = createCountingFeatureFunction(WordPenaltyCounter());
	else if(type == "oov-penalty")
		ff = createCountingFeatureFunction(OOVPenaltyCounter());
	else if(type == "long-word-penalty")
		ff = createCountingFeatureFunction(LongWordCounter(params));
	else if(type == "semantic-space-language-model")
		ff = SemanticSpaceLanguageModelFactory::createSemanticSpaceLanguageModel(params);
	else if(type == "sentence-parity-model")
		ff = new SentenceParityModel(params);
	else if(type == "ovix")
		ff = new OvixModel(params);
	else if(type == "type-token")
		ff = new TypeTokenRateModel(params);
	else if(type == "consistency-q-model-phrase")
		ff = new ConsistencyQModelPhrase(params);
	else if(type == "consistency-q-model-word")
		ff = new ConsistencyQModelWord(params);
	else if(type == "bleu-model")
		ff = new BleuModel(params);
	else if(type == "bracketing-model")
		ff = new BracketingModel(params);
	else if(type == "well-formedness-model")
		ff = new WellFormednessModel(params);
	else if(type == "gappy-lm")
		ff = GappyLanguageModelFactory::createNgramModel(params);
	else if(type == "semantic-similarity-model")
		ff = new SemanticSimilarityModel(params);
	else
		BOOST_THROW_EXCEPTION(ConfigurationException());

	return boost::shared_ptr<FeatureFunction>(ff);
}
