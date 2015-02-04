/*
 *  DocumentState.cpp
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

#include "Docent.h"
#include "DocumentState.h"
#include "DecoderConfiguration.h"
#include "FeatureFunction.h"
#include "MMAXDocument.h"
#include "PhrasePair.h"
#include "PhrasePairCollection.h"
#include "PhraseTable.h"
#include "PlainTextDocument.h"
#include "Random.h"
#include "SearchStep.h"
#include "StateGenerator.h"

#include <algorithm>
#include <iterator>

#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>
#include <boost/lambda/if.hpp>
#include <boost/scoped_ptr.hpp>

DocumentState::DocumentState(const DecoderConfiguration &config, const boost::shared_ptr<const MMAXDocument> &inputdoc, int docNumber) :
		logger_("DocumentState"),
		configuration_(&config), docNumber_(docNumber), inputdoc_(inputdoc),
		scores_(configuration_->getTotalNumberOfScores()), generation_(0) {
	init();
}

DocumentState::DocumentState(const DecoderConfiguration &config, const boost::shared_ptr<const NistXmlDocument> &inputdoc, int docNumber) :
		logger_("DocumentState"),
		configuration_(&config), docNumber_(docNumber), inputdoc_(inputdoc->asMMAXDocument()),
		scores_(configuration_->getTotalNumberOfScores()), generation_(0) {
	init();
}

void DocumentState::init() {
	sentences_.reserve(inputdoc_->getNumberOfSentences());
	phraseTranslations_.reserve(inputdoc_->getNumberOfSentences());
	std::vector<Float> *sntlen = new std::vector<Float>();
	sntlen->reserve(inputdoc_->getNumberOfSentences());
	Float cumlength = Float(0);
	const PhraseTable &ttable = configuration_->getPhraseTable();
	const StateGenerator &generator = configuration_->getStateGenerator();
	boost::scoped_ptr<const StateInitialiser> initialiser(
		generator.createDocumentInitialiser(docNumber_, inputdoc_));
	for(uint i = 0; i < inputdoc_->getNumberOfSentences(); i++) {
		std::vector<Word> snt(inputdoc_->sentence_begin(i), inputdoc_->sentence_end(i));
		phraseTranslations_.push_back(ttable.getPhrasesForSentence(snt));
		PhraseSegmentation ps = initialiser->initSegmentation(phraseTranslations_[i], snt, i);
		sentences_.push_back(ps);
		cumlength += snt.size();
		sntlen->push_back(cumlength);
	}
	cumulativeSentenceLength_.reset(sntlen);

	Scores::iterator scoreit = scores_.begin();
	const DecoderConfiguration::FeatureFunctionList &ff = configuration_->getFeatureFunctions();
	for(DecoderConfiguration::FeatureFunctionList::const_iterator it = ff.begin(); it != ff.end();
			scoreit += it->getNumberOfScores(), ++it)
		featureStates_.push_back(it->initDocument(*this, scoreit));
}

DocumentState::DocumentState(const DocumentState &o)
	: logger_("DocumentState"),
	  configuration_(o.configuration_), docNumber_(o.docNumber_), inputdoc_(o.inputdoc_),
	  sentences_(o.sentences_), phraseTranslations_(o.phraseTranslations_),
	  cumulativeSentenceLength_(o.cumulativeSentenceLength_), scores_(o.scores_),
	  generation_(o.generation_) {
	using namespace boost::lambda;
	std::transform(o.featureStates_.begin(), o.featureStates_.end(), std::back_inserter(featureStates_),
		if_then_else_return(_1, bind(&FeatureFunction::State::clone, _1),
			static_cast<FeatureFunction::State *>(NULL)));
}

DocumentState &DocumentState::operator=(const DocumentState &o) {
	using namespace boost::lambda;
	
	if(&o == this)
		return *this;

	configuration_ = o.configuration_;
	inputdoc_ = o.inputdoc_;
	sentences_ = o.sentences_;
	docNumber_ = o.docNumber_;
	phraseTranslations_ = o.phraseTranslations_;
	cumulativeSentenceLength_ = o.cumulativeSentenceLength_;
	scores_ = o.scores_;
	generation_ = o.generation_;
	std::vector<FeatureFunction::State *> ffs;
	std::transform(o.featureStates_.begin(), o.featureStates_.end(), std::back_inserter(ffs),
		if_then_else_return(_1, bind(&FeatureFunction::State::clone, _1),
			static_cast<FeatureFunction::State *>(NULL)));
	uint onf = featureStates_.size();
	if(ffs.size() > featureStates_.size())
		featureStates_.resize(ffs.size());
	std::swap_ranges(ffs.begin(), ffs.end(), featureStates_.begin());
	std::for_each(featureStates_.begin() + onf, featureStates_.end(), bind(delete_ptr(), _1));
	featureStates_.resize(ffs.size());
	std::for_each(ffs.begin(), ffs.end(), bind(delete_ptr(), _1));
	return *this;
}

DocumentState::~DocumentState() {
	using namespace boost::lambda;
	std::for_each(featureStates_.begin(), featureStates_.end(), bind(delete_ptr(), _1));
}

Scores DocumentState::computeSentenceScores(uint i) const {
	Scores s(configuration_->getTotalNumberOfScores());
	Scores::iterator scoreit = s.begin();
	const DecoderConfiguration::FeatureFunctionList &ff = configuration_->getFeatureFunctions();
	for(DecoderConfiguration::FeatureFunctionList::const_iterator it = ff.begin(); it != ff.end();
			scoreit += it->getNumberOfScores(), ++it)
		it->computeSentenceScores(*this, i, scoreit);
	return s;
}

void DocumentState::registerAttemptedMove(const SearchStep *step) {
	moveCount_[step->getOperation()].first++;
}

void DocumentState::applyModifications(SearchStep *step) {
	assert(&step->getDocumentState() == this && step->getDocumentGeneration() == generation_);

	moveCount_[step->getOperation()].second++;

	std::vector<SearchStep::Modification> &mods = step->getModifications();
	for(std::vector<SearchStep::Modification>::iterator it = mods.begin(); it != mods.end(); ++it) {
		uint sentno = it->sentno;
		PhraseSegmentation &sent = sentences_[sentno];
		PhraseSegmentation::const_iterator c_from_it = it->from_it;
		PhraseSegmentation::const_iterator c_to_it = it->to_it;
		PhraseSegmentation &proposal = it->proposal;

		PhraseSegmentation::iterator from_it = sent.begin();
		std::advance(from_it, std::distance<PhraseSegmentation::const_iterator>(from_it, c_from_it));
		PhraseSegmentation::iterator to_it = from_it;
		std::advance(to_it, std::distance<PhraseSegmentation::const_iterator>(to_it, c_to_it));

		sent.erase(from_it, to_it);
		sent.splice(to_it, proposal);
	}
	scores_ = step->getScores();

/*
	BOOST_FOREACH(const PhraseSegmentation &sent, sentences_)
		debugSentenceCoverage(sent);
*/

	const DecoderConfiguration::FeatureFunctionList &ffs = configuration_->getFeatureFunctions();
	const std::vector<FeatureFunction::StateModifications *> &smods = step->getStateModifications();
	for(uint i = 0; i < ffs.size(); i++)
		if(smods[i] != NULL)
			featureStates_[i] = ffs[i].applyStateModifications(featureStates_[i], smods[i]);

	delete step;
	
	generation_++;
}

