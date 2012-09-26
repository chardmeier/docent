/*
 *  SemanticSpace.h
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

#ifndef docent_SemanticSpace_h
#define docent_SemanticSpace_h

#include "Docent.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iosfwd>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/unordered_map.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_sparse.hpp>

typedef boost::numeric::ublas::vector<Float> DenseVectorType;
typedef boost::numeric::ublas::compressed_vector<Float> SparseVectorType;

template<class WordVectorType>
class SemanticSpaceImplementation : boost::noncopyable {
public:
	typedef DenseVectorType DenseVector;
	typedef WordVectorType WordVector;

private:
	typedef boost::unordered_map<Word,WordVector *> VectorMap_;

	uint ndimensions_;
	VectorMap_ vectors_;

	SemanticSpaceImplementation(uint ndim) : ndimensions_(ndim) {}

	static SemanticSpaceImplementation<WordVectorType> *loadSparseText(std::istream &file);
	static SemanticSpaceImplementation<WordVectorType> *loadDenseText(std::istream &file);

public:
	static SemanticSpaceImplementation<WordVectorType> *load(const std::string &file);

	~SemanticSpaceImplementation();

	uint getDimensionality() const {
		return ndimensions_;
	}

	const WordVector *lookup(const Word &word) const {
		typename VectorMap_::const_iterator it = vectors_.find(word);
		if(it == vectors_.end())
			return NULL;
		else
			return it->second;
	}
};

typedef SemanticSpaceImplementation<DenseVectorType> SemanticSpace;

template<class WV>
SemanticSpaceImplementation<WV> *SemanticSpaceImplementation<WV>::load(const std::string &file) {
	SemanticSpaceImplementation<WV> *space;

	std::ifstream is(file.c_str());
	char hdr[4];
	is.read(hdr, 4);
	if(std::equal(hdr, hdr + 4, "\000s\0002"))
		space = SemanticSpaceImplementation<WV>::loadSparseText(is);
	else if(std::equal(hdr, hdr + 4, "\000s\0000"))
		space = SemanticSpaceImplementation<WV>::loadDenseText(is);
	else {
		Logger logger("SemanticSpace");
		LOG(logger, error, file << ": Unknown sspace file format.");
		BOOST_THROW_EXCEPTION(FileFormatException());
	}

	is.close();
	return space;
}

template<class WV>
SemanticSpaceImplementation<WV>::~SemanticSpaceImplementation() {
	BOOST_FOREACH(typename VectorMap_::value_type &e, vectors_)
		delete e.second;
}

// TODO: proper error handling
template<class WV>
SemanticSpaceImplementation<WV> *SemanticSpaceImplementation<WV>::loadSparseText(std::istream &is) {
	std::string line;

	std::getline(is, line);

	std::istringstream dims(line);
	uint nitems, ndims;
	dims >> nitems >> ndims;

	SemanticSpaceImplementation<WV> *space = new SemanticSpaceImplementation<WV>(ndims);

	while(getline(is, line)) {
		Word word;
		WV *vec = new WV(ndims);
		vec->clear();

		bool zero = true;
		std::istringstream item(line);
		getline(item, word, '|');
		for(;;) {
			std::string sindex, svalue;
			getline(item, sindex, ',');
			getline(item, svalue, ',');
			if(!item)
				break;
			uint index = boost::lexical_cast<uint>(sindex);
			Float value = boost::lexical_cast<Float>(svalue);
			if(value != Float(0)) {
				vec->insert_element(index, value);
				zero = false;
			}
		}

		// discard zero vectors (TODO: is this the right thing to do?)
		if(!zero)
			space->vectors_.insert(std::make_pair(word, vec));
		else
			delete vec;
	}

	return space;
}

// TODO: proper error handling
template<class WV>
SemanticSpaceImplementation<WV> *SemanticSpaceImplementation<WV>::loadDenseText(std::istream &is) {
	std::string line;

	std::getline(is, line);

	std::istringstream dims(line);
	uint nitems, ndims;
	dims >> nitems >> ndims;

	SemanticSpaceImplementation<WV> *space = new SemanticSpaceImplementation<WV>(ndims);

	while(getline(is, line)) {
		Word word;
		WV *vec = new WV(ndims);
		vec->clear();

		bool zero = true;
		std::istringstream item(line);
		getline(item, word, '|');
		for(uint i = 0; i < ndims; i++) {
			std::string svalue;
			getline(item, svalue, ' ');
			Float value = boost::lexical_cast<Float>(svalue);
			if(value != Float(0)) {
				vec->insert_element(i, value);
				zero = false;
			}
		}

		// discard zero vectors (TODO: is this the right thing to do?)
		if(!zero)
			space->vectors_.insert(std::make_pair(word, vec));
		else
			delete vec;
	}

	return space;
}
#endif
