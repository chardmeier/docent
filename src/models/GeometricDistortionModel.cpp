/*
 *  GeometricDistortionModel.cpp
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
#include "models/GeometricDistortionModel.h"

#include <algorithm>
#include <limits>
#include <vector>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

GeometricDistortionModel::GeometricDistortionModel(
	const Parameters &params
) {
	distortionLimit_ = params.get<Float>(
		"distortion-limit",
		std::numeric_limits<Float>::infinity()
	);
}

inline Float GeometricDistortionModel::computeDistortionDistance(
	const CoverageBitmap &m1,
	const CoverageBitmap &m2
) const {
	uint fp1 = m1.find_first();
	CoverageBitmap::size_type fp1n;
	while((fp1n = m1.find_next(fp1)) != CoverageBitmap::npos)
		fp1 = fp1n;
	uint fp2 = m2.find_first();
	return static_cast<Float>(-abs(fp2 - (fp1 + 1)));
}

template<class Operator>
inline void GeometricDistortionModel::scoreSegment(
	PhraseSegmentation::const_iterator from,
	PhraseSegmentation::const_iterator to,
	Scores::iterator sbegin, Operator op
) const {
	if(from == to)
		return;

	PhraseSegmentation::const_iterator it1 = from;
	PhraseSegmentation::const_iterator it2 = from;
	++it2;
	while(it2 != to) {
		Float dist = computeDistortionDistance(it1->first, it2->first);
		*sbegin = op(*sbegin, dist);
		if(-dist > distortionLimit_)
			(*(sbegin + 1)) = op(*(sbegin + 1), Float(-1));
		it1 = it2;
		++it2;
	}
}

FeatureFunction::State
*GeometricDistortionModel::initDocument(
	const DocumentState &doc,
	Scores::iterator sbegin
) const {
	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();
	std::fill_n(sbegin, getNumberOfScores(), .0);
	for(uint i = 0; i < segs.size(); i++)
		scoreSegment(segs[i].begin(), segs[i].end(), sbegin, std::plus<Float>());
	return NULL;
}

void GeometricDistortionModel::computeSentenceScores(
	const DocumentState &doc,
	uint sentno,
	Scores::iterator sbegin
) const {
	const PhraseSegmentation &seg = doc.getPhraseSegmentation(sentno);
	std::fill_n(sbegin, getNumberOfScores(), Float(0));
	scoreSegment(seg.begin(), seg.end(), sbegin, std::plus<Float>());
}

FeatureFunction::StateModifications
*GeometricDistortionModel::estimateScoreUpdate(
	const DocumentState &doc,
	const SearchStep &step,
	const State *state,
	Scores::const_iterator psbegin,
	Scores::iterator sbegin
) const {
	std::copy(psbegin, psbegin + getNumberOfScores(), sbegin);
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
		const PhraseSegmentation &oldseg = doc.getPhraseSegmentation(sentno);

		if(!proposal.empty()) {
			if(from_it != oldseg.begin()) {
				--from_it;
				Float dist = computeDistortionDistance(from_it->first, proposal.front().first);
				*sbegin += dist;
				if(-dist > distortionLimit_)
					(*(sbegin + 1))--;
			}

			if(to_it != oldseg.end()) {
				Float dist = computeDistortionDistance(proposal.back().first, to_it->first);
				*sbegin += dist;
				if(-dist > distortionLimit_)
					(*(sbegin + 1))--;
				++to_it;
			}
		} else {
			if(from_it != oldseg.begin())
				--from_it;
			if(from_it != oldseg.begin() && to_it != oldseg.end()) {
				Float dist = computeDistortionDistance(from_it->first, to_it->first);
				*sbegin += dist;
				if(-dist > distortionLimit_)
					(*(sbegin + 1))--;
			}
			if(to_it != oldseg.end())
				++to_it;
		}

		scoreSegment(from_it, to_it, sbegin, std::minus<Float>());
		scoreSegment(proposal.begin(), proposal.end(), sbegin, std::plus<Float>());
	}
	return NULL;
}

FeatureFunction::StateModifications
*GeometricDistortionModel::updateScore(
	const DocumentState &doc,
	const SearchStep &step,
	const State *state,
	FeatureFunction::StateModifications *estmods,
	Scores::const_iterator psbegin,
	Scores::iterator estbegin
) const {
	return estmods;
}
