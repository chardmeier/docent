/*
 *  BracketingModel.cpp
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
#include "BracketingModel.h"

#include <boost/unordered_map.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/regex.hpp>

#include <cmath>
#include <fstream>
#include <sstream>

struct BracketingModelState : public FeatureFunction::State, public FeatureFunction::StateModifications {
  BracketingModelState(uint nsents)  : logger_("BracketingModel") {}

  typedef boost::unordered_map<std::string,uint> TagCounts_;
  typedef boost::unordered_map<std::string,std::string> TagList_;

  TagCounts_ opentagcount;
  TagCounts_ closetagcount;

  TagList_ taglist;

  mutable Logger logger_;

  Float score() const {

    uint diff = 0;
    for(TagList_::const_iterator it = taglist.begin(); it != taglist.end(); ++it){
      uint open = 0;
      uint closed = 0;

      TagCounts_::const_iterator tagIt1 = opentagcount.find(it->first);
      if (tagIt1 != opentagcount.end()){
	open = tagIt1->second;
      }
      TagCounts_::const_iterator tagIt2 = opentagcount.find(it->second);
      if (tagIt2 != opentagcount.end()){
	closed = tagIt2->second;
      }
      diff += abs(open - closed);

      /*
      if (diff > 0){
	LOG(logger_, debug, "tag difference between " << it->first << " and " << it->second << " = " << diff);
      }
      */

    }
    return -Float(diff);
  }

  virtual BracketingModelState *clone() const {
    return new BracketingModelState(*this);
  }
};






BracketingModel::BracketingModel(const Parameters &params) :
  logger_("BracketingModel") {

  std::string tagfile = params.get<std::string>("tags");
  std::ifstream tagstr(tagfile.c_str());
  std::string line;
  while(getline(tagstr, line)) {
    std::istringstream ss(line);
    std::string opentag;
    std::string closetag;
    ss >> opentag >> closetag;
    opentaglist[opentag] = closetag;
    closetaglist[closetag] = opentag;

    LOG(logger_, debug, "tag pair: " << opentag << " - " << closetag);
  }
}



FeatureFunction::State *BracketingModel::initDocument(const DocumentState &doc, Scores::iterator sbegin) const {
  const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();

  BracketingModelState *s = new BracketingModelState(segs.size());

  static const boost::regex opentagRE("\\[(.*)\\]");
  static const boost::regex closetagRE("\\[\\/(.*)\\]");

  for(TagList_::const_iterator it = opentaglist.begin(); it != opentaglist.end(); ++it){
    // s->taglist.insert(it->first,it->second);
    s->taglist[it->first] = it->second;
  }
  
  for(uint i = 0; i < segs.size(); i++) {
    BOOST_FOREACH(const AnchoredPhrasePair &app, segs[i]) {
	BOOST_FOREACH(const std::string &w, app.second.get().getTargetPhrase().get()) {
	  if (opentaglist.find(w) != opentaglist.end()){
	    s->opentagcount[w]++;
	  }
	  // TODO: is it OK that tags can be counted as both (opening and closing tag?)
	  if (closetaglist.find(w) != closetaglist.end()){
	    s->closetagcount[w]++;
	  }

	  boost::match_results<std::string::const_iterator> result1;
	  boost::match_results<std::string::const_iterator> result2;
	  if (boost::regex_match(w, result2, closetagRE)){
	    s->closetagcount[result2[1]]++;
	  }
	  else{
	    if (boost::regex_match(w, result1, opentagRE)){
	      s->opentagcount[result1[1]]++;
	    }
	  }
	}
    }	 
  }

  *sbegin = s->score();
  return s;
}

void BracketingModel::computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const {
	*sbegin = Float(0);
}



