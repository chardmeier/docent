/*
 *  DecoderConfiguration.cpp
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
#include "DecoderConfiguration.h"
#include "PhraseTable.h"
#include "Random.h"
#include "SearchAlgorithm.h"
#include "StateGenerator.h"

#include <iterator>
#include <limits>
#include <map>
#include <numeric>
#include <vector>

#include <boost/dynamic_bitset.hpp>
#include <boost/lexical_cast.hpp>

#include <DOM/SAX2DOM/SAX2DOM.hpp>
#include <SAX/helpers/CatchErrorHandler.hpp>

DecoderConfiguration::DecoderConfiguration(const std::string &file) :
		logger_("DecoderConfiguration"), random_(Random::create()) {
	Arabica::SAX2DOM::Parser<std::string> domParser;
	Arabica::SAX::InputSource<std::string> is(file);
	Arabica::SAX::CatchErrorHandler<std::string> errh;
	domParser.setErrorHandler(errh);
	domParser.parse(is);
	if(errh.errorsReported())
		LOG(logger_, error, errh.errors());

	Arabica::DOM::Document<std::string> doc = domParser.getDocument();
	if(doc == 0) {
		LOG(logger_, error, "Error parsing configuration file: " << file);
		BOOST_THROW_EXCEPTION(ConfigurationException());
	}
	
	doc.getDocumentElement().normalize();
	for(Arabica::DOM::Node<std::string> n = doc.getDocumentElement().getFirstChild(); n != 0; n = n.getNextSibling()) {
		if(n.getNodeType() != Arabica::DOM::Node<std::string>::ELEMENT_NODE)
			continue;

		if(n.getNodeName() == "random")
			setupRandomGenerator(n);
		else if(n.getNodeName() == "state-generator")
			setupStateGenerator(n);
		else if(n.getNodeName() == "search")
			setupSearch(n);
		else if(n.getNodeName() == "models")
			setupModels(n);
		else if(n.getNodeName() == "weights")
			setupWeights(n);
		else
			LOG(logger_, error, "Unknown configuration section: " << n.getNodeName());
	}
}

DecoderConfiguration::~DecoderConfiguration() {
	delete stateGenerator_;
	delete search_;
}

void DecoderConfiguration::setupRandomGenerator(Arabica::DOM::Node<std::string> n) {
	for(Arabica::DOM::Node<std::string> c = n.getFirstChild(); c != 0; c = c.getNextSibling()) {
		if(c.getNodeType() == Arabica::DOM::Node<std::string>::TEXT_NODE) {
			uint seed = boost::lexical_cast<uint>(c.getNodeValue());
			random_.seed(seed);
			return;
		}
	}

	random_.seed();
}

void DecoderConfiguration::setupStateGenerator(Arabica::DOM::Node<std::string> n) {
	Arabica::DOM::Node<std::string> c = n.getFirstChild();

	while(c != 0 && (c.getNodeType() != Arabica::DOM::Node<std::string>::ELEMENT_NODE || c.getNodeName() != "initial-state"))
		c = c.getNextSibling();

	if(c == 0) {
		LOG(logger_, error, "Element 'initial-state' not found.");
		BOOST_THROW_EXCEPTION(ConfigurationException());
	}

	Arabica::DOM::Element<std::string> istateNode = static_cast<Arabica::DOM::Element<std::string> >(c);
	std::string type = istateNode.getAttribute("type");
	if(type == "") {
		LOG(logger_, error, "Lacking required attribute 'type' on initial-state element.");
		BOOST_THROW_EXCEPTION(ConfigurationException());
	}

	stateGenerator_ = new StateGenerator(type, Parameters(logger_, istateNode), random_);

	for(c = n.getFirstChild(); c != 0; c = c.getNextSibling()) {
		if(c.getNodeType() != Arabica::DOM::Node<std::string>::ELEMENT_NODE || c.getNodeName() != "operation")
			continue;
		Arabica::DOM::Element<std::string> onode = static_cast<Arabica::DOM::Element<std::string> >(c);
		type = onode.getAttribute("type");
		std::string weight = onode.getAttribute("weight");
		if(type == "" || weight == "") {
			LOG(logger_, error, "Lacking required attribute on operation element.");
			BOOST_THROW_EXCEPTION(ConfigurationException());
		}
		
		stateGenerator_->addOperation(boost::lexical_cast<Float>(weight), type, Parameters(logger_, onode));
	}
}

void DecoderConfiguration::setupSearch(Arabica::DOM::Node<std::string> n) {
	Arabica::DOM::Element<std::string> e = static_cast<Arabica::DOM::Element<std::string> >(n);
	std::string algo = e.getAttribute("algorithm");
	search_ = SearchAlgorithm::createSearchAlgorithm(algo, *this, Parameters(logger_, e));
}

void DecoderConfiguration::setupModels(Arabica::DOM::Node<std::string> n) {
	FeatureFunctionFactory ffFactory(random_);
	uint scoreIndex = 0;
	std::set<std::string> ids;
	for(Arabica::DOM::Node<std::string> c = n.getFirstChild(); c != 0; c = c.getNextSibling()) {
		if(c.getNodeType() != Arabica::DOM::Node<std::string>::ELEMENT_NODE || c.getNodeName() != "model")
			continue;
		
		Arabica::DOM::Element<std::string> mnode = static_cast<Arabica::DOM::Element<std::string> >(c);
		std::string type = mnode.getAttribute("type");
		std::string id = mnode.getAttribute("id");
		if(type == "" || id == "") {
			LOG(logger_, error, "Lacking required attribute on model element.");
			BOOST_THROW_EXCEPTION(ConfigurationException());
		}
		if(!ids.insert(id).second) {
			LOG(logger_, error, "Double specification for model " << id);
			BOOST_THROW_EXCEPTION(ConfigurationException());
		}
		boost::shared_ptr<FeatureFunction> ff_impl = ffFactory.create(type, Parameters(logger_, mnode));
		FeatureFunctionInstantiation *ff = new FeatureFunctionInstantiation(id, scoreIndex, ff_impl);
		featureFunctions_.push_back(ff);
		scoreIndex += ff->getNumberOfScores();
		
		// TODO: This is messy.
		if(type == "phrase-table" && !phraseTable_) {
			phraseTable_ = boost::dynamic_pointer_cast<const PhraseTable>(ff_impl);
			assert(phraseTable_);
		}
	}
	nscores_ = scoreIndex;
}

void DecoderConfiguration::setupWeights(Arabica::DOM::Node<std::string> n) {
	boost::dynamic_bitset<> coveredWeights(getTotalNumberOfScores());
	featureWeights_.resize(getTotalNumberOfScores());
	for(Arabica::DOM::Node<std::string> c = n.getFirstChild(); c != 0; c = c.getNextSibling()) {
		if(c.getNodeType() != Arabica::DOM::Node<std::string>::ELEMENT_NODE || c.getNodeName() != "weight")
			continue;
		Arabica::DOM::Element<std::string> wnode = static_cast<Arabica::DOM::Element<std::string> >(c);
		std::string id = wnode.getAttribute("model");
		if(id == "") {
			LOG(logger_, error, "Lacking required attribute 'model' on weight element.");
			BOOST_THROW_EXCEPTION(ConfigurationException());
		}
		FeatureFunctionList::const_iterator ffit;
		uint scoreIndex = 0;
		for(ffit = featureFunctions_.begin(); ffit != featureFunctions_.end(); ++ffit) {
			if(ffit->getId() == id)
				break;
			scoreIndex += ffit->getNumberOfScores();
		}
		if(ffit == featureFunctions_.end()) {
			LOG(logger_, error, "Ignoring weight for non-existent model " << id);
			continue;
		}
		std::string s_order = wnode.getAttribute("score");
		if(s_order == "") {
			if(ffit->getNumberOfScores() > 1) {
				LOG(logger_, error, "Lacking attribute 'score' on weight for model " << id);
				BOOST_THROW_EXCEPTION(ConfigurationException());
			} else
				s_order = "0";
		}
		uint order = boost::lexical_cast<uint>(s_order);
		if(order >= ffit->getNumberOfScores()) {
			LOG(logger_, error, "Found inexistent weight " << order << " for model " << id <<
				", which only requires " << ffit->getNumberOfScores() << " weights.");
			BOOST_THROW_EXCEPTION(ConfigurationException());
		}
		if(coveredWeights.test(scoreIndex + order)) {
			LOG(logger_, error, "Weight " << order << " for model " << id << " already specified.");
			BOOST_THROW_EXCEPTION(ConfigurationException());
		}
		coveredWeights.set(scoreIndex + order);
		Arabica::DOM::Node<std::string> w;
		for(w = c.getFirstChild(); w != 0; w = w.getNextSibling()) {
			if(w.getNodeType() == Arabica::DOM::Node<std::string>::TEXT_NODE) {
				featureWeights_[scoreIndex + order] = boost::lexical_cast<Float>(w.getNodeValue());
				break;
			}
		}
		if(w == 0) {
			LOG(logger_, error, "Weight " << order << " for model " << id << " not found.");
			BOOST_THROW_EXCEPTION(ConfigurationException());
		}
	}
	if(coveredWeights.count() != coveredWeights.size()) {
		LOG(logger_, error, "Insufficient number of weights: " << coveredWeights);
		BOOST_THROW_EXCEPTION(ConfigurationException());
	}
}

