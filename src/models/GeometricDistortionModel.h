/*
 *  GeometricDistortionModel.h
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

#ifndef docent_GeometricDistortionModel_h
#define docent_GeometricDistortionModel_h

#include "FeatureFunction.h"

class GeometricDistortionModel : public FeatureFunction {
private:
	template<class Operator>
	void scoreSegment(
		PhraseSegmentation::const_iterator from,
		PhraseSegmentation::const_iterator to,
		Scores::iterator sbegin,
		Operator op
	) const;

	Float computeDistortionDistance(
		const CoverageBitmap &m1,
		const CoverageBitmap &m2
	) const;

	Float distortionLimit_;
public:
	GeometricDistortionModel(const Parameters &params);

	virtual State *initDocument(
		const DocumentState &doc,
		Scores::iterator sbegin
	) const;
	virtual StateModifications *estimateScoreUpdate(
		const DocumentState &doc,
		const SearchStep &step,
		const State *state,
		Scores::const_iterator psbegin,
		Scores::iterator sbegin
	) const;
	virtual StateModifications *updateScore(
		const DocumentState &doc,
		const SearchStep &step,
		const State *state,
		StateModifications *estmods,
		Scores::const_iterator psbegin,
		Scores::iterator estbegin
	) const;

	virtual uint getNumberOfScores() const {
		return distortionLimit_ == -1 ? 1 : 2;
	}

	virtual void computeSentenceScores(
		const DocumentState &doc,
		uint sentno,
		Scores::iterator sbegin
	) const;
};

#endif
