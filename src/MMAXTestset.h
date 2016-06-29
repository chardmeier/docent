/*
 *  MMAXTestset.h
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

#ifndef docent_MMAXTestset_h
#define docent_MMAXTestset_h

#include "Docent.h"
#include "Markable.h"
#include "MMAXDocument.h"
#include "NistXmlTestset.h"

#include <boost/make_shared.hpp>

template<class Value, class NistIterator, class MMAXIterator>
class MMAXTestsetIterator
:	public boost::iterator_facade<
		MMAXTestsetIterator<Value, NistIterator, MMAXIterator>,
		boost::shared_ptr<Value>,
		boost::single_pass_traversal_tag,
		boost::shared_ptr<Value>
	> {
private:
	friend class MMAXTestset;
	friend class boost::iterator_core_access;
	template<class,class,class> friend class MMAXTestsetIterator;

	NistIterator nistit_;
	MMAXIterator mmaxit_;

	MMAXTestsetIterator(
		NistIterator nistit,
		MMAXIterator mmaxit
	) :	nistit_(nistit), mmaxit_(mmaxit) {}

	template<class OtherValue,class OtherNistIterator,class OtherMMAXIterator>
	MMAXTestsetIterator(
		const MMAXTestsetIterator<OtherValue, OtherNistIterator, OtherMMAXIterator> &o
	) :	nistit_(o.nistit_), mmaxit_(o.mmaxit_) {}

	boost::shared_ptr<Value> dereference() const {
		return boost::make_shared<Value>(*mmaxit_, *nistit_);
	}

	template<class OtherValue,class OtherNistIterator,class OtherMMAXIterator>
	bool equal(
		const MMAXTestsetIterator<OtherValue, OtherNistIterator, OtherMMAXIterator> &o
	) const {
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

	Logger logger_;

	NistXmlTestset nistxml_;
	MMAXFileVector_ mmaxFiles_;

public:
	typedef uint size_type;
	typedef boost::shared_ptr<MMAXDocument> value_type;
	typedef boost::shared_ptr<const MMAXDocument> const_value_type;

	typedef MMAXTestsetIterator<
			MMAXDocument,
			NistXmlTestset::iterator,
			MMAXFileVector_::const_iterator
		> iterator;
	typedef MMAXTestsetIterator<
			const MMAXDocument,
			NistXmlTestset::const_iterator,
			MMAXFileVector_::const_iterator
		> const_iterator;

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
