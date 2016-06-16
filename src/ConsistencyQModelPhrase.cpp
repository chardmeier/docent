/*
 *  ConsistencyQModelPhrase.cpp
 *
 *  Copyright 2012 by Sara Stymne. All rights reserved.
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
#include "SearchStep.h"
#include "ConsistencyQModelPhrase.h"

#include <boost/foreach.hpp>

#include <map>

using namespace std;

struct QCount
{
	QCount() : freq(0), sSpread(0), tSpread(0) {}

	QCount& operator++ () {
		freq++;
		return *this;
	}

	QCount operator++ (int) {
		QCount res (*this);
		++(*this);
		return res;
	}

	QCount& operator-- () {
		freq--;
		return *this;
	}

	QCount operator-- (int) {
		QCount res (*this);
		--(*this);
		return res;
	}

	bool empty() {
		return freq <= 0;
	}

	uint freq;
	uint sSpread;
	uint tSpread;
};


struct PhraseMap
{
	typedef std::map<Phrase,QCount> PhraseScore_;
	typedef std::map<Phrase,PhraseScore_> PhrasePairMap_;

	PhrasePairMap_ pMap;

	void swap(PhraseMap& other) {
		this->pMap.swap(other.pMap);
	}


	void addPhrasePair(const Phrase& p1, const Phrase& p2) {
		pMap[p1][p2]++;
	}

	void removePhrasePair(const Phrase& p1, const Phrase& p2) {
		pMap[p1][p2]--;
		if (pMap[p1][p2].empty()) {
			pMap[p1].erase(p2);
			if (pMap[p1].size() == 0) {
				pMap.erase(p1);
			}
		}
	}

	uint getPhrasePairFreq(const Phrase& p1, const Phrase& p2) const {
		return pMap.find(p1)->second.find(p2)->second.freq;
	}

	float getPhraseSpread(const Phrase& p) const {
		return pMap.find(p)->second.size();
	}

	float scoreAll(const PhraseMap& pm) const {
		float score = 0;
		float numPhrases = 0;
		for(PhrasePairMap_::const_iterator it1=pMap.begin() ; it1 != pMap.end(); it1++ ) {
			//q-VALUE = TPF / (TpS + SpT)
			PhraseScore_ ps = it1->second;
			for(PhraseScore_::const_iterator it2=ps.begin() ; it2 != ps.end(); it2++ ) {
				//weight the score by the number of occurences of the phrase pair
				float newScore = it2->second.freq * it2->second.freq/(it1->second.size() + pm.getPhraseSpread(it2->first));
				score += newScore;
				numPhrases += it2->second.freq;
			}
		}
		return score/numPhrases;
	}
};


struct ConsistencyQModelPhraseState
:	public FeatureFunction::State,
	public FeatureFunction::StateModifications
{
	ConsistencyQModelPhraseState(uint nsents) : oldScore(0) {} // :  sentencePairs(nsents) {}

	PhraseMap s2t;
	PhraseMap t2s;

	float oldScore;

	//Not currently used - would probably be needed for doing more efficient scoring. But needs revising data structures...)
	//  std::vector<PhrasePair> sentencePairs;

	Float score() {
		oldScore = log(s2t.scoreAll(t2s));
		return oldScore;
	}

	void addPhrasePair(const AnchoredPhrasePair& app) {
		Phrase t = app.second.get().getTargetPhrase();
		Phrase s = app.second.get().getSourcePhrase();
		s2t.addPhrasePair(s,t);
		t2s.addPhrasePair(t,s);
	}

	void removePhrasePair(const AnchoredPhrasePair& app) {
		Phrase t = app.second.get().getTargetPhrase();
		Phrase s = app.second.get().getSourcePhrase();
		s2t.removePhrasePair(s,t);
		t2s.removePhrasePair(t,s);
	}

	virtual ConsistencyQModelPhraseState *clone() const {
		return new ConsistencyQModelPhraseState(*this);
	}
};


FeatureFunction::State
*ConsistencyQModelPhrase::initDocument(
	const DocumentState &doc,
	Scores::iterator sbegin
) const {
	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();

	ConsistencyQModelPhraseState *s = new ConsistencyQModelPhraseState(segs.size());
	for(uint i = 0; i < segs.size(); i++) {
		BOOST_FOREACH(const AnchoredPhrasePair &app, segs[i]) {
			s->addPhrasePair(app);
		}
	}


	*sbegin = s->score();
	return s;
}

void ConsistencyQModelPhrase::computeSentenceScores(
	const DocumentState &doc,
	uint sentno,
	Scores::iterator sbegin
) const {
	*sbegin = Float(0);
}

FeatureFunction::StateModifications
*ConsistencyQModelPhrase::estimateScoreUpdate(
	const DocumentState &doc,
	const SearchStep &step,
	const State *state,
	Scores::const_iterator psbegin,
	Scores::iterator sbegin
) const {
	const ConsistencyQModelPhraseState *prevstate =
		dynamic_cast<const ConsistencyQModelPhraseState *>(state);
	ConsistencyQModelPhraseState *s = prevstate->clone();

	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	for(std::vector<SearchStep::Modification>::const_iterator
		it = mods.begin();
		it != mods.end();
		++it
	) {
		// Do Nothing if it is a swap, since that don't affect this model
		if(step.getDescription().substr(0,4) != "Swap") {
			PhraseSegmentation::const_iterator from_it = it->from_it;
			PhraseSegmentation::const_iterator to_it = it->to_it;

			for(PhraseSegmentation::const_iterator pit=from_it; pit != to_it; pit++) {
				s->removePhrasePair(*pit);
			}

			BOOST_FOREACH(const AnchoredPhrasePair &app, it->proposal) {
				s->addPhrasePair(app);
			}
		}
	}

	*sbegin = s->score();
	return s;
}

FeatureFunction::StateModifications
*ConsistencyQModelPhrase::updateScore(
	const DocumentState &doc,
	const SearchStep &step,
	const State *state,
	FeatureFunction::StateModifications *estmods,
	Scores::const_iterator psbegin,
	Scores::iterator estbegin
) const {
	return estmods;
}

FeatureFunction::State
*ConsistencyQModelPhrase::applyStateModifications(
	FeatureFunction::State *oldState,
	FeatureFunction::StateModifications *modif
) const {
	ConsistencyQModelPhraseState *os = dynamic_cast<ConsistencyQModelPhraseState *>(oldState);
	ConsistencyQModelPhraseState *ms = dynamic_cast<ConsistencyQModelPhraseState *>(modif);

	os->s2t.swap(ms->s2t);
	os->t2s.swap(ms->t2s);
	return oldState;
}
