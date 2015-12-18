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
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/make_shared.hpp>
#include <boost/tokenizer.hpp>
#include <boost/unordered_map.hpp>
#include <boost/utility.hpp>
#include <boost/archive/tmpdir.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "Docent.h"
#include "DecoderConfiguration.h"
#include "DocumentState.h"
#include "MMAXDocument.h"
#include "NbestStorage.h"
#include "NistXmlTestset.h"
#include "Random.h"
#include "SimulatedAnnealing.h"

void usage();

template<class Testset>
class SingleDocumentTestset {
private:
	Testset &backend_;

public:
	SingleDocumentTestset(Testset &backend) : backend_(backend) {}

	typedef typename Testset::value_type value_type;
	typedef typename Testset::const_value_type const_value_type;

	typedef typename Testset::iterator iterator;
	typedef typename Testset::const_iterator const_iterator;

	iterator begin() {
		return backend_.begin();
	}

	iterator end() {
		if(backend_.size() > 0)
			return boost::next(backend_.begin());
		else
			return backend_.end();
	}

	const_iterator begin() const {
		return backend_.begin();
	}

	const_iterator end() const {
		if(backend_.size() > 0)
			return boost::next(backend_.begin());
		else
			return backend_.end();
	}

	uint size() const {
		if(backend_.size() > 0)
			return 1;
		else
			return 0;
	}

	void outputTranslation(std::ostream &os) const {
		backend_.outputTranslation(os);
	}
};

template<class Testset>
void processTestset(const ConfigurationFile &configFile, Testset &testset, const std::string &outstem, bool dumpStates, const std::string& firstStateFilename, const std::string& lastStateFilename);
std::string formatWordAlignment(const PhraseSegmentation &snt);

void printState(const std::string& filename, const std::vector<std::vector<PhraseSegmentation> >& state);

int main(int argc, char **argv) {
	std::vector<std::string> args;
	typedef std::pair<std::string,std::string> ModificationPair;
	std::vector<ModificationPair> xpset;
	std::vector<std::string> xpremove;
	std::string mmax, nistxml;
	bool translateSingleDocument = false;
	bool dumpstates = false;
	std::string firstStateFilename, lastStateFilename;

	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-m") == 0) {
			if(i >= argc - 2)
				usage();
			mmax = argv[++i];
			nistxml = argv[++i];
		} else if(strcmp(argv[i], "-n") == 0) {
			if(i >= argc - 1)
				usage();
			nistxml = argv[++i];
		} else if(strcmp(argv[i], "-s") == 0) {
			if(i >= argc - 2)
				usage();
			xpset.push_back(std::make_pair(argv[i+1], argv[i+2]));
			i += 2;
		} else if(strcmp(argv[i], "-r") == 0) {
			if(i >= argc - 1)
				usage();
			xpremove.push_back(argv[++i]);
		} else if(strcmp(argv[i], "-d") == 0) {
			if(i >= argc - 1)
				usage();
			Logger::setLogLevel(argv[++i], debug);
		} else if(strcmp(argv[i], "-v") == 0) {
			if(i >= argc - 1)
				usage();
			Logger::setLogLevel(argv[++i], verbose);
		} else if(strcmp(argv[i], "-1") == 0) {
			translateSingleDocument = true;
		} else if(strcmp(argv[i], "-pf") == 0) {
			if(i >= argc - 1)
				usage();
			 firstStateFilename = argv[++i];
		} else if(strcmp(argv[i], "-pl") == 0) {
			if(i >= argc - 1)
				usage();
			 lastStateFilename = argv[++i];
		} else if(strcmp(argv[i], "--dumpstates") == 0)
			dumpstates = true;
		else
			args.push_back(argv[i]);
	}

	if(nistxml.empty() || args.size() != 2)
		usage();

	const std::string &configFile = args[0];
	const std::string &outstem = args[1];

	ConfigurationFile config(configFile);

	BOOST_FOREACH(const ModificationPair &m, xpset)
		config.modifyNodes(m.first, m.second);
	BOOST_FOREACH(const std::string &m, xpremove)
		config.removeNodes(m);

	if(!mmax.empty()) {
		MMAXTestset testset(mmax, nistxml);
		if(translateSingleDocument) {
			SingleDocumentTestset<MMAXTestset> single(testset);
			processTestset(config, single, outstem, dumpstates, firstStateFilename, lastStateFilename);
		} else
			processTestset(config, testset, outstem, dumpstates, firstStateFilename, lastStateFilename);
	} else {
		NistXmlTestset testset(nistxml);
		if(translateSingleDocument) {
			SingleDocumentTestset<NistXmlTestset> single(testset);
			processTestset(config, single, outstem, dumpstates, firstStateFilename, lastStateFilename);
		} else
			processTestset(config, testset, outstem, dumpstates, firstStateFilename, lastStateFilename);
	}

	return 0;
}

