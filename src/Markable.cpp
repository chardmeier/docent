/*
 *  Markable.cpp
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

#include "Markable.h"

#include "MMAXDocument.h"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include <DOM/SAX2DOM/SAX2DOM.hpp>
#include <SAX/helpers/CatchErrorHandler.hpp>

MarkableLevel::MarkableLevel(
	const MMAXDocument &mmax,
	const std::string &name,
	const std::string &file
) :	logger_("MMAXDocument"),
	name_(name)
{
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
	for(Arabica::DOM::Node<std::string>
		n = doc.getDocumentElement().getFirstChild();
		n != 0;
		n = n.getNextSibling()
	) {
		if(n.getNodeType() != Arabica::DOM::Node<std::string>::ELEMENT_NODE)
			continue;

		if(n.getNodeName() == "markable") {
			Arabica::DOM::Element<std::string> elem =
				static_cast<Arabica::DOM::Element<std::string> >(n);

			const std::string &span = elem.getAttribute("span");
			const std::string &level = elem.getAttribute("mmax_level");
			const std::string &id = elem.getAttribute("id");

			if(level != name) {
				LOG(logger_, error,
					file << ": Expected level " << name << ", found " << level
				);
				BOOST_THROW_EXCEPTION(FileFormatException() << err_info::Filename(file));
			}

			boost::smatch m;
			if(
				!boost::regex_match(span.begin(), span.end(), m, spxm)
				&& !boost::regex_match(span.begin(), span.end(), m, spx1)
			) {
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
				LOG(logger_, error,
					file << ": " << id << ": Word index beyond end of document."
				);
				BOOST_THROW_EXCEPTION(FileFormatException() << err_info::Filename(file));
			}

			if(start > end) {
				LOG(logger_, error,
					file << ": " << id << ": Invalid span (start > end)."
				);
				BOOST_THROW_EXCEPTION(FileFormatException() << err_info::Filename(file));
			}

			MMAXDocument::SentenceVector_::const_iterator
				endit = std::lower_bound(
					mmax.sentences_.begin(),
					mmax.sentences_.end(),
					start
				);
			if(*endit == start)
				++endit;
			MMAXDocument::SentenceVector_::const_iterator startit = endit - 1;

			if(end >= *endit) {
				LOG(logger_, error, file << ": " << id << ": Ignoring cross-sentence markable.");
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

const Markable &MarkableLevel::getUniqueMarkableForCoverage(
	uint sentence,
	const CoverageBitmap &coverage
) const {
	Markable tgt(sentence, coverage);
	MarkableVector_::const_iterator
		it = std::lower_bound(
			markables_.begin(),
			markables_.end(),
			tgt
		);

	if(it == markables_.end() || it->getSentence() != sentence || it->getCoverage() != coverage) {
		LOG(logger_, error, "Missing markable in sentence " << sentence <<
			", coverage " << coverage);
		BOOST_THROW_EXCEPTION(FileFormatException());
	}

	const Markable &ret = *it;

	++it;
	if(
		it != markables_.end()
		&& it->getSentence() == sentence
		&& it->getCoverage() == coverage
	) {
		LOG(logger_, error,
			"Multiple markables in sentence " << sentence << ", coverage " << coverage
		);
		BOOST_THROW_EXCEPTION(FileFormatException());
	}
	return ret;
}

std::vector<Markable> MarkableLevel::getMarkablesForCoverage(
	uint sentence,
	const CoverageBitmap &coverage
) const {
	std::vector<Markable> out;
	Markable tgt(sentence, coverage);
	MarkableVector_::const_iterator
		it = std::lower_bound(
			markables_.begin(),
			markables_.end(),
			tgt
		);

	CoverageBitmap::size_type lastidx = CoverageBitmap::npos;
	for(CoverageBitmap::size_type
		i = coverage.find_first();
		i != CoverageBitmap::npos;
		i = coverage.find_next(i)
	)
		lastidx = i;
	assert(lastidx != CoverageBitmap::npos);

	while(
		it != markables_.end()
		&& it->getSentence() == sentence
		&& it->getCoverage().find_first() <= lastidx
	) {
		if(coverage.intersects(it->getCoverage()))
			out.push_back(*it);
		++it;
	}

	return out;
}
