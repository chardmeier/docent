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
#include <boost/lambda/lambda.hpp>
#include <boost/unordered_map.hpp>

#include "Docent.h"
#include "DecoderConfiguration.h"
#include "DocumentState.h"
#include "MMAXDocument.h"
#include "NbestStorage.h"
#include "NistXmlTestset.h"
#include "Random.h"
#include "SimulatedAnnealing.h"

template<class Testset> void processTestset(const DecoderConfiguration &config, Testset &testset);

int main(int argc, char **argv) {
	bool showUsage = false;
	std::vector<std::string> args;
	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-d")) {
			if(i + 1 >= argc) {
				showUsage = true;
				break;
			} else
				Logger::setLogLevel(argv[i+1], debug);

			i++;
		} else
			args.push_back(argv[i]);
	}

	if(showUsage || args.size() < 1 || args.size() > 3) {
		std::cerr << "Usage: docent config.xml [[input.mmax-dir] input.xml]" << std::endl;
		return 1;
	}

	const std::string &configFile = args[0];
	std::string inputMMAX, inputXML;
	if(args.size() == 2)
		inputXML = args[1];
	else if(args.size() == 3) {
		inputMMAX = args[1];
		inputXML = args[2];
	}

	ConfigurationFile cf(configFile);
	DecoderConfiguration config(cf);

	if(inputMMAX.empty() && inputXML.empty()) {
		std::string line;
		uint docNum = 0;
		while(getline(std::cin, line)) {
			boost::shared_ptr<MMAXDocument> mmax = boost::make_shared<MMAXDocument>();
			std::vector<std::string> tokens;
			boost::split(tokens, line, boost::is_any_of(" "));
			mmax->addSentence(tokens.begin(), tokens.end());

			boost::shared_ptr<DocumentState> doc = boost::make_shared<DocumentState>(
				config, mmax, docNum);
			NbestStorage nbest(5);
			
			std::cout << "Initial state:" << std::endl;
			std::cout << *doc << "\n\n" << std::endl;

			config.getSearchAlgorithm().search(doc, nbest);
			std::cout << "Final state:" << std::endl;
			std::cout << *doc << "\n\n" << std::endl;

			std::vector<boost::shared_ptr<const DocumentState> > nbestList;
			nbest.copyNbestList(nbestList);
			std::cout << "N-best list size: " << nbestList.size() << std::endl;
			std::transform(nbestList.begin(), nbestList.end(),
				std::ostream_iterator<DocumentState>(std::cout, "\n\n"), *boost::lambda::_1);
			docNum++;
		}
	} else if(inputMMAX.empty()) {
		NistXmlTestset testset(inputXML);
		processTestset(config, testset);
	} else {
		MMAXTestset testset(inputMMAX, inputXML);
		processTestset(config, testset);
	}

	return 0;
}

template<class Testset>
void processTestset(const DecoderConfiguration &config, Testset &testset) {
	uint docNum = 0;
	BOOST_FOREACH(typename Testset::value_type inputdoc, testset) {
		boost::shared_ptr<DocumentState> doc = boost::make_shared<DocumentState>(config, inputdoc, docNum);
		NbestStorage nbest(1);
		std::cerr << "Initial score: " << doc->getScore() << std::endl;
		config.getSearchAlgorithm().search(doc, nbest);
		std::cerr << "Final score: " << doc->getScore() << std::endl;
		inputdoc->setTranslation(doc->asPlainTextDocument());
		docNum++;
	}
	testset.outputTranslation(std::cout);
}