void usage() {
	std::cerr << "Usage: lcurve-docent [-s xpath value] [-r xpath] "
		"[--dumpstates]  [-pf stateFileInitialisation] [-pl stateFileLast] "
		"{-n input.xml | -m input.mmaxdir input.xml} "
			"config.xml outstem" << std::endl;
	exit(1);
}

template<class Testset>
void processTestset(const ConfigurationFile &configFile, Testset &testset, const std::string &outstem, bool dumpStates, const std::string &firstStateFilename, const std::string &lastStateFilename) {
	try {
		//Random::initGenerator(3525497962);
		//Random::initGenerator(3812725332);

		DecoderConfiguration config(configFile);

		std::vector<typename Testset::value_type> inputdocs;
		inputdocs.reserve(testset.size());
		inputdocs.insert(inputdocs.end(), testset.begin(), testset.end());
		std::vector<SearchState *> states;
		states.reserve(testset.size());
		const SearchAlgorithm &algo = config.getSearchAlgorithm();
		uint docNum = 0;
		BOOST_FOREACH(typename Testset::value_type inputdoc, inputdocs) {
			boost::shared_ptr<DocumentState> doc =
				boost::make_shared<DocumentState>(config, inputdoc, docNum);
			states.push_back(algo.createState(doc));
			std::cerr << "* " << docNum << "\t0\t" << doc->getScore() << std::endl;

			PlainTextDocument ptout = doc->asPlainTextDocument();
			for(uint j = 0; j < ptout.getNumberOfSentences(); j++) {
				std::ostringstream os;
				Scores sntscores = doc->computeSentenceScores(j);
				os << std::inner_product(sntscores.begin(), sntscores.end(),
					config.getFeatureWeights().begin(), Float(0))
					<< " - " << sntscores
					<< " - " << formatWordAlignment(doc->getPhraseSegmentation(j));
				inputdocs[docNum]->annotateSentence(j, os.str());
			}
			std::ostringstream tos;
			tos << doc->getScore() << " - " << doc->getScores();
			inputdocs[docNum]->annotateDocument(tos.str());
			inputdocs[docNum]->setTranslation(ptout);

			docNum++;
		}
		std::ofstream of((outstem + ".000000000.xml").c_str());
		of.exceptions(std::ofstream::failbit | std::ofstream::badbit);
		testset.outputTranslation(of);
		of.close();
		
		// Print the state after initialization if asked for
		if (!firstStateFilename.empty()) {
		  std::vector<std::vector<PhraseSegmentation> > state;
		  for(uint i = 0; i < states.size(); i++) {
			  state.push_back(states[i]->getLastDocumentState()->getPhraseSegmentations());
		  }
		  printState(firstStateFilename, state);
		}


		uint steps_done = 0;
		std::vector<NbestStorage> nbest(inputdocs.size(), NbestStorage(1));		

		for(uint steps = 256; steps <= 134217728; steps *= 2) {
			for(uint i = 0; i < inputdocs.size(); i++) {
				std::cerr << "Document " << i << ", approaching " << steps << " steps." << std::endl;
				//std::cerr << "Initial score: " << docs[i]->getScore() << std::endl;
				algo.search(states[i], nbest[i], steps - steps_done, std::numeric_limits<uint>::max());
				std::vector<boost::shared_ptr<const DocumentState> > out(1);
				nbest[i].copyNbestList(out);
				std::cerr << "Final score: " << out[0]->getScore() << std::endl;
				std::cerr << "* " << i << '\t' << steps << '\t' << out[0]->getScore() << std::endl;
				PlainTextDocument ptout = out[0]->asPlainTextDocument();
				for(uint j = 0; j < ptout.getNumberOfSentences(); j++) {
					std::ostringstream os;
					Scores sntscores = out[0]->computeSentenceScores(j);
					os << std::inner_product(sntscores.begin(), sntscores.end(),
						config.getFeatureWeights().begin(), Float(0))
						<< " - " << sntscores
						<< " - " << formatWordAlignment(
						out[0]->getPhraseSegmentation(j));
					inputdocs[i]->annotateSentence(j, os.str());
				}
				std::ostringstream tos;
				tos << out[0]->getScore() << " - " << out[0]->getScores();
				inputdocs[i]->annotateDocument(tos.str());
				inputdocs[i]->setTranslation(ptout);
				if(dumpStates)
					out[0]->dumpFeatureFunctionStates();
			}
			steps_done = steps;
			std::ostringstream outname;
			outname << outstem << '.' << std::setfill('0') << std::setw(9) << steps << ".xml";
			std::ofstream of(outname.str().c_str());
			of.exceptions(std::ofstream::failbit | std::ofstream::badbit);
			testset.outputTranslation(of);
			of.close();
		}
		
		// Print the final best state if asked for
		if (!lastStateFilename.empty()) {
		  std::vector<std::vector<PhraseSegmentation> > state;
		  for(uint i = 0; i < nbest.size(); i++) {
			  std::cerr << "getting state for sentence " << i << std::endl;
			  state.push_back(nbest[i].getBestDocumentState()->getPhraseSegmentations());
		  }
		  printState(lastStateFilename, state);
		}

		BOOST_FOREACH(SearchState *state, states)
			delete state;
	} catch(DocentException &e) {
		std::cerr << boost::diagnostic_information(e);
		abort();
	}
}

