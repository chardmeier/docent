/*
 *  ConsistencyQModelWord.cpp
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
#include "FeatureFunction.h"
#include "SearchStep.h"
#include "ConsistencyQModelWord.h"

#include <boost/foreach.hpp>

#include <iostream>
#include <map>

using namespace std;

struct QCount {

	QCount() : freq(0) {}

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

};

struct AlignMap {

	typedef std::map<std::string,QCount> AlignScore_;
	typedef std::map<std::string,AlignScore_> AlignPairMap_;

	AlignPairMap_ pMap;

	void swap(AlignMap& other) {
		this->pMap.swap(other.pMap);
	}


	void addAlignPair(const std::string& p1, const std::string& p2) {
		pMap[p1][p2]++;
	}

	void removeAlignPair(const std::string& p1, const std::string& p2) {
		pMap[p1][p2]--;
		if (pMap[p1][p2].empty()) {
			pMap[p1].erase(p2);
			if (pMap[p1].size() == 0) {
				pMap.erase(p1);
			}
		}
	}

	uint getAlignPairFreq(const std::string& p1, const std::string& p2) const {
		return pMap.find(p1)->second.find(p2)->second.freq;
	}

	float getAlignSpread(const std::string& p) const {
		return pMap.find(p)->second.size();
	}

	float scoreAll(const AlignMap& pm) const {
		float score = 0;
		float numPairs = 0;
		for (AlignPairMap_::const_iterator it1=pMap.begin() ; it1 != pMap.end(); it1++ ) {
			//q-VALUE = TPF / (TpS + SpT)
			AlignScore_ ps = it1->second;
			for (AlignScore_::const_iterator it2=ps.begin() ; it2 != ps.end(); it2++ ) {
				//weight the score by the number of occurences of the phrase pair
				float newScore = it2->second.freq * it2->second.freq/(it1->second.size() + pm.getAlignSpread(it2->first));
				score += newScore;
				numPairs += it2->second.freq;
			}
		}
		return score/numPairs;
	}

};




struct ConsistencyQModelWordState : public FeatureFunction::State, public FeatureFunction::StateModifications {
	ConsistencyQModelWordState(uint nsents) : oldScore(0) {}

	AlignMap s2t;
	AlignMap t2s;

	float oldScore;

	Float score() {
		oldScore = log(s2t.scoreAll(t2s));
		return oldScore;
	}

	void addPhrasePair(const AnchoredPhrasePair& app) {
		PhraseData td = app.second.get().getTargetPhrase().get();
		WordAlignment wa = app.second.get().getWordAlignment();
		for (uint i=0; i<td.size(); ++i) {
			std::string ts = td[i];
			std::string ss = "";
			for(WordAlignment::const_iterator wit = wa.begin_for_target(i);
				wit != wa.end_for_target(i); ++wit) {
				ss += app.second.get().getSourcePhrase().get()[*wit];
			}

			s2t.addAlignPair(ss,ts);
			t2s.addAlignPair(ts,ss);
		}
	}

	void removePhrasePair(const AnchoredPhrasePair& app) {
		PhraseData td = app.second.get().getTargetPhrase().get();
		WordAlignment wa = app.second.get().getWordAlignment();
		for (uint i=0; i<td.size(); ++i) {
			std::string ts = td[i];
			std::string ss = "";
			for(WordAlignment::const_iterator wit = wa.begin_for_target(i);
				wit != wa.end_for_target(i); ++wit) {
				ss += app.second.get().getSourcePhrase().get()[*wit];
			}

			s2t.removeAlignPair(ss,ts);
			t2s.removeAlignPair(ts,ss);
		}
	}

	virtual ConsistencyQModelWordState *clone() const {
		return new ConsistencyQModelWordState(*this);
	}
};


FeatureFunction::State *ConsistencyQModelWord::initDocument(const DocumentState &doc, Scores::iterator sbegin) const {
	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();

	ConsistencyQModelWordState *s = new ConsistencyQModelWordState(segs.size());
	for(uint i = 0; i < segs.size(); i++) {
		BOOST_FOREACH(const AnchoredPhrasePair &app, segs[i]) {
			s->addPhrasePair(app);
		}
	}


	*sbegin = s->score();
	return s;
}

void ConsistencyQModelWord::computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const {
	*sbegin = Float(0);
}

FeatureFunction::StateModifications *ConsistencyQModelWord::estimateScoreUpdate(const DocumentState &doc, const SearchStep &step, const State *state,
																				Scores::const_iterator psbegin, Scores::iterator sbegin) const {
	const ConsistencyQModelWordState *prevstate = dynamic_cast<const ConsistencyQModelWordState *>(state);
	ConsistencyQModelWordState *s = prevstate->clone();

	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	for(std::vector<SearchStep::Modification>::const_iterator it = mods.begin(); it != mods.end(); ++it) {

		// Do Nothing if it is a swap,s ince that don't affect this model
		if (step.getDescription().substr(0,4) != "Swap") {
			PhraseSegmentation::const_iterator from_it = it->from_it;
			PhraseSegmentation::const_iterator to_it = it->to_it;

			for (PhraseSegmentation::const_iterator pit=from_it; pit != to_it; pit++) {
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

FeatureFunction::StateModifications *ConsistencyQModelWord::updateScore(const DocumentState &doc, const SearchStep &step, const State *state,
																		FeatureFunction::StateModifications *estmods, Scores::const_iterator psbegin, Scores::iterator estbegin) const {
	return estmods;
}

FeatureFunction::State *ConsistencyQModelWord::applyStateModifications(FeatureFunction::State *oldState, FeatureFunction::StateModifications *modif) const {
	ConsistencyQModelWordState *os = dynamic_cast<ConsistencyQModelWordState *>(oldState);
	ConsistencyQModelWordState *ms = dynamic_cast<ConsistencyQModelWordState *>(modif);

	os->s2t.swap(ms->s2t);
	os->t2s.swap(ms->t2s);
	return oldState;
}
