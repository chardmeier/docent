/*
 *  SemanticSimilarityModel.cpp
 *
 *  Copyright 2012 by Joerg Tiedemann. All rights reserved.
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

#ifndef docent_SemanticSimilarityModel_h
#define docent_SemanticSimilarityModel_h

class SemanticSimilarityModel : public FeatureFunction {
private:
	mutable Logger logger_;
	std::string selectedPOS;
	std::string word2vecFile;
	uint HistorySize;

	float *M;
	char *vocab;
	long long words, size;
	uint MaxWordSize;

	void loadVectors(
		const std::string file_name
	);

public:
	SemanticSimilarityModel(const Parameters &params);
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
		Scores::const_iterator,
		Scores::iterator estbegin
	) const;

	virtual FeatureFunction::State *applyStateModifications(
		FeatureFunction::State *oldState,
		FeatureFunction::StateModifications *modif
	) const;

	virtual uint getNumberOfScores() const {
		return 1;
	}

	virtual void computeSentenceScores(
		const DocumentState &doc,
		uint sentno,
		Scores::iterator sbegin
	) const;
};

#endif
