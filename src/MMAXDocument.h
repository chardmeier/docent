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
#include "NistXmlTestset.h"

#include <boost/filesystem/path.hpp>
#include <boost/foreach.hpp>
#include <boost/iterator_adaptors.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/unordered_map.hpp>

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
	Markable(uint sentence, const CoverageBitmap &cov, InputIterator words_begin, InputIterator words_end) :
		sentence_(sentence), coverage_(cov), words_(words_begin, words_end) {}

	Markable(uint sentence, const CoverageBitmap &cov) :
		sentence_(sentence), coverage_(cov) {} // for comparisons

	uint getSentence() const {
		return sentence_;
	}

	const CoverageBitmap &getCoverage() const {
		return coverage_;
	}

	const std::vector<Word> &getWords() const {
		return words_;
	}

	void setAttribute(const std::string &attr, const std::string &value) {
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

inline bool operator<(const Markable &m1, const Markable &m2) {
	return std::make_pair(m1.sentence_, m1.coverage_) < std::make_pair(m2.sentence_, m2.coverage_);
}

class MMAXDocument;

class MarkableLevel {
private:
	typedef std::vector<Markable> MarkableVector_;
	typedef boost::unordered_map<std::string,uint> MarkableIDMap_;

	mutable Logger logger_;

	std::string name_;
	MarkableVector_ markables_;
	MarkableIDMap_ idmap_;

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		ar & BOOST_SERIALIZATION_NVP(name_);
		ar & BOOST_SERIALIZATION_NVP(markables_);
	}
	MarkableLevel() {} // for unpacking

public:
	typedef MarkableVector_::const_iterator const_iterator;

	MarkableLevel(const MMAXDocument &mmax, const std::string &name, const std::string &file);

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

class MMAXDocument {
	friend class MarkableLevel;

private:
	mutable Logger logger_;

	typedef std::map<std::string,std::pair<std::string,const MarkableLevel *> > LevelMap_;
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
	void loadBasedata(const boost::filesystem::path &mmax, const boost::filesystem::path &basedataPath);
	void loadSentenceLevel(const std::string &sentenceLevel);

public:
	typedef std::vector<Word>::iterator word_iterator;
	typedef std::vector<Word>::const_iterator const_word_iterator;

	MMAXDocument();
	MMAXDocument(const MMAXDocument &o);
	MMAXDocument(const std::string &file);
	MMAXDocument(const boost::filesystem::path &file);
	MMAXDocument(const boost::filesystem::path &file, const boost::shared_ptr<NistXmlDocument> &nistxml);

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
		std::copy(words_.begin() + sentences_[s], words_.begin() + sentences_[s + 1] - 1, oit);
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

template<class Value,class NistIterator,class MMAXIterator>
class MMAXTestsetIterator : public boost::iterator_facade<MMAXTestsetIterator<Value,NistIterator,MMAXIterator>,
		boost::shared_ptr<Value>,boost::single_pass_traversal_tag,boost::shared_ptr<Value> > {
private:
	friend class MMAXTestset;
	friend class boost::iterator_core_access;
	template<class,class,class> friend class MMAXTestsetIterator;

	NistIterator nistit_;
	MMAXIterator mmaxit_;

	MMAXTestsetIterator(NistIterator nistit, MMAXIterator mmaxit)
		: nistit_(nistit), mmaxit_(mmaxit) {}

	template<class OtherValue,class OtherNistIterator,class OtherMMAXIterator>
	MMAXTestsetIterator(const MMAXTestsetIterator<OtherValue,OtherNistIterator,OtherMMAXIterator> &o)
		: nistit_(o.nistit_), mmaxit_(o.mmaxit_) {}

	boost::shared_ptr<Value> dereference() const {
		return boost::make_shared<Value>(*mmaxit_, *nistit_);
	}

	template<class OtherValue,class OtherNistIterator,class OtherMMAXIterator>
	bool equal(const MMAXTestsetIterator<OtherValue,OtherNistIterator,OtherMMAXIterator> &o) const {
		return mmaxit_ == o.mmaxit_ && nistit_ == o.nistit_;
	}

	void increment() {
		++mmaxit_;
		++nistit_;
	}
};

class MMAXTestset {
private:
	typedef std::vector<boost::filesystem::path> MMAXFileVector_;

	mutable Logger logger_;

	NistXmlTestset nistxml_;
	MMAXFileVector_ mmaxFiles_;

public:
	typedef uint size_type;
	typedef boost::shared_ptr<MMAXDocument> value_type;
	typedef boost::shared_ptr<const MMAXDocument> const_value_type;

	typedef MMAXTestsetIterator<MMAXDocument,NistXmlTestset::iterator,
					MMAXFileVector_::const_iterator> iterator;
	typedef MMAXTestsetIterator<const MMAXDocument,NistXmlTestset::const_iterator,
					MMAXFileVector_::const_iterator> const_iterator;

	MMAXTestset(const std::string &directory, const std::string &nistxml);

	iterator begin() {
		return iterator(nistxml_.begin(), mmaxFiles_.begin());
	}

	iterator end() {
		return iterator(nistxml_.end(), mmaxFiles_.end());
	}

	const_iterator begin() const {
		return const_iterator(nistxml_.begin(), mmaxFiles_.begin());
	}

	const_iterator end() const {
		return const_iterator(nistxml_.end(), mmaxFiles_.end());
	}

	uint size() const {
		return mmaxFiles_.size();
	}

	void outputTranslation(std::ostream &os) const {
		nistxml_.outputTranslation(os);
	}
};

#endif
