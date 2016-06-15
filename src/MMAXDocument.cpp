/*
 *  MMAXDocument.cpp
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

#include "MMAXDocument.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include <DOM/SAX2DOM/SAX2DOM.hpp>
#include <SAX/helpers/CatchErrorHandler.hpp>

MMAXDocument::MMAXDocument() : logger_("MMAXDocument"), nistxml_() {
	sentences_.push_back(0);
}

MMAXDocument::MMAXDocument(const std::string &file) :
		logger_("MMAXDocument"), nistxml_() {
	load(boost::filesystem::path(file));
}

MMAXDocument::MMAXDocument(const boost::filesystem::path &file) :
		logger_("MMAXDocument"), nistxml_() {
	load(file);
}

MMAXDocument::MMAXDocument(const boost::filesystem::path &file, const boost::shared_ptr<NistXmlDocument> &nistxml) :
		logger_("MMAXDocument"), nistxml_(nistxml) {
	load(file);
}

template<class Node>
inline std::string MMAXDocument::getTextValue(Node n) const {
	std::string out;
	for(Arabica::DOM::Node<std::string> c = n.getFirstChild(); c != 0; c = c.getNextSibling()) {
		if(c.getNodeType() == Node::TEXT_NODE) {
			out = c.getNodeValue();
			break;
		}
	}
	return out;
}

void MMAXDocument::load(const boost::filesystem::path &mmax) {
	boost::filesystem::path baseDirectory = mmax.parent_path();

	Arabica::SAX2DOM::Parser<std::string> domParser;
	Arabica::SAX::CatchErrorHandler<std::string> errh;
	domParser.setErrorHandler(errh);

	Arabica::SAX::InputSource<std::string> commonPaths((baseDirectory / "common_paths.xml").string());
	domParser.parse(commonPaths);

	Arabica::DOM::Document<std::string> doc = domParser.getDocument();
	if(doc == 0) {
		LOG(logger_, error, "Error parsing common_paths.xml");
		BOOST_THROW_EXCEPTION(FileFormatException());
	}

	boost::filesystem::path basedataPath, markablePath;
	boost::filesystem::path p;

	std::string mmaxstem = mmax.stem().string();

	doc.getDocumentElement().normalize();
	for(Arabica::DOM::Node<std::string> n = doc.getDocumentElement().getFirstChild(); n != 0;
			n = n.getNextSibling()) {
		if(n.getNodeType() != Arabica::DOM::Node<std::string>::ELEMENT_NODE)
			continue;

		if(n.getNodeName() == "basedata_path") {
			p = boost::replace_all_copy(getTextValue(n), "\\", "/");
			if(p.is_absolute())
				basedataPath = p;
			else
				basedataPath = baseDirectory / p;
		} else if(n.getNodeName() == "markable_path") {
			p = boost::replace_all_copy(getTextValue(n), "\\", "/");
			if(p.is_absolute())
				markablePath = p;
			else
				markablePath = baseDirectory / p;
		} else if(n.getNodeName() == "annotations") {
			for(Arabica::DOM::Node<std::string> l = n.getFirstChild(); l != 0;
					l = l.getNextSibling()) {
				if(l.getNodeType() == Arabica::DOM::Node<std::string>::ELEMENT_NODE &&
						l.getNodeName() == "level") {
					Arabica::DOM::Element<std::string> elem =
						static_cast<Arabica::DOM::Element<std::string> >(l);
					const std::string &name = elem.getAttribute("name");
					p = boost::algorithm::replace_first_copy(getTextValue(elem), "$", mmaxstem);
					if(p.is_relative())
						p = markablePath / p;
					LOG(logger_, debug, "Level " << name << " in " << p);
					levels_.insert(std::make_pair(name, std::make_pair(p.string(),
						static_cast<MarkableLevel *>(NULL))));
				}
			}
		}
	}

	loadBasedata(mmax, basedataPath);
	loadSentenceLevel("sentence");
}

MMAXDocument::MMAXDocument(const MMAXDocument &o) :
		logger_("MMAXDocument"),
		words_(o.words_), sentences_(o.sentences_) {
	BOOST_FOREACH(LevelMap_::value_type &v, o.levels_) {
		const MarkableLevel *lvl;
		if(v.second.second == NULL)
			lvl = NULL;
		else
			lvl = new MarkableLevel(*v.second.second);
		levels_.insert(std::make_pair(v.first, std::make_pair(v.second.first, lvl)));
	}
}

MMAXDocument &MMAXDocument::operator=(const MMAXDocument &o) {
	WordVector_ newwords(o.words_);
	SentenceVector_ newsentences(o.sentences_);

	LevelMap_ newlevels;
	BOOST_FOREACH(LevelMap_::value_type &v, o.levels_) {
		const MarkableLevel *lvl;
		if(v.second.second == NULL)
			lvl = NULL;
		else
			lvl = new MarkableLevel(*v.second.second);
		newlevels.insert(std::make_pair(v.first, std::make_pair(v.second.first, lvl)));
	}

	words_.swap(newwords);
	sentences_.swap(newsentences);
	levels_.swap(newlevels);

	return *this;
}

MMAXDocument::~MMAXDocument() {
	BOOST_FOREACH(LevelMap_::value_type &v, levels_)
		if(v.second.second != NULL)
			delete v.second.second;
}

void MMAXDocument::loadBasedata(const boost::filesystem::path &mmax, const boost::filesystem::path &basedataPath) {
	Arabica::SAX2DOM::Parser<std::string> domParser;
	Arabica::SAX::CatchErrorHandler<std::string> errh;
	domParser.setErrorHandler(errh);

	Arabica::SAX::InputSource<std::string> mmaxSource(mmax.string());
	domParser.parse(mmaxSource);

	Arabica::DOM::Document<std::string> doc = domParser.getDocument();
	if(doc == 0) {
		LOG(logger_, error, "Error parsing " << mmax);
		BOOST_THROW_EXCEPTION(FileFormatException());
	}

	boost::filesystem::path file;

	doc.getDocumentElement().normalize();
	for(Arabica::DOM::Node<std::string> n = doc.getDocumentElement().getFirstChild(); n != 0;
			n = n.getNextSibling()) {
		if(n.getNodeType() != Arabica::DOM::Node<std::string>::ELEMENT_NODE)
			continue;

		if(n.getNodeName() == "words") {
			file = getTextValue(n);
			break;
		}
	}

	if(file.empty()) {
		LOG(logger_, error, "No or empty <words> tag in .mmax file " << mmax);
		BOOST_THROW_EXCEPTION(FileFormatException() << err_info::Filename(mmax.string()));
	}

	if(file.is_relative())
		file = basedataPath / file;

	Arabica::SAX::InputSource<std::string> is(file.string());
	domParser.parse(is);

	doc = domParser.getDocument();
	if(doc == 0) {
		LOG(logger_, error, "Error parsing " << file);
		BOOST_THROW_EXCEPTION(FileFormatException());
	}

	uint widx = 1;
	doc.getDocumentElement().normalize();
	for(Arabica::DOM::Node<std::string> n = doc.getDocumentElement().getFirstChild(); n != 0;
			n = n.getNextSibling()) {
		if(n.getNodeType() != Arabica::DOM::Node<std::string>::ELEMENT_NODE)
			continue;

		if(n.getNodeName() == "word") {
			std::ostringstream os;
			os << "word_" << widx;

			Arabica::DOM::Element<std::string> elem =
				static_cast<Arabica::DOM::Element<std::string> >(n);
			const std::string &id = elem.getAttribute("id");

			if(id != os.str()) {
				LOG(logger_, error, file << ": Expected word " <<
					os.str() << ", found " << id);
				BOOST_THROW_EXCEPTION(FileFormatException());
			}

			widx++;
			words_.push_back(getTextValue(n));
		}
	}
}

void MMAXDocument::loadSentenceLevel(const std::string &sentenceLevel) {
	Arabica::SAX2DOM::Parser<std::string> domParser;
	Arabica::SAX::CatchErrorHandler<std::string> errh;
	domParser.setErrorHandler(errh);

	LevelMap_::const_iterator it = levels_.find(sentenceLevel);
	if(it == levels_.end()) {
		LOG(logger_, error, "Sentence level " << sentenceLevel << " undefined.");
		BOOST_THROW_EXCEPTION(FileFormatException());
	}
	const std::string &file = it->second.first;

	Arabica::SAX::InputSource<std::string> is(file);
	domParser.parse(is);

	Arabica::DOM::Document<std::string> doc = domParser.getDocument();
	if(doc == 0) {
		LOG(logger_, error, "Error parsing " << file);
		BOOST_THROW_EXCEPTION(FileFormatException() << err_info::Filename(file));
	}

	uint sidx = 0;
	uint nextstart = 1;
	boost::regex spx1("word_([0-9]+)");
	boost::regex spxm("word_([0-9]+)\\.\\.word_([0-9]+)");
	doc.getDocumentElement().normalize();
	for(Arabica::DOM::Node<std::string> n = doc.getDocumentElement().getFirstChild(); n != 0;
			n = n.getNextSibling()) {
		if(n.getNodeType() != Arabica::DOM::Node<std::string>::ELEMENT_NODE)
			continue;

		if(n.getNodeName() == "markable") {
			Arabica::DOM::Element<std::string> elem =
				static_cast<Arabica::DOM::Element<std::string> >(n);

			const std::string &level = elem.getAttribute("mmax_level");
			const std::string &orderid = elem.getAttribute("orderid");
			const std::string &span = elem.getAttribute("span");

			if(level != sentenceLevel) {
				LOG(logger_, error, file << ": Expected level " << sentenceLevel <<
					", found " << level);
				BOOST_THROW_EXCEPTION(FileFormatException() << err_info::Filename(file));
			}

			if(orderid != boost::lexical_cast<std::string>(sidx)) {
				LOG(logger_, error, file << ": Expected sentence " << sidx <<
					", found " << orderid);
				BOOST_THROW_EXCEPTION(FileFormatException() << err_info::Filename(file));
			}

			boost::smatch m;
			if(!boost::regex_match(span.begin(), span.end(), m, spxm) &&
					!boost::regex_match(span.begin(), span.end(), m, spx1)) {
				LOG(logger_, error, file << ": Can't parse sentence span: " << span);
				BOOST_THROW_EXCEPTION(FileFormatException() << err_info::Filename(file));
			}

			if(m.size() == 2) {
				uint idx = boost::lexical_cast<uint>(m[1]);
				if(idx != nextstart) {
					LOG(logger_, error, file <<
						": Sentence " << sidx << " starts at word_" << idx <<
						" instead of word_" << nextstart << " as expected.");
					BOOST_THROW_EXCEPTION(FileFormatException() << err_info::Filename(file));
				}
				nextstart = idx + 1;
				sentences_.push_back(idx - 1);
			} else {
				uint start = boost::lexical_cast<uint>(m[1]);
				uint end = boost::lexical_cast<uint>(m[2]);
				if(start != nextstart) {
					LOG(logger_, error, file <<
						": Sentence " << sidx << " starts at word_" << start <<
						" instead of word_" << nextstart << " as expected.");
					BOOST_THROW_EXCEPTION(FileFormatException() << err_info::Filename(file));
				}
				nextstart = end + 1;
				sentences_.push_back(start - 1);
			}

			sidx++;
		}
	}

	sentences_.push_back(nextstart - 1);
}

const MarkableLevel &MMAXDocument::getMarkableLevel(const std::string &name) const {
	LevelMap_::iterator it = levels_.find(name);

	if(it == levels_.end()) {
		LOG(logger_, error, "Unknown markable level (not in common_paths.xml): " << name);
		BOOST_THROW_EXCEPTION(FileFormatException());
	}

	if(it->second.second == NULL)
		it->second.second = new MarkableLevel(*this, name, it->second.first);

	return *it->second.second;
}

MarkableLevel::MarkableLevel(const MMAXDocument &mmax, const std::string &name, const std::string &file)
		: logger_("MMAXDocument"), name_(name) {
	Arabica::SAX2DOM::Parser<std::string> domParser;
	Arabica::SAX::CatchErrorHandler<std::string> errh;
	domParser.setErrorHandler(errh);

	Arabica::SAX::InputSource<std::string> is(file);
	domParser.parse(is);

	Arabica::DOM::Document<std::string> doc = domParser.getDocument();
	if(doc == 0) {
		LOG(logger_, error, "Error parsing " << file);
		BOOST_THROW_EXCEPTION(FileFormatException());
	}

	boost::regex spx1("word_([0-9]+)");
	boost::regex spxm("word_([0-9]+)\\.\\.word_([0-9]+)");
	doc.getDocumentElement().normalize();
	for(Arabica::DOM::Node<std::string> n = doc.getDocumentElement().getFirstChild(); n != 0;
			n = n.getNextSibling()) {
		if(n.getNodeType() != Arabica::DOM::Node<std::string>::ELEMENT_NODE)
			continue;

		if(n.getNodeName() == "markable") {
			Arabica::DOM::Element<std::string> elem =
				static_cast<Arabica::DOM::Element<std::string> >(n);

			const std::string &span = elem.getAttribute("span");
			const std::string &level = elem.getAttribute("mmax_level");
			const std::string &id = elem.getAttribute("id");

			if(level != name) {
				LOG(logger_, error, file << ": Expected level " << name <<
					", found " << level);
				BOOST_THROW_EXCEPTION(FileFormatException() << err_info::Filename(file));
			}

			boost::smatch m;
			if(!boost::regex_match(span.begin(), span.end(), m, spxm) &&
					!boost::regex_match(span.begin(), span.end(), m, spx1)) {
				LOG(logger_, error, file << ": Can't parse sentence span: " << span);
				BOOST_THROW_EXCEPTION(FileFormatException() << err_info::Filename(file));
			}

			uint start, end;
			if(m.size() == 2)
				start = end = boost::lexical_cast<uint>(m[1]);
			else {
				start = boost::lexical_cast<uint>(m[1]);
				end = boost::lexical_cast<uint>(m[2]);
			}

			if(start == 0 || end == 0) {
				LOG(logger_, error, file << ": " << id <<
					": Word numbering must be 1-based.");
				BOOST_THROW_EXCEPTION(FileFormatException() << err_info::Filename(file));
			} else {
				// convert to 0-based counting
				start--;
				end--;
			}

			if(start >= mmax.sentences_.back() || end >= mmax.sentences_.back()) {
				LOG(logger_, error, file << ": " << id <<
					": Word index beyond end of document.");
				BOOST_THROW_EXCEPTION(FileFormatException() << err_info::Filename(file));
			}

			if(start > end) {
				LOG(logger_, error, file << ": " << id <<
					": Invalid span (start > end).");
				BOOST_THROW_EXCEPTION(FileFormatException() << err_info::Filename(file));
			}

			MMAXDocument::SentenceVector_::const_iterator endit =
				std::lower_bound(mmax.sentences_.begin(), mmax.sentences_.end(), start);
			if(*endit == start)
				++endit;
			MMAXDocument::SentenceVector_::const_iterator startit = endit - 1;

			if(end >= *endit) {
				LOG(logger_, error, file << ": " << id <<
					": Ignoring cross-sentence markable.");
				continue;
			}

			uint sidx = startit - mmax.sentences_.begin();
			CoverageBitmap cov(*endit - *startit);
			for(uint i = start - *startit; i <= end - *startit; i++)
				cov.set(i);
			assert(cov.any());

			std::vector<Word>::const_iterator wbegin, wend;
			wbegin = mmax.words_.begin() + start;
			wend = mmax.words_.begin() + end + 1;
			Markable mrk(sidx, cov, wbegin, wend);
			Arabica::DOM::NamedNodeMap<std::string> atts = n.getAttributes();
			for(uint i = 0; i < atts.getLength(); i++) {
				Arabica::DOM::Node<std::string> a = atts.item(i);
				if(a.getNodeType() != Arabica::DOM::Node<std::string>::ATTRIBUTE_NODE)
					continue;
				if(a.getNodeName() == "mmax_level" || a.getNodeName() == "span")
					continue;
				mrk.setAttribute(a.getNodeName(), a.getNodeValue());
			}

			markables_.push_back(mrk);
		}
	}

	std::sort(markables_.begin(), markables_.end());

	idmap_.rehash((5 * markables_.size()) / 3);
	for(uint i = 0; i < markables_.size(); i++)
		idmap_.insert(std::make_pair(markables_[i].getAttribute("id"), i));
}

const Markable &MarkableLevel::getUniqueMarkableForCoverage(uint sentence, const CoverageBitmap &coverage) const {
	Markable tgt(sentence, coverage);
	MarkableVector_::const_iterator it = std::lower_bound(markables_.begin(), markables_.end(), tgt);

	if(it == markables_.end() || it->getSentence() != sentence || it->getCoverage() != coverage) {
		LOG(logger_, error, "Missing markable in sentence " << sentence <<
			", coverage " << coverage);
		BOOST_THROW_EXCEPTION(FileFormatException());
	}

	const Markable &ret = *it;

	++it;
	if(it != markables_.end() && it->getSentence() == sentence && it->getCoverage() == coverage) {
		LOG(logger_, error, "Multiple markables in sentence " << sentence <<
			", coverage " << coverage);
		BOOST_THROW_EXCEPTION(FileFormatException());
	}

	return ret;
}

std::vector<Markable> MarkableLevel::getMarkablesForCoverage(uint sentence, const CoverageBitmap &coverage) const {
	std::vector<Markable> out;
	Markable tgt(sentence, coverage);
	MarkableVector_::const_iterator it = std::lower_bound(markables_.begin(), markables_.end(), tgt);

	CoverageBitmap::size_type lastidx = CoverageBitmap::npos;
	for(CoverageBitmap::size_type i = coverage.find_first(); i != CoverageBitmap::npos;
			i = coverage.find_next(i))
		lastidx = i;
	assert(lastidx != CoverageBitmap::npos);

	while(it != markables_.end() && it->getSentence() == sentence &&
			it->getCoverage().find_first() <= lastidx) {
		if(coverage.intersects(it->getCoverage()))
			out.push_back(*it);
		++it;
	}

	return out;
}

MMAXTestset::MMAXTestset(const std::string &directory, const std::string &nistxml) :
		logger_("MMAXTestset"), nistxml_(nistxml) {
	namespace fs = boost::filesystem;

	fs::path dir(directory);
	for(fs::directory_iterator it = fs::directory_iterator(dir); it != fs::directory_iterator(); ++it)
		if(it->path().extension() == ".mmax")
			mmaxFiles_.push_back(it->path());

	std::sort(mmaxFiles_.begin(), mmaxFiles_.end());

	if(mmaxFiles_.size() != nistxml_.size()) {
		LOG(logger_, error, "MMAX test set has " << mmaxFiles_.size()
			<< " documents, NIST file has " << nistxml_.size());
		BOOST_THROW_EXCEPTION(FileFormatException());
	}
}
