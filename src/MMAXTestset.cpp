/*
 *  MMAXTestset.cpp
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

#include "MMAXTestset.h"

#include <boost/filesystem/operations.hpp>

MMAXTestset::MMAXTestset(
	const std::string &directory,
	const std::string &nistxml
) :	logger_("MMAXTestset"),
	nistxmlcorpus_(nistxml)
{
	namespace fs = boost::filesystem;

	fs::path dir(directory);
	for(fs::directory_iterator
		it = fs::directory_iterator(dir);
		it != fs::directory_iterator();
		++it
	)
		if(it->path().extension() == ".mmax")
			mmaxFiles_.push_back(it->path());

	std::sort(mmaxFiles_.begin(), mmaxFiles_.end());

	if(mmaxFiles_.size() != nistxmlcorpus_.size()) {
		LOG(logger_, error, "MMAX test set has " << mmaxFiles_.size()
			<< " documents, NIST file has " << nistxmlcorpus_.size()
		);
		BOOST_THROW_EXCEPTION(FileFormatException());
	}
}
