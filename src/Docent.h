/*
 *  Docent.h
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

#ifndef docent_Docent_h
#define docent_Docent_h

#define HAVE_BOOST // for moses. We always have boost.

#include <algorithm>
#include <list>
#include <string>

#include <boost/dynamic_bitset.hpp>
#include <boost/exception/all.hpp>
#include <boost/flyweight.hpp>
#include <boost/flyweight/no_tracking.hpp>
#include <boost/functional/hash.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/version.hpp>

#if BOOST_VERSION < 104400
#error "Docent requires at least Boost version 1.44.0. Is your compiler picking up the right header files?"
#endif

typedef unsigned int uint;
typedef float Float;

typedef std::string Word;
typedef std::vector<Word> PhraseData;
typedef boost::flyweight<PhraseData,boost::flyweights::no_tracking> Phrase;

typedef std::vector<Float> Scores;

typedef boost::dynamic_bitset<> CoverageBitmap;

BOOST_SERIALIZATION_SPLIT_FREE(CoverageBitmap)
namespace boost {
namespace serialization {
	template<class Archive>
	void save(Archive &ar, const CoverageBitmap &bm, const uint version) {
		CoverageBitmap::size_type size = bm.size();
		std::vector<CoverageBitmap::block_type> blocks(bm.num_blocks());
		boost::to_block_range(bm, blocks.begin());

		ar << size;
		ar << const_cast<const std::vector<CoverageBitmap::block_type>& >(blocks);
		//ar << blocks;
	}

	template<class Archive>
	void load(Archive &ar, CoverageBitmap &bm, const uint version) {
		CoverageBitmap::size_type size;
		std::vector<CoverageBitmap::block_type> blocks;

		ar >> size;
		ar >> blocks;

		bm.resize(size);
		boost::from_block_range(blocks.begin(), blocks.end(), bm);
	}

	template<class Archive, class T>
	void save(Archive & ar,
			  const boost::flyweights::flyweight<T> & t,
			  const unsigned int file_version){
		ar & t.get(); // save the flyweight content
	}

	template<class Archive, class T>
	void load(Archive & ar,
					 boost::flyweights::flyweight<T> & t,
					 const unsigned int file_version){
		T p;
		ar & p; // get the flyweight content
		t = p;
	}

	template<class Archive, class T>
	void serialize(Archive & ar,
						  boost::flyweights::flyweight<T> &t,
						  const unsigned int file_version){
		boost::serialization::split_free(ar, t, file_version);
	}	

}} //end namespace boost serialization

const Float IMPOSSIBLE_SCORE = -1e30f;

inline Scores &operator+=(Scores &a, const Scores &b) {
	assert(a.size() == b.size());
	std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::plus<Float>());
	return a;
}

inline Scores &operator-=(Scores &a, const Scores &b) {
	assert(a.size() == b.size());
	std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::minus<Float>());
	return a;
}

inline Scores operator+(const Scores &a, const Scores &b) {
	Scores ret(a);
	ret += b;
	return ret;
}

inline Scores operator-(const Scores &a, const Scores &b) {
	Scores ret(a);
	ret -= b;
	return ret;
}

inline std::ostream &operator<<(std::ostream &os, const Scores &s) {
	os << "[";
	if(!s.empty()) {
		std::copy(s.begin(), s.end()-1, std::ostream_iterator<Float>(os, ", "));
		os << s.back();
	}
	os << "]";
	return os;
}

template<class T>
struct PointerEqualsTo : public std::binary_function<T,T,bool> {
	bool operator()(T a, T b) const {
		return *a == *b;
	}
};

namespace boost { // hijack namespace to make sure it's found where it's needed
inline std::size_t hash_value(const CoverageBitmap &bm) {
	std::vector<CoverageBitmap::block_type> blocks(bm.num_blocks());
	boost::to_block_range(bm, blocks.begin());
	return boost::hash_range(blocks.begin(), blocks.end());
}
}

/*** Exceptions ***/

struct DocentException : virtual std::exception, virtual boost::exception {};
struct ConfigurationException : virtual DocentException {};
struct ParameterNotFoundException : virtual ConfigurationException {};
struct FileFormatException : virtual DocentException {};

namespace err_info {
	typedef boost::error_info<struct tag_Filename,std::string> Filename;
	typedef boost::error_info<struct tag_Parameter,std::string> Parameter;
}

#include "Logger.h"

#endif

