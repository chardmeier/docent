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


struct Tag {
  Tag(std::string t) {
    tag=t;
    closing = false;
    if (tag[0] == '/'){
      tag.erase(tag.begin());
      closing = true;
    }
  };
  bool operator==(Tag a){
    if (a.closing != closing) return false;
    if (a.tag.compare(tag) != 0) return false;
    return true;
  }
  bool operator!=(Tag a){
    if (a.closing != closing) return true;
    if (a.tag.compare(tag) != 0) return true;
    return false;
  }
  bool closes(Tag a){
    if (! closing) return false;
    if (a.closing) return false;
    if (a.tag.compare(tag) != 0) return false;
    return true;
  }

  std::string tag;
  bool closing;
};



struct WellFormednessModelState : public FeatureFunction::State, public FeatureFunction::StateModifications {
  WellFormednessModelState(uint nsents)  : logger_("WellFormednessModel"), currentScore(0), requiresUpdate(false) {
    sentTags.resize(nsents);
  };

  std::vector< std::vector<Tag> > sentTags;

  mutable Logger logger_;
  Float currentScore;
  bool requiresUpdate;


  Float score() {
    
    if (requiresUpdate){
      std::vector<std::string> tags;

      uint conflictCount=0;

      // tag index vector should be sorted!
      for (uint s = 0; s != sentTags.size(); ++s){
	for (uint i = 0; i != sentTags[s].size(); ++i){

	  // treat closing tags
	  if (sentTags[s][i].closing){
	    if (tags.size() == 0){
	      conflictCount++;
	    }
	    else{
	      if (tags[tags.size()-1].compare(sentTags[s][i].tag) != 0){
		conflictCount++;
		// check if the next one would match and pop the last two tags if it does
		if (tags.size() > 1){
		  if (tags[tags.size()-2].compare(sentTags[s][i].tag) == 0){
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
	    tags.push_back(sentTags[s][i].tag);
	  }
	}
      }

      // all remaining opening tags are conflicts!
      while (tags.size() > 0){
	conflictCount++;
	tags.pop_back();
      }
      if (currentScore < -Float(conflictCount)){
	LOG(logger_, debug, "improved wellformedness score: " << currentScore << " --> " << -Float(conflictCount));
      }
      std::string tagSeq = "";
      for (uint s = 0; s != sentTags.size(); ++s){
	for (uint i = 0; i != sentTags[s].size(); ++i){
	  tagSeq += sentTags[s][i].tag + ' ';
	}
	tagSeq += "- ";
      }
      LOG(logger_, debug, "tag sequence: " << tagSeq << " (" << -Float(conflictCount) << ")");
      currentScore = -Float(conflictCount);
    }
    requiresUpdate=false;
    return currentScore;
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
    BOOST_FOREACH(const AnchoredPhrasePair &app, segs[i]) {
      BOOST_FOREACH(const std::string &w, app.second.get().getTargetPhrase().get()) {
	boost::match_results<std::string::const_iterator> result;
	if (boost::regex_match(w, result, tagRE)){
	  Tag tag(result[1]);
	  s->sentTags[i].push_back(tag);
	}
      }
    }
  }

  s->requiresUpdate = true;
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

    // check if we need to update the current tag list for the current sentence

    bool requiresUpdate = false;
    std::vector<Tag> currentTags;
    std::vector<Tag> modifTags;

    // record the tags in the current sentence in the modified region

    for (PhraseSegmentation::const_iterator pit=from_it; pit != to_it; pit++) {
      BOOST_FOREACH(const std::string &w, pit->second.get().getTargetPhrase().get()) {
	boost::match_results<std::string::const_iterator> result;
	if (boost::regex_match(w, result, tagRE)){
	  Tag tag(result[1]);
	  currentTags.push_back(tag);
	}
      }
    }

    // record the tags in the proposed modification (and compare to current tag list)

    BOOST_FOREACH(const AnchoredPhrasePair &app, it->proposal) {
      BOOST_FOREACH(const std::string &w, app.second.get().getTargetPhrase().get()) {
	boost::match_results<std::string::const_iterator> result;
	if (boost::regex_match(w, result, tagRE)){
	  Tag tag(result[1]);
	  modifTags.push_back(tag);
	  if (currentTags.size() >= modifTags.size()){
	    if (tag != currentTags[modifTags.size()-1]){
	      requiresUpdate=true;
	    }
	  }
	  else{
	    requiresUpdate=true;
	  }
	}
      }
    }

    // re-create the tag list for the ENTIRE sentence if we need to update
    // TODO: this is probably not very optimal)

    if (requiresUpdate){
      s->sentTags[sentNo].clear();
      for (PhraseSegmentation::const_iterator pit=current.begin(); pit != from_it; pit++) {
	BOOST_FOREACH(const std::string &w, pit->second.get().getTargetPhrase().get()) {
	  boost::match_results<std::string::const_iterator> result;
	  if (boost::regex_match(w, result, tagRE)){
	    Tag tag(result[1]);
	    s->sentTags[sentNo].push_back(tag);
	  }
	}
      }
      for (std::vector<Tag>::iterator tit=modifTags.begin(); tit != modifTags.end(); ++tit){
	s->sentTags[sentNo].push_back(*tit);
      }
      for (PhraseSegmentation::const_iterator pit=to_it; pit != current.end(); pit++) {
	BOOST_FOREACH(const std::string &w, pit->second.get().getTargetPhrase().get()) {
	  boost::match_results<std::string::const_iterator> result;
	  if (boost::regex_match(w, result, tagRE)){
	    Tag tag(result[1]);
	    s->sentTags[sentNo].push_back(tag);
	  }
	}
      }
      s->requiresUpdate = true;
    }

  }

  *sbegin = s->score();
  return s;
}


FeatureFunction::StateModifications *WellFormednessModel::updateScore(const DocumentState &doc, const SearchStep &step, const State *state,
		FeatureFunction::StateModifications *estmods, Scores::const_iterator psbegin, Scores::iterator estbegin) const {
	return estmods;
}

FeatureFunction::State *WellFormednessModel::applyStateModifications(FeatureFunction::State *oldState, FeatureFunction::StateModifications *modif) const {
	WellFormednessModelState *os = dynamic_cast<WellFormednessModelState *>(oldState);
	WellFormednessModelState *ms = dynamic_cast<WellFormednessModelState *>(modif);

	os->currentScore = ms->currentScore;
	os->requiresUpdate = ms->requiresUpdate;
	os->sentTags.swap(ms->sentTags);

	return oldState;
}