PlainTextDocument DocumentState::asPlainTextDocument() const {
	std::vector<std::vector<Word> > out(sentences_.size());

	for(uint i = 0; i < sentences_.size(); i++)
		BOOST_FOREACH(const AnchoredPhrasePair &app, sentences_[i]) {
			const std::vector<Word> &phr = app.second.get().getTargetPhrase().get();
			std::copy(phr.begin(), phr.end(), std::back_inserter(out[i]));
		}

	return PlainTextDocument(out);
}

void DocumentState::dumpFeatureFunctionStates() const {
	const DecoderConfiguration::FeatureFunctionList &ffs = configuration_->getFeatureFunctions();
	for(uint i = 0; i < ffs.size(); i++)
		ffs[i].dumpFeatureFunctionState(*this, featureStates_[i]);
}

void DocumentState::debugSentenceCoverage(const PhraseSegmentation &seg) const {
	CoverageBitmap bm(seg.front().first.size());
	BOOST_FOREACH(const AnchoredPhrasePair &app, seg) {
		if((bm & app.first).any()) {
			LOG(logger_, error, "OVERLAP\n" << (bm & app.first) << '\n' << *this);
			abort();
		}
		bm |= app.first;
	}
	if(bm.count() != bm.size()) {
		LOG(logger_, error, "INCOMPLETE COVERAGE\n" << bm << '\n' << *this);
		abort();
	}
}

std::ostream &operator<<(std::ostream &os, const DocumentState &doc) {
	os << "DOCUMENT STATE:\n";
	std::copy(doc.sentences_.begin(), doc.sentences_.end(), std::ostream_iterator<PhraseSegmentation>(os));
	os << doc.scores_ << " * " << doc.configuration_->getFeatureWeights() << " = " << doc.getScore() << '\n';
	return os;
}
