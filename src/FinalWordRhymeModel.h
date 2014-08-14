/*
 *  FinalWordRhymeModel.cpp
 *
 *  Copyright 2013 by Joerg Tiedemann. All rights reserved.
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

#ifndef docent_FinalWordRhymeModel_h
#define docent_FinalWordRhymeModel_h

// using namespace std;

#include <boost/regex.hpp>


class FinalWordRhymeModel : public FeatureFunction {

 private:

  void initializeRhymeModel(const Parameters &params);
  const std::vector<std::string> getRhyme(const std::string &word) const;
  const std::string getLastSyllable(const std::string &word) const;

  typedef boost::unordered_map<std::string,std::vector<std::string> > Rhymes_;
  Rhymes_ rhymes;
  uint maxRhymeDistance;
  std::vector<boost::regex> lastSyllRE;

  mutable Logger logger_;

 public:

 FinalWordRhymeModel(const Parameters &params) : logger_("FinalWordRhymeModel") {
    this->initializeRhymeModel(params);
  }

  void printPronounciationTable() const;

  virtual State *initDocument(const DocumentState &doc, Scores::iterator sbegin) const;
  virtual StateModifications *estimateScoreUpdate(const DocumentState &doc, const SearchStep &step, const State *state,
						  Scores::const_iterator psbegin, Scores::iterator sbegin) const;
  virtual StateModifications *updateScore(const DocumentState &doc, const SearchStep &step, const State *state,
					  StateModifications *estmods, Scores::const_iterator, Scores::iterator estbegin) const;
  virtual FeatureFunction::State *applyStateModifications(FeatureFunction::State *oldState, FeatureFunction::StateModifications *modif) const;
  virtual uint getNumberOfScores() const {
    return 1;
  }

  virtual void computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const;

};

#endif