FeatureFunction::StateModifications *BracketingModel::estimateScoreUpdate(const DocumentState &doc, const SearchStep &step, const State *state,
		Scores::const_iterator psbegin, Scores::iterator sbegin) const {
  
  static const boost::regex opentagRE("\\[(.*)\\]");
  static const boost::regex closetagRE("\\[\\/(.*)\\]");

	const BracketingModelState *prevstate = dynamic_cast<const BracketingModelState *>(state);
	BracketingModelState *s = prevstate->clone();

	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	for(std::vector<SearchStep::Modification>::const_iterator it = mods.begin(); it != mods.end(); ++it) {

		// Do Nothing if it is a swap,s ince that don't affect this model
		if (step.getDescription().substr(0,4) != "Swap") {

		  PhraseSegmentation::const_iterator from_it = it->from_it;
		  PhraseSegmentation::const_iterator to_it = it->to_it;

		  for (PhraseSegmentation::const_iterator pit=from_it; pit != to_it; pit++) {
		    BOOST_FOREACH(const std::string &w, pit->second.get().getTargetPhrase().get()) {
		      if (opentaglist.find(w) != opentaglist.end()){
			s->opentagcount[w]--;
			LOG(logger_, debug, "removed opening tag " << w << " (" << s->opentagcount[w] << ")");
		      }
		      // TODO: is it OK that tags can be counted as both (opening and closing tag?)
		      if (closetaglist.find(w) != closetaglist.end()){
			s->closetagcount[w]--;
			LOG(logger_, debug, "removed closing tag " << w << " (" << s->closetagcount[w] << ")");
		      }
		      boost::match_results<std::string::const_iterator> result1;
		      boost::match_results<std::string::const_iterator> result2;
		      if (boost::regex_match(w, result2, closetagRE)){
			s->closetagcount[result2[1]]--;
			LOG(logger_, debug, "removed closing tag " << w << " (" << s->closetagcount[w] << ")");
		      }
		      else{
			if (boost::regex_match(w, result1, opentagRE)){
			  s->opentagcount[result1[1]]--;
			  LOG(logger_, debug, "removed opening tag " << w << " (" << s->opentagcount[w] << ")");
			}
		      }
		    }
		  }

		
		  BOOST_FOREACH(const AnchoredPhrasePair &app, it->proposal) {
		    BOOST_FOREACH(const std::string &w, app.second.get().getTargetPhrase().get()) {
		      if (opentaglist.find(w) != opentaglist.end()){
			s->opentagcount[w]++;
			LOG(logger_, debug, "added opening tag " << w << " (" << s->opentagcount[w] << ")");
		      }
		      // TODO: is it OK that tags can be counted as both (opening and closing tag?)
		      if (closetaglist.find(w) != closetaglist.end()){
			s->closetagcount[w]++;
			LOG(logger_, debug, "added closing tag " << w << " (" << s->closetagcount[w] << ")");
		      }
		      boost::match_results<std::string::const_iterator> result1;
		      boost::match_results<std::string::const_iterator> result2;
		      if (boost::regex_match(w, result2, closetagRE)){
			s->closetagcount[result2[1]]++;
			LOG(logger_, debug, "added closing tag " << w << " (" << s->closetagcount[w] << ")");
		      }
		      else{
			if (boost::regex_match(w, result1, opentagRE)){
			  s->opentagcount[result1[1]]++;
			  LOG(logger_, debug, "added opening tag " << w << " (" << s->opentagcount[w] << ")");
			}
		      }
		    }
		  }

		}
	}

	*sbegin = s->score();
	return s;
}

FeatureFunction::StateModifications *BracketingModel::updateScore(const DocumentState &doc, const SearchStep &step, const State *state,
		FeatureFunction::StateModifications *estmods, Scores::const_iterator psbegin, Scores::iterator estbegin) const {
	return estmods;
}

FeatureFunction::State *BracketingModel::applyStateModifications(FeatureFunction::State *oldState, FeatureFunction::StateModifications *modif) const {
	BracketingModelState *os = dynamic_cast<BracketingModelState *>(oldState);
	BracketingModelState *ms = dynamic_cast<BracketingModelState *>(modif);

	os->opentagcount.swap(ms->opentagcount);
	os->closetagcount.swap(ms->closetagcount);
	os->taglist.swap(ms->taglist);

	return oldState;
}
