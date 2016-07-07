/*
 *  NistXmlDocument.cpp
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

#include "NistXmlDocument.h"

#include "MMAXDocument.h"
#include "PlainTextDocument.h"

#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>

struct SegNodeFilter : public Arabica::DOM::Traversal::NodeFilter<std::string>
{
	Result acceptNode(const NodeT &node) const
	{
		if(node.getParentNode().getNodeName() == "seg")
			return FILTER_ACCEPT;
		return FILTER_SKIP;
	}
};


PlainTextDocument
NistXmlDocument::asPlainTextDocument() const
{
	std::vector<std::vector<Word> > txt;

	typedef Arabica::DOM::Traversal::DocumentTraversal<std::string> Traversal;

	Traversal dt = topnode_.getOwnerDocument().createDocumentTraversal();
	SegNodeFilter filter;
	Traversal::TreeWalkerT it = dt.createTreeWalker(
		topnode_,
		static_cast<unsigned long>(Arabica::DOM::Traversal::SHOW_TEXT),
		filter,
		true
	);

	for(;;) {
		Traversal::NodeT n = it.nextNode();
		if(n == 0)
			break;
		std::string seg = n.getNodeValue();
		boost::trim(seg);
		txt.push_back(std::vector<Word>());
		boost::split(txt.back(), seg, boost::is_any_of(" "));
	}

	return PlainTextDocument(txt);
}

boost::shared_ptr<const MMAXDocument>
NistXmlDocument::asMMAXDocument() const
{
	typedef Arabica::DOM::Traversal::DocumentTraversal<std::string> Traversal;

	Traversal dt = topnode_.getOwnerDocument().createDocumentTraversal();
	SegNodeFilter filter;
	Traversal::TreeWalkerT it = dt.createTreeWalker(
		topnode_,
		static_cast<unsigned long>(Arabica::DOM::Traversal::SHOW_TEXT),
		filter,
		true
	);

	boost::shared_ptr<MMAXDocument> mmax = boost::make_shared<MMAXDocument>();
	for(;;) {
		Traversal::NodeT n = it.nextNode();
		if(n == 0)
			break;
		std::string seg = n.getNodeValue();
		boost::trim(seg);
		std::vector<Word> snt;
		boost::split(snt, seg, boost::is_any_of(" "));
		mmax->addSentence(snt.begin(), snt.end());
	}

	return mmax;
}

void NistXmlDocument::setTranslation(const PlainTextDocument &doc)
{
	typedef Arabica::DOM::Traversal::DocumentTraversal<std::string> Traversal;

	Traversal dt = outnode_.getOwnerDocument().createDocumentTraversal();
	SegNodeFilter filter;
	Traversal::TreeWalkerT it = dt.createTreeWalker(
		outnode_,
		static_cast<unsigned long>(Arabica::DOM::Traversal::SHOW_TEXT),
		filter,
		true
	);

	uint i = 0;
	for(;;) {
		Traversal::NodeT n = it.nextNode();
		if(n == 0)
			break;
		std::ostringstream os;
		std::copy(
			doc.sentence_begin(i),
			doc.sentence_end(i),
			std::ostream_iterator<Word>(os, " ")
		);
		i++;
		std::string str = os.str();
		str.erase(str.end() - 1);
		n.setNodeValue(str);
	}
}

void NistXmlDocument::annotateDocument(const std::string &annot)
{
	typedef Arabica::DOM::Comment<std::string> Comment;
	typedef Arabica::DOM::Node<std::string> Node;

	Comment comm = outnode_.getOwnerDocument().createComment(" DOC " + annot + " ");

	if(outnode_.hasChildNodes()) {
		Node n1 = outnode_.getFirstChild();
		Node n2 = n1.getNextSibling();

		if(n2 != 0
			&& n2.getNodeType() == Node::COMMENT_NODE
			&& boost::starts_with(n2.getNodeValue(), " DOC ")
		) {
			outnode_.replaceChild(comm, n2);
		} else {
			if(n1.getNodeType() == Node::TEXT_NODE) {
				Node ws = n1.cloneNode(false);
				outnode_.insertBefore(ws, n1);
			}
			outnode_.insertBefore(comm, n1);
		}
	} else
		outnode_.appendChild(comm);
}

void NistXmlDocument::annotateSentence(uint sentno, const std::string &annot)
{
	typedef Arabica::DOM::Traversal::DocumentTraversal<std::string> Traversal;
	typedef Arabica::DOM::Comment<std::string> Comment;

	Traversal dt = outnode_.getOwnerDocument().createDocumentTraversal();
	SegNodeFilter filter;
	Traversal::TreeWalkerT it = dt.createTreeWalker(
		outnode_,
		static_cast<unsigned long>(Arabica::DOM::Traversal::SHOW_TEXT),
		filter,
		true
	);

	for(uint i = 0; i < sentno; i++)
		it.nextNode();

	Traversal::NodeT n = it.nextNode(); // the filter finds the next node inside the <seg> element
	assert(n != 0);
	n = n.getParentNode(); // get the <seg>

	Comment comm = n.getOwnerDocument().createComment(" SEG " + annot + " ");

	Traversal::NodeT p = n.getPreviousSibling();
	Traversal::NodeT txt;
	if(p != 0 && p.getNodeType() == Arabica::DOM::Node<std::string>::TEXT_NODE) {
		txt = p;
		p = p.getPreviousSibling();
	}

	if(p != 0
		&& p.getNodeType() == Arabica::DOM::Node<std::string>::COMMENT_NODE
		&& boost::starts_with(p.getNodeValue(), " SEG ")
	) {
		p.getParentNode().replaceChild(comm, p);
	} else {
		n.getParentNode().insertBefore(comm, n);
		if(txt != 0)
			n.getParentNode().insertBefore(txt.cloneNode(false), n);
	}
}
