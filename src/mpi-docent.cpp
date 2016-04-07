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

#include <mpi.h>

#include <boost/foreach.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/nonblocking.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/thread/thread.hpp>
#include <boost/unordered_map.hpp>

#include "Docent.h"
#include "DecoderConfiguration.h"
#include "DocumentState.h"
#include "MMAXDocument.h"
#include "NbestStorage.h"
#include "NistXmlTestset.h"
#include "Random.h"
#include "SimulatedAnnealing.h"

class DocumentDecoder {
private:
	typedef std::pair<uint,MMAXDocument> NumberedInputDocument;
	typedef std::pair<uint,PlainTextDocument> NumberedOutputDocument;

	static const int TAG_TRANSLATE = 0;
	static const int TAG_STOP_TRANSLATING = 1;
	static const int TAG_COLLECT = 2;
	static const int TAG_STOP_COLLECTING = 3;

	static Logger logger_;

	boost::mpi::communicator communicator_;
	DecoderConfiguration configuration_;

	static void manageTranslators(boost::mpi::communicator comm, NistXmlTestset &testset);

	PlainTextDocument runDecoder(const NumberedInputDocument &input);

public:
	DocumentDecoder(boost::mpi::communicator comm, const std::string &config) :
		communicator_(comm), configuration_(ConfigurationFile(config)) {}

	void runMaster(const std::string &infile);
	void translate();
};

Logger DocumentDecoder::logger_("DocumentDecoder");

int main(int argc, char **argv) {
	int prov;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &prov);
	std::cerr << "MPI implementation provides thread level " << prov << std::endl;

	boost::mpi::communicator world;

	if(argc != 3) {
		std::cerr << "Usage: docent config.xml input.xml" << std::endl;
		return 1;
	}

	DocumentDecoder decoder(world, argv[1]);

	if(world.rank() == 0)
		decoder.runMaster(argv[2]);
	else
		decoder.translate();

	MPI_Finalize();
	return 0;
}

void DocumentDecoder::runMaster(const std::string &infile) {
	NistXmlTestset testset(infile);

	boost::thread manager(manageTranslators, communicator_, testset);

	translate();

	manager.join();

	testset.outputTranslation(std::cout);
}

void DocumentDecoder::manageTranslators(boost::mpi::communicator comm, NistXmlTestset &testset) {
	namespace mpi = boost::mpi;

	mpi::request reqs[2];
	int stopped = 0;

	NumberedOutputDocument translation;
	reqs[0] = comm.irecv(mpi::any_source, TAG_COLLECT, translation);
	reqs[1] = comm.irecv(mpi::any_source, TAG_STOP_COLLECTING);

	NistXmlTestset::const_iterator it = testset.begin();
	uint docno = 0;
	for(int i = 0; i < comm.size() && it != testset.end(); ++i, ++docno, ++it) {
		LOG(logger_, debug, "S: Sending document " << docno << " to translator " << i);
		comm.send(i, TAG_TRANSLATE, std::make_pair(docno, *(*it)->asMMAXDocument()));
	}

	for(;;) {
		std::pair<mpi::status, mpi::request *> wstat = mpi::wait_any(reqs, reqs + 2);
		if(wstat.first.tag() == TAG_STOP_COLLECTING) {
			stopped++;
			LOG(logger_, debug, "C: Received STOP_COLLECTING from translator "
				<< wstat.first.source() << ", now " << stopped << " stopped translators.");
			if(stopped == comm.size()) {
				reqs[0].cancel();
				return;
			}
			*wstat.second = comm.irecv(mpi::any_source, TAG_STOP_COLLECTING);
		} else {
			LOG(logger_, debug, "C: Received translation of document " <<
				translation.first << " from translator " << wstat.first.source());
			reqs[0] = comm.irecv(mpi::any_source, TAG_COLLECT, translation);
			if(it != testset.end()) {
				LOG(logger_, debug, "S: Sending document " << docno <<
					" to translator " << wstat.first.source());
				comm.send(wstat.first.source(), TAG_TRANSLATE,
					std::make_pair(docno, *(*it)->asMMAXDocument()));
				++docno; ++it;
			} else {
				LOG(logger_, debug,
					"S: Sending STOP_TRANSLATING to translator " << wstat.first.source());
				comm.send(wstat.first.source(), TAG_STOP_TRANSLATING);
			}
			testset[translation.first]->setTranslation(translation.second);
		}
	}
}

void DocumentDecoder::translate() {
	namespace mpi = boost::mpi;

	mpi::request reqs[2];
	reqs[1] = communicator_.irecv(0, TAG_STOP_TRANSLATING);
	NumberedInputDocument input;
	for(;;) {
		reqs[0] = communicator_.irecv(0, TAG_TRANSLATE, input);
		std::pair<mpi::status, mpi::request *> wstat = mpi::wait_any(reqs, reqs + 2);
		if(wstat.first.tag() == TAG_STOP_TRANSLATING) {
			LOG(logger_, debug, "T: Received STOP_TRANSLATING.");
			reqs[0].cancel();
			communicator_.send(0, TAG_STOP_COLLECTING);
			return;
		} else {
			NumberedOutputDocument output;
			LOG(logger_, debug, "T: Received document " << input.first << " for translation.");
			output.first = input.first;
			output.second = runDecoder(input);
			LOG(logger_, debug, "T: Sending translation of document " << input.first << " to collector.");
			communicator_.send(0, TAG_COLLECT, output);
		}
	}
}

PlainTextDocument DocumentDecoder::runDecoder(const NumberedInputDocument &input) {
	boost::shared_ptr<MMAXDocument> mmax = boost::make_shared<MMAXDocument>(input.second);
	boost::shared_ptr<DocumentState> doc(new DocumentState(configuration_, mmax, input.first));
	NbestStorage nbest(1);
	std::cerr << "Initial score: " << doc->getScore() << std::endl;
	configuration_.getSearchAlgorithm().search(doc, nbest);
	std::cerr << "Final score: " << doc->getScore() << std::endl;
	return doc->asPlainTextDocument();
}
