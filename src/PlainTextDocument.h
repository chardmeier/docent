/*
 *  PlainTextDocument.h
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

#ifndef docent_PlainTextDocument_h
#define docent_PlainTextDocument_h

#include "Docent.h"

#include <boost/foreach.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>

#include <sstream>

class PlainTextDocument {
private:
	std::vector<std::vector<Word> > text_;

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		ar & BOOST_SERIALIZATION_NVP(text_);
	}

public:
	typedef std::vector<Word>::iterator word_iterator;
	typedef std::vector<Word>::const_iterator const_word_iterator;

	PlainTextDocument() {}

	PlainTextDocument(const std::vector<std::vector<Word> > &text) :
		text_(text.begin(), text.end()) {}

	uint getNumberOfSentences() const {
		return text_.size();
	}

	word_iterator sentence_begin(uint s) {
		return text_[s].begin();
	}

	word_iterator sentence_end(uint s) {
		return text_[s].end();
	}

	const_word_iterator sentence_begin(uint s) const {
		return text_[s].begin();
	}

	const_word_iterator sentence_end(uint s) const {
		return text_[s].end();
	}

	std::vector<PlainTextDocument> toSentenceDocuments() const {
		std::vector<PlainTextDocument> ret;
		ret.reserve(text_.size());
		BOOST_FOREACH(const std::vector<Word> &snt, text_) {
			std::vector<std::vector<Word> > onesnt(1, snt);
			ret.push_back(PlainTextDocument());
			ret.back().text_.swap(onesnt);
		}
		return ret;
	}
};

#endif
