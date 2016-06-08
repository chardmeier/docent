/*
 *  NistXmlRefset.cpp
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

#include "Docent.h"
#include "DocumentState.h"
#include "NistXmlDocument.h"
#include "NistXmlRefset.h"

#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#include <DOM/Document.hpp>
#include <DOM/io/Stream.hpp>
#include <DOM/SAX2DOM/SAX2DOM.hpp>
#include <DOM/Traversal/DocumentTraversal.hpp>
#include <SAX/helpers/CatchErrorHandler.hpp>
#include <XPath/XPath.hpp>

NistXmlRefset::NistXmlRefset(
	const std::string &file
)	: logger_("NistXmlRefset")
{
	Arabica::SAX2DOM::Parser<std::string> domParser;
	Arabica::SAX::InputSource<std::string> is(file);
	Arabica::SAX::CatchErrorHandler<std::string> errh;
	domParser.setErrorHandler(errh);
	domParser.parse(is);
	if(errh.errorsReported())
		LOG(logger_, error, errh.errors());

	Arabica::DOM::Document<std::string> doc = domParser.getDocument();
	if(doc == 0) {
		LOG(logger_, error, "Error parsing input file: " << file);
		BOOST_THROW_EXCEPTION(FileFormatException());
	}

	doc.getDocumentElement().normalize();

	Arabica::XPath::XPath<std::string> xp;

	Arabica::XPath::NodeSet<std::string> docnodes = xp
		.compile("/mteval/refset/doc")
		.evaluateAsNodeSet(doc.getDocumentElement());
	docnodes.to_document_order();
	BOOST_FOREACH(Arabica::DOM::Node<std::string> n, docnodes)
		documents_.push_back(boost::make_shared<NistXmlDocument>(n));
}
