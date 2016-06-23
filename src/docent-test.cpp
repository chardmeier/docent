/*
 *  docent.cpp
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

#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/lambda/lambda.hpp>

#include "Docent.h"
#include "DecoderConfiguration.h"
#include "DocumentState.h"
#include "MMAXDocument.h"
#include "NbestStorage.h"
#include "SearchAlgorithm.h"

void usage() {
	std::cerr << "Usage: cat test.txt |"
		" docent-test [-d moduleToDebug] config.xml"
		<< std::endl;
	exit(1);
}

int main(int argc, char **argv)
{
	std::string configFile;
	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-d")) {
			if(i >= argc - 1)
				usage();
			Logger::setLogLevel(argv[++i], debug);
		} else {
			configFile = std::string(argv[i]);
		}
	}
	if(configFile.empty())
		usage();

	ConfigurationFile cf(configFile);
	DecoderConfiguration config(cf);

	std::string line;
	uint docNum = 0;
	while(getline(std::cin, line)) {
		boost::shared_ptr<MMAXDocument> mmax = boost::make_shared<MMAXDocument>();
		std::vector<std::string> tokens;
		boost::split(tokens, line, boost::is_any_of(" "));
		mmax->addSentence(tokens.begin(), tokens.end());

		boost::shared_ptr<DocumentState> doc =
			boost::make_shared<DocumentState>(config, mmax, docNum);
		NbestStorage nbest(5);

		std::cout << "Initial state:" << std::endl;
		std::cout << *doc << "\n\n" << std::endl;

		config.getSearchAlgorithm().search(doc, nbest);
		std::cout << "Final state:" << std::endl;
		std::cout << *doc << "\n\n" << std::endl;

		std::vector<boost::shared_ptr<const DocumentState> > nbestList;
		nbest.copyNbestList(nbestList);
		std::cout << "N-best list size: " << nbestList.size() << std::endl;
		std::transform(
			nbestList.begin(),
			nbestList.end(),
			std::ostream_iterator<DocumentState>(std::cout, "\n\n"),
			*boost::lambda::_1
		);
		docNum++;
	}
	return 0;
}
