/*
 *  Markable.h
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

#ifndef docent_Markable_h
#define docent_Markable_h

#include "Docent.h"

#include <boost/foreach.hpp>

class Markable {
	friend bool operator<(const Markable &m1, const Markable &m2);

private:
	uint sentence_;
	CoverageBitmap coverage_;
	typedef std::pair<std::string,std::string> AttributePair_;
	std::vector<AttributePair_> attributes_;
	std::vector<Word> words_;

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		ar & BOOST_SERIALIZATION_NVP(sentence_);
		ar & BOOST_SERIALIZATION_NVP(coverage_);
		ar & BOOST_SERIALIZATION_NVP(attributes_);
		ar & BOOST_SERIALIZATION_NVP(words_);
	}
	Markable() {} // for unpacking

public:
	template<class InputIterator>
	Markable(
		uint sentence,
		const CoverageBitmap &cov,
		InputIterator words_begin,
		InputIterator words_end
	) :	sentence_(sentence),
		coverage_(cov),
		words_(words_begin, words_end)
	{}

	Markable(  // for comparisons
		uint sentence,
		const CoverageBitmap &cov
	) :	sentence_(sentence),
		coverage_(cov)
	{}

	uint getSentence() const {
		return sentence_;
	}

	const CoverageBitmap &getCoverage() const {
		return coverage_;
	}

	const std::vector<Word> &getWords() const {
		return words_;
	}

	void setAttribute(
		const std::string &attr,
		const std::string &value
	) {
		BOOST_FOREACH(AttributePair_ &ap, attributes_)
			if(ap.first == attr) {
				ap.second = value;
				return;
			}

		attributes_.push_back(std::make_pair(attr, value));
	}

	const std::string &getAttribute(const std::string &attr) const {
		static const std::string EMPTY_STRING = "";

		BOOST_FOREACH(const AttributePair_ &ap, attributes_)
			if(ap.first == attr)
				return ap.second;

		return EMPTY_STRING;
	}
};

inline bool operator<(
	const Markable &m1,
	const Markable &m2
) {
	return std::make_pair(m1.sentence_, m1.coverage_) < std::make_pair(m2.sentence_, m2.coverage_);
}

class MMAXDocument;

class MarkableLevel {
private:
	typedef std::vector<Markable> MarkableVector_;
	typedef boost::unordered_map<std::string,uint> MarkableIDMap_;

	Logger logger_;

	std::string name_;
	MarkableVector_ markables_;
	MarkableIDMap_ idmap_;

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		ar & BOOST_SERIALIZATION_NVP(name_);
		ar & BOOST_SERIALIZATION_NVP(markables_);
	}
	MarkableLevel() : logger_("MarkableLevel") {} // for unpacking

public:
	typedef MarkableVector_::const_iterator const_iterator;

	MarkableLevel(
		const MMAXDocument &mmax,
		const std::string &name,
		const std::string &file
	);

	const std::string &getName() const {
		return name_;
	}

	const_iterator begin() const {
		return markables_.begin();
	}

	const_iterator end() const {
		return markables_.end();
	}

	const Markable *getMarkableByID(const std::string &markableID) const {
		MarkableIDMap_::const_iterator it = idmap_.find(markableID);
		if(it == idmap_.end())
			return NULL;
		else
			return &markables_[it->second];
	}

	const Markable &getUniqueMarkableForCoverage(uint sentence, const CoverageBitmap &coverage) const;
	std::vector<Markable> getMarkablesForCoverage(uint sentence, const CoverageBitmap &coverage) const;
};

#endif