std::string formatWordAlignment(const PhraseSegmentation &snt) {
	std::ostringstream out;
	uint tgtoffset = 0;
	BOOST_FOREACH(const AnchoredPhrasePair &app, snt) {
		uint srcoffset = app.first.find_first();
		const WordAlignment &wa = app.second.get().getWordAlignment();
		for(uint t = 0; t < app.second.get().getTargetPhrase().get().size(); t++)
			for(WordAlignment::const_iterator it = wa.begin_for_target(t);
					it != wa.end_for_target(t); ++it)
				out << (srcoffset + *it) << '-' << (tgtoffset + t) << ' ';
		tgtoffset += app.second.get().getTargetPhrase().get().size();
	}
	std::string retstr = out.str();
	retstr.erase(retstr.size() - 1);
	return retstr;
}

std::ostream &operator<<(std::ostream &os, const std::vector<Word> &phrase) {
	bool first = true;
	BOOST_FOREACH(const Word &w, phrase) {
		if(!first)
			os << ' ';
		else
			first = false;
		os << w;
	}

	return os;
}

std::ostream &operator<<(std::ostream &os, const PhraseSegmentation &seg) {
	std::copy(seg.begin(), seg.end(), std::ostream_iterator<AnchoredPhrasePair>(os, "\n"));
	return os;
}

std::ostream &operator<<(std::ostream &os, const AnchoredPhrasePair &ppair) {
	os << ppair.first << "\t[" << ppair.second.get().getSourcePhrase().get() << "] -\t[" << ppair.second.get().getTargetPhrase().get() << ']';
	return os;
}

void printState(const std::string& filename, const std::vector<std::vector<PhraseSegmentation> >& state) {
	std::ofstream ofs(filename.c_str());
	boost::archive::text_oarchive oa(ofs);
	oa << state;
}
