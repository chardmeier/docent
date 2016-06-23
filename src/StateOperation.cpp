/*
 *  StateOperation.h
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

#include "StateOperation.h"

#include "PhrasePairCollection.h"
#include "SearchStep.h"

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

SearchStep
*ChangePhraseTranslationOperation::createSearchStep(
	const DocumentState &doc
) const {
	const std::vector<PhraseSegmentation>
		&sentences = doc.getPhraseSegmentations();
	const std::vector<boost::shared_ptr<const PhrasePairCollection> >
		&phraseTranslations = getPhraseTranslations(doc);

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

	const std::vector<FeatureFunction::State *>
		&featureStates = getFeatureStates(doc);
	SearchStep *step = new SearchStep(this, doc, featureStates);
	PhraseSegmentation::const_iterator endit = it;
	step->addModification(
		sentno,
		ph, ph + 1,
		it, ++endit,
		PhraseSegmentation(1, pp)
	);
	return step;
}


PermutePhrasesOperation::PermutePhrasesOperation(
	const Parameters &params
) :	logger_("PermutePhrasesOperation")
{
	phrasePermutationDecay_ = params.get<Float>("phrase-permutation-decay");
}

SearchStep
*PermutePhrasesOperation::createSearchStep(
	const DocumentState &doc
) const {
	const std::vector<PhraseSegmentation>
		&sentences = doc.getPhraseSegmentations();

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
	m2 = std::mismatch(
		std::reverse_iterator<PhraseSegmentation::const_iterator>(oe),
		std::reverse_iterator<PhraseSegmentation::const_iterator>(os),
		pg.rbegin()
	);

	LOG(logger_, debug, "permute");
	for(PhraseSegmentation::const_iterator it = m1.first; it != m2.first.base(); ++it)
		LOG(logger_, debug, *it);

	start += std::distance(pg.begin(), m1.second);
	uint end = start + std::distance(m1.second, m2.second.base());

	const std::vector<FeatureFunction::State *> &featureStates = getFeatureStates(doc);
	SearchStep *step = new SearchStep(this, doc, featureStates);
	step->addModification(
		sentno,
		start, end,
		m1.first, m2.first.base(),
		PhraseSegmentation(m1.second, m2.second.base())
	);
	return step;
}


LinearisePhrasesOperation::LinearisePhrasesOperation(
	const Parameters &params
) :	logger_("LinearisePhrasesOperation")
{
	phraseLinearisationDecay_ = params.get<Float>("phrase-linearisation-decay");
}

SearchStep
*LinearisePhrasesOperation::createSearchStep(
	const DocumentState &doc
) const {
	const std::vector<PhraseSegmentation>
		&sentences = doc.getPhraseSegmentations();

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

	std::pair<
		PhraseSegmentation::const_iterator,
		std::vector<AnchoredPhrasePair>::iterator
	> m1;
	std::pair<
		PhraseSegmentation::const_reverse_iterator,
		std::vector<AnchoredPhrasePair>::reverse_iterator
	> m2;
	m1 = std::mismatch(os, oe, pg.begin());
	m2 = std::mismatch(
		std::reverse_iterator<PhraseSegmentation::const_iterator>(oe),
		std::reverse_iterator<PhraseSegmentation::const_iterator>(os),
		pg.rbegin()
	);

	LOG(logger_, debug, "linearise");
	for(PhraseSegmentation::const_iterator it = m1.first; it != m2.first.base(); ++it)
		LOG(logger_, debug, *it);

	start += std::distance(pg.begin(), m1.second);
	uint end = start + std::distance(m1.second, m2.second.base());

	const std::vector<FeatureFunction::State *>
		&featureStates = getFeatureStates(doc);
	SearchStep *step = new SearchStep(this, doc, featureStates);
	step->addModification(
		sentno,
		start, end,
		m1.first, m2.first.base(),
		PhraseSegmentation(m1.second, m2.second.base())
	);
	return step;
}


SwapPhrasesOperation::SwapPhrasesOperation(
	const Parameters &params
) :	logger_("SwapPhrasesOperation")
{
	swapDistanceDecay_ = params.get<Float>("swap-distance-decay");
}

SearchStep
*SwapPhrasesOperation::createSearchStep(
	const DocumentState &doc
) const {
	const std::vector<PhraseSegmentation>
		&sentences = doc.getPhraseSegmentations();

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

	const std::vector<FeatureFunction::State *>
		&featureStates = getFeatureStates(doc);
	SearchStep *step = new SearchStep(this, doc, featureStates);
	step->addModification(
		sentno,
		phrase1, phrase1 + 1,
		it1, after_it1,
		PhraseSegmentation(it2, after_it2)
	);
	step->addModification(
		sentno,
		phrase2, phrase2 + 1,
		it2, after_it2,
		PhraseSegmentation(it1, after_it1)
	);

	return step;
}


MovePhrasesOperation::MovePhrasesOperation(
	const Parameters &params
) :	logger_("MovePhrasesOperation")
{
	blockSizeDecay_ = params.get<Float>("block-size-decay");
	rightMovePreference_ = params.get<Float>("right-move-preference", Float(.5));
	rightDistanceDecay_ = params.get<Float>("right-distance-decay");
	leftDistanceDecay_ = params.get<Float>("left-distance-decay");
}

SearchStep
*MovePhrasesOperation::createSearchStep(
	const DocumentState &doc
) const {
	const std::vector<PhraseSegmentation>
		&sentences = doc.getPhraseSegmentations();

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

	const std::vector<FeatureFunction::State *>
		&featureStates = getFeatureStates(doc);
	SearchStep *step = new SearchStep(this, doc, featureStates);
	step->addModification(
		sentno,
		dest, dest,
		dest_it, dest_it,
		PhraseSegmentation(block_start, block_end)
	);
	step->addModification(
		sentno,
		start, start + block,
		block_start, block_end,
		PhraseSegmentation()
	);

	return step;
}


ResegmentOperation::ResegmentOperation(
	const Parameters &params
) :	logger_("ResegmentOperation")
{
	phraseResegmentationDecay_ = params.get<Float>("phrase-resegmentation-decay");
}

SearchStep
*ResegmentOperation::createSearchStep(const DocumentState &doc) const {
	const std::vector<PhraseSegmentation>
		&sentences = doc.getPhraseSegmentations();
	const std::vector<boost::shared_ptr<const PhrasePairCollection> >
		&phraseTranslations = getPhraseTranslations(doc);
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

	std::pair<
		PhraseSegmentation::const_iterator,
		PhraseSegmentation::const_iterator
	> m1;
	std::pair<
		PhraseSegmentation::const_reverse_iterator,
		PhraseSegmentation::const_reverse_iterator
	> m2;
	m1 = std::mismatch(os, oe, newseg.begin());
	if(m1.first == oe)
		return NULL;
	m2 = std::mismatch(
		PhraseSegmentation::const_reverse_iterator(oe),
		PhraseSegmentation::const_reverse_iterator(os),
		newseg.rbegin()
	);

	PhraseSegmentation::const_iterator it = newseg.begin();
	while(it != m1.second)
		++start, ++it;

	uint end = start;
	while(it != m2.second.base())
		++end, ++it;

	const std::vector<FeatureFunction::State *>
		&featureStates = getFeatureStates(doc);
	SearchStep *step = new SearchStep(this, doc, featureStates);
	step->addModification(
		sentno,
		start, end,
		m1.first, m2.first.base(),
		PhraseSegmentation(m1.second, m2.second.base())
	);
	return step;
}
