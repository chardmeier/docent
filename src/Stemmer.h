/*
 *  Stemmer.h
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

#ifndef docent_Stemmer_h
#define docent_Stemmer_h

#include "Docent.h"

#include <functional>
#include <string>

#include <boost/utility.hpp>

#include "libstemmer.h"

class Stemmer : public std::unary_function<const std::string &,std::string>, boost::noncopyable {
private:
	struct sb_stemmer *stemmer_;

public:
	Stemmer(const std::string &algorithm, const std::string &encoding) {
		stemmer_ = sb_stemmer_new(algorithm.c_str(), encoding.c_str());
		if(stemmer_ == NULL) {
			Logger log("Stemmer");
			LOG(log, error, "Unable to create " << algorithm <<
				" stemmer for encoding " << encoding);
			exit(1);
		}
	}

	~Stemmer() {
		sb_stemmer_delete(stemmer_);
	}

	std::string operator()(const std::string &word) {
		const sb_symbol *stem = sb_stemmer_stem(stemmer_,
			reinterpret_cast<const sb_symbol *>(word.c_str()), word.length());
		if(stem == NULL)
			BOOST_THROW_EXCEPTION(std::bad_alloc());
		return std::string(reinterpret_cast<const char *>(stem),
			sb_stemmer_length(stemmer_));
	}
};

#endif
