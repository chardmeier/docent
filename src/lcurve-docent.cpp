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
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_values_view.hpp>
#include <boost/log/filters/basic_filters.hpp>
#include <boost/log/utility/init/common_attributes.hpp>
#include <boost/log/utility/init/to_console.hpp>
#include <boost/make_shared.hpp>
#include <boost/tokenizer.hpp>
#include <boost/unordered_map.hpp>

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
void processTestset(const std::string &configFile, Testset &testset, const std::string &outstem, bool dumpStates);

class LogFilter : public std::unary_function<const boost::log::attribute_values_view &,bool> {
private:
	typedef boost::unordered_map<std::string,LogLevel> ChannelMapType_;
	ChannelMapType_ channelMap_;
	LogLevel defaultLevel_;

public:
	LogFilter() : defaultLevel_(normal) {}

	void setDefaultLevel(LogLevel deflt) {
		defaultLevel_ = deflt;
	}

	LogLevel &operator[](const std::string &channel) {
		return channelMap_[channel];
	}

	bool operator()(const boost::log::attribute_values_view &attrs) const {
		LogLevel level = *attrs["Severity"].extract<LogLevel>();
		ChannelMapType_::const_iterator it = channelMap_.find(*attrs["Channel"].extract<std::string>());
		if(it == channelMap_.end())
			return level >= defaultLevel_;
		else
			return level >= it->second;
	}
};

int main(int argc, char **argv) {
	std::vector<std::string> args;
	std::string mmax, nistxml;
	bool dumpstates = false;

	LogFilter logFilter;

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
		} else if(strcmp(argv[i], "-d") == 0) {
			logFilter[argv[++i]] = debug;
		} else if(strcmp(argv[i], "--dumpstates") == 0)
			dumpstates = true;
		else
			args.push_back(argv[i]);
	}

	boost::log::init_log_to_console()->set_filter(boost::log::filters::wrap(logFilter));
	boost::log::add_common_attributes();

	if(nistxml.empty() || args.size() != 2)
		usage();

	const std::string &config = args[0];
	const std::string &outstem = args[1];

	if(!mmax.empty()) {
		MMAXTestset testset(mmax, nistxml);
		processTestset(config, testset, outstem, dumpstates);
	} else {
		NistXmlTestset testset(nistxml);
		processTestset(config, testset, outstem, dumpstates);
	}

	return 0;
}

void usage() {
	std::cerr << "Usage: lcurve-docent [--dumpstates] {-n input.xml | -m input.mmaxdir input.xml} "
			"config.xml outstem" << std::endl;
	exit(1);
}

template<class Testset>
void processTestset(const std::string &configFile, Testset &testset, const std::string &outstem, bool dumpStates) {
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
		uint i = 0;
		BOOST_FOREACH(typename Testset::value_type inputdoc, inputdocs) {
			boost::shared_ptr<DocumentState> doc =
				boost::make_shared<DocumentState>(config, inputdoc);
			states.push_back(algo.createState(doc));
			std::cerr << "* " << i << "\t0\t" << doc->getScore() << std::endl;

			PlainTextDocument ptout = doc->asPlainTextDocument();
			for(uint j = 0; j < ptout.getNumberOfSentences(); j++) {
				std::ostringstream os;
				Scores sntscores = doc->computeSentenceScores(j);
				os << std::inner_product(sntscores.begin(), sntscores.end(),
					config.getFeatureWeights().begin(), Float(0))
					<< " - " << sntscores;
				inputdocs[i]->annotateSentence(j, os.str());
			}
			std::ostringstream tos;
			tos << doc->getScore() << " - " << doc->getScores();
			inputdocs[i]->annotateDocument(tos.str());
			inputdocs[i]->setTranslation(ptout);

			i++;
		}
		std::ofstream of((outstem + ".000000000.xml").c_str());
		of.exceptions(std::ofstream::failbit | std::ofstream::badbit);
		testset.outputTranslation(of);
		of.close();

		uint steps_done = 0;

		for(uint steps = 256; steps <= 134217728; steps *= 2) {
			for(uint i = 0; i < inputdocs.size(); i++) {
				NbestStorage nbest(1);
				std::cerr << "Document " << i << ", approaching " << steps << " steps." << std::endl;
				//std::cerr << "Initial score: " << docs[i]->getScore() << std::endl;
				algo.search(states[i], nbest, steps - steps_done, std::numeric_limits<uint>::max());
				std::vector<boost::shared_ptr<const DocumentState> > out(1);
				nbest.copyNbestList(out);
				std::cerr << "Final score: " << out[0]->getScore() << std::endl;
				std::cerr << "* " << i << '\t' << steps << '\t' << out[0]->getScore() << std::endl;
				PlainTextDocument ptout = out[0]->asPlainTextDocument();
				for(uint j = 0; j < ptout.getNumberOfSentences(); j++) {
					std::ostringstream os;
					Scores sntscores = out[0]->computeSentenceScores(j);
					os << std::inner_product(sntscores.begin(), sntscores.end(),
						config.getFeatureWeights().begin(), Float(0))
						<< " - " << sntscores;
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

		BOOST_FOREACH(SearchState *state, states)
			delete state;
	} catch(DocdecException &e) {
		std::cerr << boost::diagnostic_information(e);
		abort();
	}
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

