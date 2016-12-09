/*
 *  FeatureFunction.h
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

#ifndef docent_FeatureFunction_h
#define docent_FeatureFunction_h

#include "Docent.h"
#include "DecoderConfiguration.h"

#include <boost/shared_ptr.hpp>

class DocumentState;
class SearchStep;

/***********************************************************************
 * API for all FeatureFunctions
 **/

class FeatureFunction {
public:
	class State {
	protected:
		State() {}
		State(const State &o) {}
	public:
		virtual State *clone() const = 0;
		virtual ~State() {}
	};

	class StateModifications {
	protected:
		StateModifications() {}
		StateModifications(const StateModifications &o) {}
	public:
		virtual ~StateModifications() {}
	};

	virtual ~FeatureFunction() {}

	virtual FeatureFunction::State
	*initDocument(
		const DocumentState &doc,
		Scores::iterator sbegin
	) const = 0;

	virtual FeatureFunction::StateModifications
	*estimateScoreUpdate(
		const DocumentState &doc,
		const SearchStep &step,
		const FeatureFunction::State *state,
		Scores::const_iterator psbegin,
		Scores::iterator sbegin
	) const = 0;

	virtual FeatureFunction::StateModifications
	*updateScore(
		const DocumentState &doc,
		const SearchStep &step,
		const FeatureFunction::State *state,
		StateModifications *estmods,
		Scores::const_iterator psbegin,
		Scores::iterator estbegin
	) const = 0;

	virtual uint getNumberOfScores() const = 0;

	virtual FeatureFunction::State
	*applyStateModifications(
		FeatureFunction::State *oldState,
		FeatureFunction::StateModifications *modif
	) const {
		return NULL;
	}

	virtual void dumpFeatureFunctionState(
		const DocumentState &doc,
		FeatureFunction::State *state
	) const {}

	// debugging only!
	virtual void computeSentenceScores(
		const DocumentState &doc,
		uint sentno,
		Scores::iterator sbegin
	) const = 0;
};

/**
 * End of FeatureFunction API
 **********************************************************************/


class FeatureFunctionInstantiation {
private:
	std::string id_;
	uint scoreIndex_;
	boost::shared_ptr<const FeatureFunction> impl_;

public:
	FeatureFunctionInstantiation(
		const std::string &id,
		uint scoreIndex,
		boost::shared_ptr<const FeatureFunction> impl
	) :	id_(id), scoreIndex_(scoreIndex), impl_(impl) {}

	const std::string &getId() const {
		return id_;
	}

	uint getScoreIndex() const {
		return scoreIndex_;
	}

	FeatureFunction::State
	*initDocument(
		const DocumentState &doc,
		Scores::iterator sbegin
	) const {
		return impl_->initDocument(doc, sbegin);
	}

	// debugging only!
	void computeSentenceScores(
		const DocumentState &doc,
		uint sentno,
		Scores::iterator sbegin
	) const {
		impl_->computeSentenceScores(doc, sentno, sbegin);
	}

	FeatureFunction::StateModifications
	*estimateScoreUpdate(
		const DocumentState &doc,
		const SearchStep &step,
		const FeatureFunction::State *state,
		Scores::const_iterator psbegin,
		Scores::iterator sbegin
	) const {
		return impl_->estimateScoreUpdate(doc, step, state, psbegin, sbegin);
	}

	FeatureFunction::StateModifications
	*updateScore(
		const DocumentState &doc,
		const SearchStep &step,
		const FeatureFunction::State *state,
		FeatureFunction::StateModifications *estmods,
		Scores::const_iterator psbegin,
		Scores::iterator estbegin
	) const {
		return impl_->updateScore(doc, step, state, estmods, psbegin, estbegin);
	}

	uint getNumberOfScores() const {
		return impl_->getNumberOfScores();
	}

	FeatureFunction::State
	*applyStateModifications(
		FeatureFunction::State *oldState,
		FeatureFunction::StateModifications *modif
	) const {
		return impl_->applyStateModifications(oldState, modif);
	}

	void dumpFeatureFunctionState(
		const DocumentState &doc,
		FeatureFunction::State *state
	) const {
		return impl_->dumpFeatureFunctionState(doc, state);
	}
};

class FeatureFunctionFactory {
private:
	Random random_;

public:
	FeatureFunctionFactory(Random rnd) : random_(rnd) {}

	boost::shared_ptr<FeatureFunction>
	create(
		const std::string &type,
		const Parameters &params
	) const;
};

#endif
