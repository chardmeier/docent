/*
 *  WellFormednessModel.cpp
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

#include "Docent.h"
#include "DocumentState.h"
#include "FeatureFunction.h"
#include "SearchStep.h"
#include "WellFormednessModel.h"

#include <boost/unordered_map.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/regex.hpp>

#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>


struct TagIndex {
  TagIndex(std::string t, uint s, uint p, uint w) {
    tag=t;
    closing = false;
    if (tag[0] == '/'){
      tag.erase(tag.begin());
      closing = true;
    }
    sentNo=s;
    phraseNo=p;
    wordNo=w;
  };

  std::string tag;
  bool closing;
  uint sentNo;
  uint phraseNo;
  uint wordNo;
};



struct WellFormednessModelState : public FeatureFunction::State, public FeatureFunction::StateModifications {
  WellFormednessModelState(uint nsents)  : logger_("WellFormednessModel"), currentScore(0), requiresUpdate(false) { };

  std::vector<TagIndex> tagindex;

  mutable Logger logger_;
  Float currentScore;
  bool requiresUpdate;


  Float score() {
    
    if (requiresUpdate){
      std::vector<std::string> tags;

      uint conflictCount=0;
      // tag index vector should be sorted!
      for (uint i = 0; i != tagindex.size(); ++i){

	// treat closing tags
	if (tagindex[i].closing){
	  if (tags.size() == 0){
	    conflictCount++;
	  }
	  else{
	    if (tags[tags.size()].compare(tagindex[i].tag) != 0){
	      conflictCount++;
	      // check if the next one would match and pop the last two tags if it does
	      if (tags.size() > 1){
		if (tags[tags.size()-1].compare(tagindex[i].tag) == 0){
		  tags.pop_back();
		  tags.pop_back();
		}
	      }
	    }
	    else{
	      tags.pop_back();
	    }
	  }
	}

	// treat opening tags
	else{
	  tags.push_back(tagindex[i].tag);
	}
      }

      // all remaining opening tags are conflicts!
      while (tags.size() > 0){
	conflictCount++;
	tags.pop_back();
      }
      currentScore = -Float(conflictCount);
    }
    requiresUpdate=false;
    return currentScore;
  }


  bool CompareTagPosition (const TagIndex &a, const TagIndex &b){
    if (a.sentNo < b.sentNo) return true;
    if (a.sentNo > b.sentNo) return false;
    if (a.phraseNo < b.phraseNo) return true;
    if (a.phraseNo > b.phraseNo) return false;
    if (a.wordNo < b.wordNo) return true;
    if (a.wordNo > b.wordNo) return false;
    return false;
  }

  bool MatchTag (const TagIndex &a, const TagIndex &b){
    if (a.sentNo != b.sentNo) return false;
    if (a.phraseNo != b.phraseNo) return false;
    if (a.wordNo != b.wordNo) return false;
    if (a.tag.compare(b.tag) != 0) return false;
    return true;
  }


  void AddTag(TagIndex &tag) {
    for (uint i = 0; i != tagindex.size(); ++i){
      if (CompareTagPosition(tagindex[i],tag)){
	tagindex.insert(tagindex.begin()+i,tag);
	requiresUpdate = true;
	return;
      }
    }
    tagindex.push_back(tag);
    requiresUpdate = true;
  }

  void RemoveTag(TagIndex &tag) {
    for (uint i = 0; i != tagindex.size(); ++i){
      if (MatchTag(tagindex[i],tag)){
	tagindex.erase(tagindex.begin()+i);
	requiresUpdate = true;
	return;
      }
    }
    // ..... this should not happen! add error message!
  }


  virtual WellFormednessModelState *clone() const {
    return new WellFormednessModelState(*this);
  }
};






WellFormednessModel::WellFormednessModel(const Parameters &params) : logger_("WellFormednessModel") { }


FeatureFunction::State *WellFormednessModel::initDocument(const DocumentState &doc, Scores::iterator sbegin) const {
  const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();

  WellFormednessModelState *s = new WellFormednessModelState(segs.size());

  static const boost::regex tagRE("\\[(.*)\\]");

  for(uint i = 0; i < segs.size(); i++) {
    uint phraseCount=0;
    BOOST_FOREACH(const AnchoredPhrasePair &app, segs[i]) {
      uint wordCount=0;
      BOOST_FOREACH(const std::string &w, app.second.get().getTargetPhrase().get()) {
	boost::match_results<std::string::const_iterator> result;
	if (boost::regex_match(w, result, tagRE)){
	  TagIndex tag(result[1],i,phraseCount,wordCount);
	  s->tagindex.push_back(tag);
	}
	wordCount++;
      }
      phraseCount++;
    }
  }

  *sbegin = s->score();
  return s;
}

void WellFormednessModel::computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const {
	*sbegin = Float(0);
}



FeatureFunction::StateModifications *WellFormednessModel::estimateScoreUpdate(const DocumentState &doc, const SearchStep &step, const State *state,
		Scores::const_iterator psbegin, Scores::iterator sbegin) const {
  
  static const boost::regex tagRE("\\[(.*)\\]");

  const WellFormednessModelState *prevstate = dynamic_cast<const WellFormednessModelState *>(state);
  WellFormednessModelState *s = prevstate->clone();

  const std::vector<SearchStep::Modification> &mods = step.getModifications();
  for(std::vector<SearchStep::Modification>::const_iterator it = mods.begin(); it != mods.end(); ++it) {

    PhraseSegmentation::const_iterator from_it = it->from_it;
    PhraseSegmentation::const_iterator to_it = it->to_it;

    uint sentNo = it->sentno;
    const PhraseSegmentation &current = doc.getPhraseSegmentation(sentNo);
    uint startPhrase = std::distance(current.begin(), from_it);

    uint phraseNo = startPhrase;
    uint wordNo = 0;
    for (PhraseSegmentation::const_iterator pit=from_it; pit != to_it; pit++) {
      BOOST_FOREACH(const std::string &w, pit->second.get().getTargetPhrase().get()) {
	boost::match_results<std::string::const_iterator> result;
	if (boost::regex_match(w, result, tagRE)){
	  TagIndex tag(result[1],sentNo,phraseNo,wordNo);
	  s->RemoveTag(tag);
	  LOG(logger_, debug, "removed tag " << w);
	}
	wordNo++;
      }
      phraseNo++;
    }

    phraseNo = startPhrase;
    wordNo = 0;

    BOOST_FOREACH(const AnchoredPhrasePair &app, it->proposal) {
      BOOST_FOREACH(const std::string &w, app.second.get().getTargetPhrase().get()) {
	boost::match_results<std::string::const_iterator> result;
	if (boost::regex_match(w, result, tagRE)){
	  TagIndex tag(result[1],sentNo,phraseNo,wordNo);
	  s->AddTag(tag);
	  LOG(logger_, debug, "added tag " << w);
	}
	wordNo++;
      }
      phraseNo++;
    }

    *sbegin = s->score();
    return s;
  }
}


FeatureFunction::StateModifications *WellFormednessModel::updateScore(const DocumentState &doc, const SearchStep &step, const State *state,
		FeatureFunction::StateModifications *estmods, Scores::const_iterator psbegin, Scores::iterator estbegin) const {
	return estmods;
}

FeatureFunction::State *WellFormednessModel::applyStateModifications(FeatureFunction::State *oldState, FeatureFunction::StateModifications *modif) const {
	WellFormednessModelState *os = dynamic_cast<WellFormednessModelState *>(oldState);
	WellFormednessModelState *ms = dynamic_cast<WellFormednessModelState *>(modif);

	ms->currentScore = os->currentScore;
	ms->requiresUpdate = os->requiresUpdate;
	os->tagindex.swap(ms->tagindex);

	return oldState;
}
