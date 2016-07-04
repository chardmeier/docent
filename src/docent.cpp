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
#include <iostream>
#include <iterator>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

#include "Docent.h"
#include "DecoderConfiguration.h"
#include "DocumentState.h"
#include "MMAXTestset.h"
#include "NbestStorage.h"
#include "NistXmlCorpus.h"
#include "SearchAlgorithm.h"

void usage() {
	std::cerr << "Usage: docent [-d moduleToDebug]"
		" [-t moses-translations.xml]"
		" config.xml [input.mmax-dir] input.xml"
		<< std::endl;
	exit(1);
}

template<class Testset> void
processTestset(
	const DecoderConfiguration &config,
	Testset &testset
);

int main(int argc, char **argv)
{
	std::string configFile, mosesResultFilename;
	std::vector<std::string> args;
	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-d")) {
			if(i >= argc - 1)
				usage();
			Logger::setLogLevel(argv[++i], debug);
		} else if(strcmp(argv[i], "-t") == 0) {
			if(i >= argc - 1)
				usage();
			mosesResultFilename = argv[++i];
		} else
			args.push_back(argv[i]);
	}

	if(args.size() < 2 || args.size() > 3)
		usage();

	ConfigurationFile cf(args[0]);
	if(!mosesResultFilename.empty()) {
		cf.modifyAttribute(
			"/docent/state-generator/initial-state",
			"type",
			"testset"
		);
		cf.modifyOrAddProperty(
			"/docent/state-generator/initial-state",
			"file",
			mosesResultFilename
		);
	}
	DecoderConfiguration config(cf);

	std::string inputMMAX, inputXML;
	if(args.size() == 2) {
		inputXML = args[1];
		NistXmlCorpus testset(inputXML);
		processTestset(config, testset);
	} else if(args.size() == 3) {
		inputMMAX = args[1];
		inputXML = args[2];
		MMAXTestset testset(inputMMAX, inputXML);
		processTestset(config, testset);
	}
	return 0;
}

template<class Testset>
void processTestset(
	const DecoderConfiguration &config,
	Testset &testset
) {
	uint docNum = 0;
	BOOST_FOREACH(typename Testset::value_type inputdoc, testset) {
		boost::shared_ptr<DocumentState> doc =
			boost::make_shared<DocumentState>(config, inputdoc, docNum);
		NbestStorage nbest(1);
		std::cerr << "Initial score: " << doc->getScore() << std::endl;
		config.getSearchAlgorithm().search(doc, nbest);
		std::cerr << "Final score: " << doc->getScore() << std::endl;
		inputdoc->setTranslation(doc->asPlainTextDocument());
		docNum++;
	}
	testset.outputTranslation(std::cout);
}
