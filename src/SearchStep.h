/*
 *  SearchStep.h
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

#ifndef docent_SearchStep_h
#define docent_SearchStep_h

#include "Docent.h"
#include "DocumentState.h"
#include "FeatureFunction.h"
#include "PhrasePair.h"

#include <vector>

class AcceptanceDecision;
class DecoderConfiguration;

class SearchStep {
public:
	struct Modification {
		uint sentno;
		uint from;
		uint to;
		PhraseSegmentation::const_iterator from_it;
		PhraseSegmentation::const_iterator to_it;
		PhraseSegmentation proposal;

		Modification(uint psentno, uint pfrom, uint pto,
				PhraseSegmentation::const_iterator pfrom_it,
				PhraseSegmentation::const_iterator pto_it,
				const PhraseSegmentation &pproposal) :
			sentno(psentno), from(pfrom), to(pto),
			from_it(pfrom_it), to_it(pto_it), proposal(pproposal) {}
	};
	
private:
	Logger logger_;
	const DocumentState &document_;
	DocumentGeneration generation_;
	const std::vector<FeatureFunction::State *> &featureStates_;
	const DecoderConfiguration &configuration_;
	// Many data elements are mutable because modification consolidation and score
	// computation is done lazily as required by the accessor functions. Logically
	// the accessors are still read-only since the outside world should never get
	// to see our state before consolidation/score computation.
	mutable std::vector<FeatureFunction::StateModifications *> stateModifications_;
	const StateOperation *operation_;
	mutable std::vector<Modification> modifications_; // mutable for consolidateModifications only!
	mutable bool modificationsConsolidated_;
	mutable Scores scores_;
	mutable enum ScoreState { NoScores, ScoresEstimated, ScoresComputed } scoreState_;
	
	void consolidateModifications() const;
	static bool compareModifications(const Modification &a, const Modification &b);
	void estimateScores() const;
	void computeScores() const;

public:
	SearchStep(const StateOperation *op, const DocumentState &doc, const std::vector<FeatureFunction::State *> &featureStates);
	~SearchStep();

	const StateOperation *getOperation() const {
		return operation_;
	}
	
	const std::vector<Modification> &getModifications() const {
		if(!modificationsConsolidated_)
			consolidateModifications();
		return modifications_;
	}
	
	std::vector<Modification> &getModifications() {
		if(!modificationsConsolidated_)
			consolidateModifications();
		return modifications_;
	}

	void addModification(uint sentno, uint start, uint end,
			PhraseSegmentation::const_iterator new1, PhraseSegmentation::const_iterator new2, const PhraseSegmentation &proposal) {
		modifications_.push_back(Modification(sentno, start, end, new1, new2, proposal));
		modificationsConsolidated_ = false;
	}
	
	const Scores &getScores() const {
		computeScores();
		return scores_;
	}
	
	bool isProvisionallyAcceptable(const AcceptanceDecision &accept) const;

	Float getScore() const {
		computeScores();
		return std::inner_product(scores_.begin(), scores_.end(), configuration_.getFeatureWeights().begin(), static_cast<Float>(0));
	}
	
	Float getScoreEstimate() const {
		estimateScores();
		return std::inner_product(scores_.begin(), scores_.end(), configuration_.getFeatureWeights().begin(), static_cast<Float>(0));
	}
	
	void setStateModifications(uint i, FeatureFunction::StateModifications *mod) {
		stateModifications_[i] = mod;
	}
	
	const std::vector<FeatureFunction::StateModifications *> &getStateModifications() const {
		return stateModifications_;
	}
	
	const DocumentState &getDocumentState() const {
		return document_;
	}
	
	DocumentGeneration getDocumentGeneration() const {
		return generation_;
	}
};

#endif

