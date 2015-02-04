/*
 *  StateGenerator.cpp
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
#include "DecoderConfiguration.h"
#include "FeatureFunction.h"
#include "MMAXDocument.h"
#include "PhrasePair.h"
#include "PhrasePairCollection.h"
#include "PhraseTable.h"
#include "Random.h"
#include "SearchStep.h"
#include "StateGenerator.h"

#include <algorithm>
#include <sstream>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/archive/tmpdir.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

struct ChangePhraseTranslationOperation : public StateOperation {
private:
	mutable Logger logger_;

public:
	ChangePhraseTranslationOperation(const Parameters &params) :
		logger_("ChangePhraseTranslationOperation") {}

	virtual std::string getDescription() const {
		return "ChangePhraseTranslation";
	}

	virtual SearchStep *createSearchStep(const DocumentState &doc) const;
};

class PermutePhrasesOperation : public StateOperation {
private:
	mutable Logger logger_;
	Float phrasePermutationDecay_;

public:
	PermutePhrasesOperation(const Parameters &params);

	virtual std::string getDescription() const {
		std::ostringstream os;
		os << "PermutePhrases(decay=" << phrasePermutationDecay_ << ")";
		return os.str();
	}

	virtual SearchStep *createSearchStep(const DocumentState &doc) const;
};

class LinearisePhrasesOperation : public StateOperation {
private:
	mutable Logger logger_;
	Float phraseLinearisationDecay_;

public:
	LinearisePhrasesOperation(const Parameters &params);

	virtual std::string getDescription() const {
		std::ostringstream os;
		os << "LinearisePhrases(decay=" << phraseLinearisationDecay_ << ")";
		return os.str();
	}

	virtual SearchStep *createSearchStep(const DocumentState &doc) const;
};

class SwapPhrasesOperation : public StateOperation {
private:
	mutable Logger logger_;
	Float swapDistanceDecay_;

public:
	SwapPhrasesOperation(const Parameters &params);

	virtual std::string getDescription() const {
		std::ostringstream os;
		os << "SwapPhrases(decay=" << swapDistanceDecay_ << ")";
		return os.str();
	}

	virtual SearchStep *createSearchStep(const DocumentState &doc) const;
};

class MovePhrasesOperation : public StateOperation {
private:
	mutable Logger logger_;
	Float blockSizeDecay_;
	Float rightMovePreference_;
	Float rightDistanceDecay_;
	Float leftDistanceDecay_;

public:
	MovePhrasesOperation(const Parameters &params);

	virtual std::string getDescription() const {
		std::ostringstream os;
		os << "MovePhrases(block-size-decay=" << blockSizeDecay_ <<
			",right-move-preference=" << rightMovePreference_ <<
			",right-distance-decay=" << rightDistanceDecay_ <<
			",left-distance-decay=" << leftDistanceDecay_ << ')';
		return os.str();
	}

	virtual SearchStep *createSearchStep(const DocumentState &doc) const;
};

class ResegmentOperation : public StateOperation {
private:
	mutable Logger logger_;
	Float phraseResegmentationDecay_;

public:
	ResegmentOperation(const Parameters &params);

	virtual std::string getDescription() const {
		std::ostringstream os;
		os << "Resegment(decay=" << phraseResegmentationDecay_ << ")";
		return os.str();
	}

	virtual SearchStep *createSearchStep(const DocumentState &doc) const;
};

SearchStep *ChangePhraseTranslationOperation::createSearchStep(const DocumentState &doc) const {
	const std::vector<boost::shared_ptr<const PhrasePairCollection> > &phraseTranslations = getPhraseTranslations(doc);
	const std::vector<PhraseSegmentation> &sentences = doc.getPhraseSegmentations();
	

	Random rnd = doc.getDecoderConfiguration()->getRandom();

	uint sentno = doc.drawSentence(rnd);
	const PhraseSegmentation &sent = sentences[sentno];
	const PhrasePairCollection &pcoll = *phraseTranslations[sentno];

	uint sentsize = sent.size();
	LOG(logger_, verbose, "changePhraseTranslation " << sentno << " " << sentsize);
	uint ph = rnd.drawFromRange(sentsize);
	PhraseSegmentation::const_iterator it = sent.begin();
	for(uint i = 0; i < ph; i++)
		++it;
		
	AnchoredPhrasePair pp = pcoll.proposeAlternativeTranslation(*it);
	
	if(*it == pp)
		return NULL;

	const std::vector<FeatureFunction::State *> &featureStates = getFeatureStates(doc);
	SearchStep *step = new SearchStep(this, doc, featureStates);
	PhraseSegmentation::const_iterator endit = it;
	step->addModification(sentno, ph, ph+1, it, ++endit, PhraseSegmentation(1, pp));
	
	return step;
}

PermutePhrasesOperation::PermutePhrasesOperation(const Parameters &params) :
		logger_("PermutePhrasesOperation") {
	phrasePermutationDecay_ = params.get<Float>("phrase-permutation-decay");
}

SearchStep *PermutePhrasesOperation::createSearchStep(const DocumentState &doc) const {
	const std::vector<PhraseSegmentation> &sentences = doc.getPhraseSegmentations();

	LOG(logger_, verbose, "permutePhrases");
	Random rnd = doc.getDecoderConfiguration()->getRandom();

	uint sentno;
	uint sentsize;
	uint trials = 0;
	do {
		sentno = doc.drawSentence(rnd);
		sentsize = sentences[sentno].size();
	} while(sentsize < 2 && trials++ < 10);

	if(sentsize < 2)
		return NULL;

	const PhraseSegmentation &sent = sentences[sentno];

	uint nperm = rnd.drawFromGeometricDistribution(phrasePermutationDecay_, sentsize - 1) + 1;
	uint start = rnd.drawFromRange(sentsize - nperm + 1);
	
	PhraseSegmentation::const_iterator os = sent.begin();
	std::advance(os, start);
	PhraseSegmentation::const_iterator oe = os;
	std::advance(oe, nperm);

	std::vector<AnchoredPhrasePair> pg(os, oe);
	std::pair<PhraseSegmentation::const_iterator,std::vector<AnchoredPhrasePair>::iterator> m1;
	std::pair<PhraseSegmentation::const_reverse_iterator,std::vector<AnchoredPhrasePair>::reverse_iterator> m2;
	trials = 0;
	do {
		std::random_shuffle(pg.begin(), pg.end(), rnd.getUintGenerator());
		m1 = std::mismatch(os, oe, pg.begin());
	} while(m1.first == oe && trials++ < 10);
	if(m1.first == oe)
		return NULL;
	m2 = std::mismatch(std::reverse_iterator<PhraseSegmentation::const_iterator>(oe),
		std::reverse_iterator<PhraseSegmentation::const_iterator>(os), pg.rbegin());

	LOG(logger_, debug, "permute");
	for(PhraseSegmentation::const_iterator it = m1.first; it != m2.first.base(); ++it)
		LOG(logger_, debug, *it);

	start += std::distance(pg.begin(), m1.second);
	uint end = start + std::distance(m1.second, m2.second.base());

	const std::vector<FeatureFunction::State *> &featureStates = getFeatureStates(doc);
	SearchStep *step = new SearchStep(this, doc, featureStates);
	step->addModification(sentno, start, end, m1.first, m2.first.base(), PhraseSegmentation(m1.second, m2.second.base()));
	
	return step;
}

LinearisePhrasesOperation::LinearisePhrasesOperation(const Parameters &params) :
		logger_("LinearisePhrasesOperation") {
	phraseLinearisationDecay_ = params.get<Float>("phrase-linearisation-decay");
}

SearchStep *LinearisePhrasesOperation::createSearchStep(const DocumentState &doc) const {
	const std::vector<PhraseSegmentation> &sentences = doc.getPhraseSegmentations();

	LOG(logger_, verbose, "linearisePhrases");
	Random rnd = doc.getDecoderConfiguration()->getRandom();

	uint sentno;
	uint sentsize;
	uint trials = 0;
	do {
		sentno = doc.drawSentence(rnd);
		sentsize = sentences[sentno].size();
	} while(sentsize < 2 && trials++ < 10);

	if(sentsize < 2)
		return NULL;

	const PhraseSegmentation &sent = sentences[sentno];

	uint nperm = rnd.drawFromGeometricDistribution(phraseLinearisationDecay_, sentsize - 1) + 1;
	uint start = rnd.drawFromRange(sentsize - nperm + 1);
	
	PhraseSegmentation::const_iterator os = sent.begin();
	std::advance(os, start);
	PhraseSegmentation::const_iterator oe = os;
	std::advance(oe, nperm);

	// check if already in order
	os = std::adjacent_find(os, oe, std::not2(CompareAnchoredPhrasePairs()));
	if(os == oe)
		return NULL;

	std::vector<AnchoredPhrasePair> pg(os, oe);
	std::sort(pg.begin(), pg.end(), CompareAnchoredPhrasePairs());

	std::pair<PhraseSegmentation::const_iterator,std::vector<AnchoredPhrasePair>::iterator> m1;
	std::pair<PhraseSegmentation::const_reverse_iterator,std::vector<AnchoredPhrasePair>::reverse_iterator> m2;
	m1 = std::mismatch(os, oe, pg.begin());
	m2 = std::mismatch(std::reverse_iterator<PhraseSegmentation::const_iterator>(oe),
		std::reverse_iterator<PhraseSegmentation::const_iterator>(os), pg.rbegin());

	LOG(logger_, debug, "linearise");
	for(PhraseSegmentation::const_iterator it = m1.first; it != m2.first.base(); ++it)
		LOG(logger_, debug, *it);

	start += std::distance(pg.begin(), m1.second);
	uint end = start + std::distance(m1.second, m2.second.base());

	const std::vector<FeatureFunction::State *> &featureStates = getFeatureStates(doc);
	SearchStep *step = new SearchStep(this, doc, featureStates);
	step->addModification(sentno, start, end, m1.first, m2.first.base(), PhraseSegmentation(m1.second, m2.second.base()));
	
	return step;
}

SwapPhrasesOperation::SwapPhrasesOperation(const Parameters &params) :
		logger_("SwapPhrasesOperation") {
	swapDistanceDecay_ = params.get<Float>("swap-distance-decay");
}

SearchStep *SwapPhrasesOperation::createSearchStep(const DocumentState &doc) const {
	const std::vector<PhraseSegmentation> &sentences = doc.getPhraseSegmentations();

	LOG(logger_, verbose, "swapPhrases");
	Random rnd = doc.getDecoderConfiguration()->getRandom();

	uint sentno;
	uint sentsize;
	uint trials = 0;
	do {
		sentno = doc.drawSentence(rnd);
		sentsize = sentences[sentno].size();
	} while(sentsize < 2 && trials++ < 10);

	if(sentsize < 2)
		return NULL;

	const PhraseSegmentation &sent = sentences[sentno];

	uint phrase1 = rnd.drawFromRange(sentsize);
	bool direction;
	if(phrase1 == 0)
		direction = true;
	else if(phrase1 == sentsize - 1)
		direction = false;
	else
		direction = rnd.flipCoin();

	uint phrase2;
	if(direction) {
		if(phrase1 == sentsize - 2)
			phrase2 = sentsize - 1;
		else {
			uint ph2range = sentsize - phrase1 - 1;
			uint ph2dist = rnd.drawFromGeometricDistribution(swapDistanceDecay_, ph2range - 1) + 1;
			phrase2 = phrase1 + ph2dist;
			assert(phrase2 < sentsize);
		}
	} else {
		if(phrase1 == 1)
			phrase2 = 0;
		else {
			uint ph2dist = rnd.drawFromGeometricDistribution(swapDistanceDecay_, phrase1 - 1) + 1;
			phrase2 = phrase1 - ph2dist;
		}
	}
	
	PhraseSegmentation::const_iterator it1 = sent.begin();
	std::advance(it1, phrase1);
	PhraseSegmentation::const_iterator after_it1 = it1;
	++after_it1;

	PhraseSegmentation::const_iterator it2 = sent.begin();
	std::advance(it2, phrase2);
	PhraseSegmentation::const_iterator after_it2 = it2;
	++after_it2;

	LOG(logger_, debug, "swap");
	LOG(logger_, debug, *it1);
	LOG(logger_, debug, *it2);

	const std::vector<FeatureFunction::State *> &featureStates = getFeatureStates(doc);
	SearchStep *step = new SearchStep(this, doc, featureStates);
	step->addModification(sentno, phrase1, phrase1 + 1, it1, after_it1, PhraseSegmentation(it2, after_it2));
	step->addModification(sentno, phrase2, phrase2 + 1, it2, after_it2, PhraseSegmentation(it1, after_it1));
	
	return step;
}

MovePhrasesOperation::MovePhrasesOperation(const Parameters &params) :
		logger_("MovePhrasesOperation") {
	blockSizeDecay_ = params.get<Float>("block-size-decay");
	rightMovePreference_ = params.get<Float>("right-move-preference", Float(.5));
	rightDistanceDecay_ = params.get<Float>("right-distance-decay");
	leftDistanceDecay_ = params.get<Float>("left-distance-decay");
}

SearchStep *MovePhrasesOperation::createSearchStep(const DocumentState &doc) const {
	const std::vector<PhraseSegmentation> &sentences = doc.getPhraseSegmentations();

	LOG(logger_, verbose, "movePhrases");
	Random rnd = doc.getDecoderConfiguration()->getRandom();

	uint sentno;
	uint sentsize;
	uint trials = 0;
	do {
		sentno = doc.drawSentence(rnd);
		sentsize = sentences[sentno].size();
	} while(sentsize < 2 && trials++ < 10);

	if(sentsize < 2)
		return NULL;

	const PhraseSegmentation &sent = sentences[sentno];

	bool direction = rnd.flipCoin(rightMovePreference_);

	uint block = rnd.drawFromGeometricDistribution(blockSizeDecay_, sentsize - 2) + 1;
	uint start = rnd.drawFromRange(sentsize - block);

	if(!direction)
		start++;

	uint dest;
	if(direction) {
		if(start + block == sentsize - 1)
			dest = sentsize;
		else {
			uint range = sentsize - start - block;
			uint dist = rnd.drawFromGeometricDistribution(rightDistanceDecay_, range - 1) + 1;
			dest = start + block + dist;
		}
	} else {
		if(start == 1)
			dest = 0;
		else {
			uint dist = rnd.drawFromGeometricDistribution(leftDistanceDecay_, start - 1) + 1;
			dest = start - dist;
		}
	}
	assert(dest <= sentsize);

	PhraseSegmentation::const_iterator block_start = sent.begin();
	std::advance(block_start, start);
	PhraseSegmentation::const_iterator block_end = block_start;
	std::advance(block_end, block);

	PhraseSegmentation::const_iterator dest_it = sent.begin();
	std::advance(dest_it, dest);

	const std::vector<FeatureFunction::State *> &featureStates = getFeatureStates(doc);
	SearchStep *step = new SearchStep(this, doc, featureStates);
	step->addModification(sentno, dest, dest, dest_it, dest_it, PhraseSegmentation(block_start, block_end));
	step->addModification(sentno, start, start + block, block_start, block_end, PhraseSegmentation());
	
	return step;
}

ResegmentOperation::ResegmentOperation(const Parameters &params) :
		logger_("ResegmentOperation") {
	phraseResegmentationDecay_ = params.get<Float>("phrase-resegmentation-decay");
}

SearchStep *ResegmentOperation::createSearchStep(const DocumentState &doc) const {
	const std::vector<boost::shared_ptr<const PhrasePairCollection> > &phraseTranslations = getPhraseTranslations(doc);
	const std::vector<PhraseSegmentation> &sentences = doc.getPhraseSegmentations();
	using namespace boost::lambda;

	LOG(logger_, verbose, "resegment");
	Random rnd = doc.getDecoderConfiguration()->getRandom();

	uint sentno = doc.drawSentence(rnd);
	const PhraseSegmentation &sent = sentences[sentno];
	const PhrasePairCollection &pcoll = *phraseTranslations[sentno];

	uint sentsize = sent.size();
	uint nperm = rnd.drawFromGeometricDistribution(phraseResegmentationDecay_, sentsize - 1) + 1;
	uint start = rnd.drawFromRange(sentsize - nperm + 1);

	PhraseSegmentation::const_iterator os = sent.begin();
	for(uint i = 0; i < start; i++)
		++os;
	PhraseSegmentation::const_iterator oe = os;
	for(uint i = start; i < start + nperm; i++)
		++oe;

	CoverageBitmap tgt(pcoll.getSentenceLength());
	std::for_each(os, oe, tgt |= bind<const CoverageBitmap &>(&AnchoredPhrasePair::first, _1));

	LOG(logger_, debug, "Resegmenting " << tgt);

	PhraseSegmentation newseg = pcoll.proposeSegmentation(tgt);

	std::pair<PhraseSegmentation::const_iterator,PhraseSegmentation::const_iterator> m1;
	std::pair<PhraseSegmentation::const_reverse_iterator,PhraseSegmentation::const_reverse_iterator> m2;
	m1 = std::mismatch(os, oe, newseg.begin());
	if(m1.first == oe)
		return NULL;
	m2 = std::mismatch(PhraseSegmentation::const_reverse_iterator(oe), PhraseSegmentation::const_reverse_iterator(os), newseg.rbegin());

	PhraseSegmentation::const_iterator it = newseg.begin();
	while(it != m1.second)
		++start, ++it;

	uint end = start;
	while(it != m2.second.base())
		++end, ++it;

	const std::vector<FeatureFunction::State *> &featureStates = getFeatureStates(doc);
	SearchStep *step = new SearchStep(this, doc, featureStates);
	step->addModification(sentno, start, end, m1.first, m2.first.base(), PhraseSegmentation(m1.second, m2.second.base()));
	
	return step;
}

struct MonotonicStateInitialiserFactory : public StateInitialiserFactory {
	MonotonicStateInitialiserFactory(const Parameters &params) {}
	const StateInitialiser *createDocumentInitialiser(
		uint docNumber, const boost::shared_ptr<const MMAXDocument> &inputdoc) const;
};

struct MonotonicStateInitialiser : public StateInitialiser {
	virtual PhraseSegmentation initSegmentation(
		boost::shared_ptr<const PhrasePairCollection> phraseTranslations,
		const std::vector<Word> &sentence, int sentenceNumber) const;
};

const StateInitialiser *MonotonicStateInitialiserFactory::createDocumentInitialiser(uint docNumber,
		const boost::shared_ptr<const MMAXDocument> &inputdoc) const {
	return new MonotonicStateInitialiser();
}

PhraseSegmentation MonotonicStateInitialiser::initSegmentation(
		boost::shared_ptr<const PhrasePairCollection> phraseTranslations,
		const std::vector<Word> &sentence, int sentenceNumber) const {
	return phraseTranslations->proposeSegmentation();
}

class FileReadStateInitialiserFactory : public StateInitialiserFactory {
private:
	Logger logger_;
	std::vector<std::vector<PhraseSegmentation> > segmentations_;
public:
	FileReadStateInitialiserFactory(const Parameters &params);
	const StateInitialiser *createDocumentInitialiser(
		uint docNumber, const boost::shared_ptr<const MMAXDocument> &inputdoc) const;
};

class FileReadStateInitialiser : public StateInitialiser {
	friend class FileReadStateInitialiserFactory;
private:
	Logger logger_;
	const std::vector<std::vector<PhraseSegmentation> > &segmentations_;
	uint documentNumber_;

	FileReadStateInitialiser(const std::vector<std::vector<PhraseSegmentation> > &segmentations,
		uint docNumber);

public:
	virtual PhraseSegmentation initSegmentation(
		boost::shared_ptr<const PhrasePairCollection> phraseTranslations,
		const std::vector<Word> &sentence, int sentenceNumber) const;
};

FileReadStateInitialiserFactory::FileReadStateInitialiserFactory(const Parameters &params)
		: logger_("StateInitialiser") {
	// get file name from params 
	std::string filename = params.get<std::string>("file");

	// open the archive
	std::ifstream ifs(filename.c_str());
	if (!ifs.good()) {
		LOG(logger_, error, "problem reading file "<<filename);
		BOOST_THROW_EXCEPTION(FileFormatException());
	}
	boost::archive::text_iarchive ia(ifs);
	
	// restore the schedule from the archive
	ia >> segmentations_;
}

const StateInitialiser *FileReadStateInitialiserFactory::createDocumentInitialiser(uint docNumber,
		const boost::shared_ptr<const MMAXDocument> &inputdoc) const {
	return new FileReadStateInitialiser(segmentations_, docNumber);
}

FileReadStateInitialiser::FileReadStateInitialiser(
	const std::vector<std::vector<PhraseSegmentation> > &segmentations,
	uint docNumber)
		: logger_("StateInitialiser"), segmentations_(segmentations), documentNumber_(docNumber) {}


PhraseSegmentation FileReadStateInitialiser::initSegmentation(
		boost::shared_ptr<const PhrasePairCollection> phraseTranslations,
		const std::vector<Word> &sentence, int sentenceNumber) const {
	PhraseSegmentation phraseSegmentation = segmentations_[documentNumber_][sentenceNumber]; 
	//Check that all phrases in the phraseSegmentation exist in phraseTranslations
	if (!phraseTranslations->phrasesExist(phraseSegmentation)) {
		LOG(logger_, error, "ERROR: A phrase from the saved state does not exist in "
			"the phrase table, make sure that the same phrase table is used as "
			"when saving the state");
		BOOST_THROW_EXCEPTION(ConfigurationException());
	}

	return phraseSegmentation;
}

class MosesStateInitialiserFactory : public StateInitialiserFactory {
private:
	Logger logger_;
	boost::filesystem::path basedir_;

public:
	MosesStateInitialiserFactory(const Parameters &params);
	const StateInitialiser *createDocumentInitialiser(uint docNumber,
		const boost::shared_ptr<const MMAXDocument> &inputdoc) const;
};

class MosesStateInitialiser : public StateInitialiser {
	friend class MosesStateInitialiserFactory;
private:
	Logger logger_;
	uint docNumber_;
	std::vector<std::string> translations_;

	MosesStateInitialiser(uint docNumber,
		const boost::shared_ptr<const MMAXDocument> &inputdoc,
		const boost::filesystem::path &basedir);

public:
	virtual PhraseSegmentation initSegmentation(
		boost::shared_ptr<const PhrasePairCollection> phraseTranslations,
		const std::vector<Word> &sentence, int sentenceNumber) const;
};

MosesStateInitialiserFactory::MosesStateInitialiserFactory(const Parameters &params)
		: logger_("StateInitialiser"),
		  basedir_(params.get<std::string>("input-dir").c_str()) {
	if(!is_directory(status(basedir_))) {
		LOG(logger_, error, "Not a directory: " << params.get<std::string>("input-file"));
		BOOST_THROW_EXCEPTION(FileFormatException());
	}
}

const StateInitialiser *MosesStateInitialiserFactory::createDocumentInitialiser(uint docNumber,
		const boost::shared_ptr<const MMAXDocument> &inputdoc) const {
	return new MosesStateInitialiser(docNumber, inputdoc, basedir_);
}

MosesStateInitialiser::MosesStateInitialiser(uint docNumber,
	const boost::shared_ptr<const MMAXDocument> &inputdoc,
	const boost::filesystem::path &basedir)
		: logger_("StateInitialiser"), docNumber_(docNumber) {
	std::ostringstream fileno;
	fileno << std::setfill('0') << std::setw(5) << docNumber_;
	boost::filesystem::path file(basedir / fileno.str());
	std::ifstream input(file.c_str());
	std::string line;
	for(uint i = 0; i < inputdoc->getNumberOfSentences(); i++)
		if(getline(input, line))
			translations_.push_back(line);
		else {
			LOG(logger_, error, "Not enough lines in input file " << file);
			BOOST_THROW_EXCEPTION(FileFormatException());
		}
}

PhraseSegmentation MosesStateInitialiser::initSegmentation(
		boost::shared_ptr<const PhrasePairCollection> phraseTranslations,
		const std::vector<Word> &sentence, int sentenceNumber) const {
	PhraseSegmentation seg;

	typedef std::vector<AnchoredPhrasePair> PPVector;
	PPVector ppvec;
	phraseTranslations->copyPhrasePairs(std::back_inserter(ppvec));
	std::sort(ppvec.begin(), ppvec.end(), CompareAnchoredPhrasePairs());

	const std::string &mosesOut = translations_[sentenceNumber];

	CoverageBitmap totalcov(sentence.size()), cov(sentence.size());
	std::istringstream is(mosesOut);
	std::string word;
	std::vector<Word> tgtpd;
	std::vector<PhraseData> annotationPhrases(phraseTranslations->getAnnotationCount());
	while(getline(is, word, ' ')) {
		if(word.size() == 0) {
			LOG(logger_, error, "Empty token in moses output: " + mosesOut);
			BOOST_THROW_EXCEPTION(FileFormatException());
		}

		if(word[0] == '|') {
			if(tgtpd.empty()) {
				LOG(logger_, error, "Empty phrase in moses output: " + mosesOut);
				BOOST_THROW_EXCEPTION(FileFormatException());
			}

			std::istringstream is2(word);
			is2.get(); // discard '|', already checked
			uint from, to;
			is2 >> from;
			if(is2.get() != '-') {
				LOG(logger_, error, "Error parsing range " + word +
					" in moses output: " + mosesOut);
				BOOST_THROW_EXCEPTION(FileFormatException());
			}
			is2 >> to;
			if(is2.get() != '|' || is2.get() != EOF) {
				LOG(logger_, error, "Error parsing range " + word +
					" in moses output: " + mosesOut);
				BOOST_THROW_EXCEPTION(FileFormatException());
			}
			to++; // convert to half-open interval

			if(from >= to || to >= sentence.size()) {
				LOG(logger_, error, "Invalid range " << word <<
					" in moses output: " << mosesOut);
				BOOST_THROW_EXCEPTION(FileFormatException());
			}

			cov.clear();
			for(uint i = from; i < to; i++)
				cov.set(i);
			if(cov.intersects(totalcov)) {
				LOG(logger_, error, "Conflicting coverage ranges in moses output: " << mosesOut);
				BOOST_THROW_EXCEPTION(FileFormatException());
			}

			std::vector<Word> srcpd(sentence.begin() + from, sentence.begin() + to);
			
			CompareAnchoredPhrasePairs compareAPP;
			CompareAnchoredPhrasePairs::PhrasePairKey key(cov, srcpd, tgtpd);
			// find range of anchored phrase pairs with matching src and tgt phrases,
			// but possibly different annotations
			PPVector::const_iterator it = std::lower_bound(ppvec.begin(), ppvec.end(),
				key, compareAPP);
			using namespace boost::lambda;
			PPVector::const_iterator eit = std::find_if(it, ppvec.end(), bind(compareAPP, key, _1));

			if(it == eit) {
				LOG(logger_, error, "No matching phrase pair in phrase table: " <<
					srcpd << " ||| " << tgtpd);
				BOOST_THROW_EXCEPTION(FileFormatException());
			}

			class CheckAnnotations_ {
			private:
				const std::vector<PhraseData> &annotationPhrases_;

			public:
				CheckAnnotations_(const std::vector<PhraseData> &annotationPhrases) :
					annotationPhrases_(annotationPhrases) {}

				bool operator()(const AnchoredPhrasePair &app) const {
					const PhrasePairData &pp = app.second.get();
					if(pp.getAnnotationCount() != annotationPhrases_.size())
						return false;
					for(uint i = 0; i < annotationPhrases_.size(); i++)
						if(pp.getTargetAnnotation(i) != annotationPhrases_[i])
							return false;
					return true;
				}
			};

			PPVector found_it = std::find_if(it, eit, CheckAnnotations_(annotationPhrases));
			if(found_it == eit) {
				LOG(logger_, error, "No phrase pair with matching annotations in phrase table: " <<
					srcpd << " ||| " << tgtpd);
				BOOST_THROW_EXCEPTION(FileFormatException());
			}

			seg.push_back(*found_it);

			tgtpd.clear();
			std::for_each(annotationPhrases.begin(), annotationPhrases.end(),
				bind(&PhraseData::clear, _1));
		} else {
			std::istringstream is2(word);
			std::string part;
			getline(is2, part, '|');
			tgtpd.push_back(part);
			for(uint i = 0; i < phraseTranslations->getAnnotationCount(); i++) {
				if(!getline(is2, part, '|')) {
					LOG(logger_, error, "Too few annotations (expected " <<
						phraseTranslations->getAnnotationCount() <<
						"): " << word);
					BOOST_THROW_EXCEPTION(FileFormatException());
				}
				annotationPhrases[i].push_back(part);
			}
			is2.get();
			if(!is2.eof()) {
				LOG(logger_, error, "Too many annotations (expected " << 
					phraseTranslations->getAnnotationCount() <<
					"): " << word);
				BOOST_THROW_EXCEPTION(FileFormatException());
			}
		}
	}

	if(!tgtpd.empty()) {
		LOG(logger_, error, "Incomplete line: " << mosesOut);
		BOOST_THROW_EXCEPTION(FileFormatException());
	}

	return seg;
}

StateGenerator::StateGenerator(const std::string &initMethod, const Parameters &params, Random(random)) :
		logger_("StateGenerator"), random_(random) {
	if(initMethod == "monotonic")
		initialiser_ = new MonotonicStateInitialiserFactory(params);
	else if(initMethod == "saved-state")
		initialiser_ = new FileReadStateInitialiserFactory(params);
	else if(initMethod == "moses")
		initialiser_ = new MosesStateInitialiserFactory(params);
	else {
		LOG(logger_, error, "Unknown initialisation method: " << initMethod);
		BOOST_THROW_EXCEPTION(ConfigurationException());
	}
}

StateGenerator::~StateGenerator() {
	delete initialiser_;
}

void StateGenerator::addOperation(Float weight, const std::string &type, const Parameters &params) {
	if(type == "change-phrase-translation")
		operations_.push_back(new ChangePhraseTranslationOperation(params));
	else if(type == "permute-phrases")
		operations_.push_back(new PermutePhrasesOperation(params));
	else if(type == "linearise-phrases")
		operations_.push_back(new LinearisePhrasesOperation(params));
	else if(type == "swap-phrases")
		operations_.push_back(new SwapPhrasesOperation(params));
	else if(type == "move-phrases")
		operations_.push_back(new MovePhrasesOperation(params));
	else if(type == "resegment")
		operations_.push_back(new ResegmentOperation(params));
	else {
		LOG(logger_, error, "Unknown operation: " << type);
		BOOST_THROW_EXCEPTION(ConfigurationException());
	}
	
	if(!cumulativeOperationDistribution_.empty())
		weight += cumulativeOperationDistribution_.back();
	cumulativeOperationDistribution_.push_back(weight);
}

SearchStep *StateGenerator::createSearchStep(const DocumentState &doc) const {
	SearchStep *nextStep;
	for(;;) {
		uint next_op = random_.drawFromCumulativeDistribution(cumulativeOperationDistribution_);
		nextStep = operations_[next_op].createSearchStep(doc);
		
		// NULL just indicates that our operator wasn't able to produce a reasonable set of changes
		// for some reason.
		if(nextStep == NULL)
			continue;

		if(!nextStep->getModifications().empty())
			break;
		
		delete nextStep;
	}

	return nextStep;
}

