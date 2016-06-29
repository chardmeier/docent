/*
 *  MMAXDocument.h
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

#ifndef docent_MMAXDocument_h
#define docent_MMAXDocument_h

#include "Docent.h"
#include "Markable.h"
#include "NistXmlDocument.h"
#include "PlainTextDocument.h"

#include <boost/filesystem/path.hpp>
#include <boost/iterator_adaptors.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/unordered_map.hpp>

class MMAXDocument {
	friend class MarkableLevel;

private:
	Logger logger_;

	typedef std::map<
		std::string,
		std::pair<std::string, const MarkableLevel*>
	> LevelMap_;
	typedef std::vector<uint> SentenceVector_;
	typedef std::vector<Word> WordVector_;

	// mutable to permit on-demand loading of markable levels
	mutable LevelMap_ levels_;
	WordVector_ words_;
	SentenceVector_ sentences_;

	boost::shared_ptr<NistXmlDocument> nistxml_;

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		ar & BOOST_SERIALIZATION_NVP(levels_);
		ar & BOOST_SERIALIZATION_NVP(words_);
		ar & BOOST_SERIALIZATION_NVP(sentences_);
	}

	template<class Node>
	std::string getTextValue(Node n) const;

	void load(const boost::filesystem::path &mmax);
	void loadBasedata(
		const boost::filesystem::path &mmax,
		const boost::filesystem::path &basedataPath
	);
	void loadSentenceLevel(const std::string &sentenceLevel);

public:
	typedef std::vector<Word>::iterator word_iterator;
	typedef std::vector<Word>::const_iterator const_word_iterator;

	MMAXDocument();
	MMAXDocument(const MMAXDocument &o);
	MMAXDocument(const std::string &file);
	MMAXDocument(const boost::filesystem::path &file);
	MMAXDocument(
		const boost::filesystem::path &file,
		const boost::shared_ptr<NistXmlDocument> &nistxml
	);

	MMAXDocument &operator=(const MMAXDocument &o);

	~MMAXDocument();

	template<class Iterator>
	void addSentence(Iterator from, Iterator to);

	uint getNumberOfSentences() const {
		return sentences_.size() - 1;
	}

	word_iterator sentence_begin(uint s) {
		assert(s < sentences_.size() - 1);
		return words_.begin() + sentences_[s];
	}

	word_iterator sentence_end(uint s) {
		assert(s < sentences_.size() - 1);
		return words_.begin() + sentences_[s + 1];
	}

	const_word_iterator sentence_begin(uint s) const {
		assert(s < sentences_.size() - 1);
		return words_.begin() + sentences_[s];
	}

	const_word_iterator sentence_end(uint s) const {
		assert(s < sentences_.size() - 1);
		return words_.begin() + sentences_[s + 1];
	}

	uint sentence_size(uint s) const {
		assert(s < sentences_.size() - 1);
		return sentences_[s + 1] - sentences_[s];
	}

	template<class OutputIterator>
	void copyWordsForSentence(uint s, OutputIterator oit) const {
		assert(s < sentences_.size() - 1);
		std::copy(
			words_.begin() + sentences_[s],
			words_.begin() + sentences_[s + 1] - 1,
			oit
		);
	}

	const MarkableLevel &getMarkableLevel(const std::string &name) const;

	void setTranslation(const PlainTextDocument &translation) {
		assert(nistxml_);
		nistxml_->setTranslation(translation);
	}

	void annotateDocument(const std::string &annot) {
		assert(nistxml_);
		nistxml_->annotateDocument(annot);
	}

	void annotateSentence(uint sentno, const std::string &annot) {
		assert(nistxml_);
		nistxml_->annotateSentence(sentno, annot);
	}
};

template<class Iterator>
inline void MMAXDocument::addSentence(Iterator from, Iterator to) {
	words_.insert(words_.end(), from, to);
	sentences_.push_back(words_.size());
}

#endif
