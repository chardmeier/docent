/*
 *  DecoderConfiguration.h
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

#ifndef docent_DecoderConfiguration_h
#define docent_DecoderConfiguration_h

#include "Docent.h"
#include "Random.h"

#include <iostream>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>

#include <DOM/Element.hpp>

class BeamSearchAdapter;
class FeatureFunctionInstantiation;
class PhraseTable;
class SearchAlgorithm;
class StateGenerator;

class DecoderConfiguration {
public:
	typedef boost::ptr_vector<FeatureFunctionInstantiation> FeatureFunctionList;

private:
	mutable Logger logger_;
	Random random_;
	
	boost::shared_ptr<const PhraseTable> phraseTable_;
	
	FeatureFunctionList featureFunctions_;
	std::vector<Float> featureWeights_;
	uint nscores_;

	StateGenerator *stateGenerator_;
	SearchAlgorithm *search_;

	void setupRandomGenerator(Arabica::DOM::Node<std::string> n);
	void setupStateGenerator(Arabica::DOM::Node<std::string> n);
	void setupSearch(Arabica::DOM::Node<std::string> n);
	void setupModels(Arabica::DOM::Node<std::string> n);
	void setupWeights(Arabica::DOM::Node<std::string> n);

public:
	DecoderConfiguration(const std::string &filename);
	~DecoderConfiguration();

	Random getRandom() const {
		return random_;
	}
	
	const PhraseTable &getPhraseTable() const {
		return *phraseTable_;
	}
	
	const FeatureFunctionList &getFeatureFunctions() const {
		return featureFunctions_;
	}
	
	const std::vector<Float> &getFeatureWeights() const {
		return featureWeights_;
	}
	
	uint getTotalNumberOfScores() const {
		return nscores_;
	}
	
	const StateGenerator &getStateGenerator() const {
		return *stateGenerator_;
	}

	const SearchAlgorithm &getSearchAlgorithm() const {
		return *search_;
	}
};

class Parameters {
private:
	mutable Logger logger_;

	const Arabica::DOM::Node<std::string> parentNode_;

	bool getString(const std::string &name, std::string &outstr) const {
		for(Arabica::DOM::Node<std::string> c = parentNode_.getFirstChild(); c != 0; c = c.getNextSibling()) {
			if(c.getNodeType() != Arabica::DOM::Node<std::string>::ELEMENT_NODE || c.getNodeName() != "p")
				continue;
			Arabica::DOM::Element<std::string> pnode = static_cast<Arabica::DOM::Element<std::string> >(c);
			std::string pname = pnode.getAttribute("name");
			if(pname == "") {
				LOG(logger_, error, "Lacking required attribute 'name' on p element.");
				BOOST_THROW_EXCEPTION(ConfigurationException());
			}
			if(pname == name) {
				for(Arabica::DOM::Node<std::string> v = pnode.getFirstChild(); v != 0; v = v.getNextSibling())
					if(v.getNodeType() == Arabica::DOM::Node<std::string>::TEXT_NODE) {
						outstr = v.getNodeValue();
						return true;
					}
				LOG(logger_, error, "No value for parameter " << name << ".");
			}
		}

		return false;
	}

	boost::optional<bool> getBool(const std::string &name) const {
		std::string str;
		if(getString(name, str)) {
			if(str == "1")
				return boost::optional<bool>(true);
			else if(str == "0")
				return boost::optional<bool>(false);

			boost::algorithm::to_lower(str);
			if(str == "true")
				return boost::optional<bool>(true);
			else if(str == "false")
				return boost::optional<bool>(false);

			BOOST_THROW_EXCEPTION(ConfigurationException() << err_info::Parameter(name));
		} else
			return boost::optional<bool>();
	}

public:
	Parameters(Logger &logger, const Arabica::DOM::Node<std::string> parent) :
		logger_(logger), parentNode_(parent) {}

	template<typename T>
	T get(const std::string &name) const {
		std::string str;
		if(getString(name, str))
			return boost::lexical_cast<T>(str);
		else
			BOOST_THROW_EXCEPTION(ParameterNotFoundException() << err_info::Parameter(name));
	}

	template<typename T>
	T get(const std::string &name, const T &defaultValue) const {
		std::string str;
		if(getString(name, str))
			return boost::lexical_cast<T>(str);
		else
			return defaultValue;
	}

};

template<>
inline bool Parameters::get<bool>(const std::string &name) const {
	boost::optional<bool> v = getBool(name);
	if(v) // check initialisation status
		return *v;
	else
		BOOST_THROW_EXCEPTION(ParameterNotFoundException() << err_info::Parameter(name));
}

template<>
inline bool Parameters::get<bool>(const std::string &name, const bool &defaultValue) const {
	boost::optional<bool> v = getBool(name);
	if(v) // check initialisation status
		return *v;
	else
		return defaultValue;
}

#endif

