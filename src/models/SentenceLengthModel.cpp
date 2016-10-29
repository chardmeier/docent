/*
 *  SentenceLengthModel.cpp
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

#include "DocumentState.h"
#include "SearchStep.h"
#include "models/CountingModels.h"
#include "models/SentenceLengthModel.h"

#include <algorithm>
#include <vector>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

SentenceLengthModel::SentenceLengthModel(
	const Parameters &params
) {
	Float identicalProb = params.get<Float>("prob-identical");
	Float shortProb = params.get<Float>("prob-short");

	identicalLogprob_ = std::log(identicalProb);
	shortLogprob_ = std::log(shortProb);
	longLogprob_ = std::log(Float(1) - identicalProb - shortProb);

	shortP_ = params.get<Float>("decay-short");
	longP_ = params.get<Float>("decay-long");
}

FeatureFunction::State
*SentenceLengthModel::initDocument(
	const DocumentState &doc,
	Scores::iterator sbegin
) const {
	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();
	Float &s = *sbegin;
	s = Float(0);
	for(uint i = 0; i < segs.size(); i++)
		s += score(doc.getInputSentenceLength(i), countTargetWords(segs[i]));
	return NULL;
}

FeatureFunction::StateModifications
*SentenceLengthModel::estimateScoreUpdate(
	const DocumentState &doc,
	const SearchStep &step,
	const State *state,
	Scores::const_iterator psbegin,
	Scores::iterator sbegin
) const {
	using namespace boost::lambda;
	Float &s = *sbegin;
	s = *psbegin;
	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	for(std::vector<SearchStep::Modification>::const_iterator
		it = mods.begin();
		it != mods.end();
		++it
	) {
		uint sentno = it->sentno;
		PhraseSegmentation::const_iterator from_it = it->from_it;
		PhraseSegmentation::const_iterator to_it = it->to_it;
		const PhraseSegmentation &proposal = it->proposal;

		Float outlen = Float(countTargetWords(doc.getPhraseSegmentation(sentno)));
		s -= score(doc.getInputSentenceLength(sentno), outlen);

		std::for_each(
			from_it, to_it,
			outlen -= boost::lambda::bind(WordPenaltyCounter(), _1)
		);
		std::for_each(
			proposal.begin(), proposal.end(),
			outlen += boost::lambda::bind(WordPenaltyCounter(), _1)
		);
		s += score(doc.getInputSentenceLength(sentno), outlen);
	}
	return NULL;
}

FeatureFunction::StateModifications
*SentenceLengthModel::updateScore(
	const DocumentState &doc,
	const SearchStep &step,
	const State *state,
	FeatureFunction::StateModifications *estmods,
	Scores::const_iterator psbegin,
	Scores::iterator sbegin
) const {
	return estmods;
}

void SentenceLengthModel::computeSentenceScores(
	const DocumentState &doc,
	uint sentno,
	Scores::iterator sbegin
) const {
	*sbegin = score(
		doc.getInputSentenceLength(sentno),
		countTargetWords(doc.getPhraseSegmentation(sentno))
	);
}

Float SentenceLengthModel::score(
	Float inputlen,
	Float outputlen
) const {
	const Float eps = Float(.1); // should we be dealing in uints here? ...
	Float diff = std::abs(inputlen - outputlen);
	if(diff < eps)
		return identicalLogprob_;
	else if(outputlen < inputlen)
		return shortLogprob_
			+ (diff - Float(1)) * std::log(Float(1) - shortP_)
			+ std::log(shortP_);
	else
		return longLogprob_
			+ (diff - Float(1)) * std::log(Float(1) - longP_)
			+ std::log(longP_);
}
