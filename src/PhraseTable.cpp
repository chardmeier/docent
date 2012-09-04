/*
 *  PhraseTable.cpp
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

#include "Docent.h"

#include "DocumentState.h"
#include "PhrasePairCollection.h"
#include "PhraseTable.h"
#include "SearchStep.h"

#include "PhraseDictionaryTree.h" // from moses

#include <boost/function.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/algorithm.hpp>
#include <boost/lambda/numeric.hpp>

#include <iostream>
#include <ostream>
#include <sstream>
#include <iterator>

PhraseTable::PhraseTable(const Parameters &params, Random random) :
		logger_(logkw::channel = "PhraseTable"), random_(random) {
	filename_ = params.get<std::string>("file");
	nscores_ = params.get<uint>("nscores", 5);
	maxPhraseLength_ = params.get<uint>("max-phrase-length", 7);
	loadAlignments_ = params.get<bool>("load-alignments", false);
	annotationCount_ = params.get<uint>("annotation-count", 0);

	backend_ = new Moses::PhraseDictionaryTree(nscores_);
	backend_->UseWordAlignment(loadAlignments_);
	backend_->Read(filename_);
}

PhraseTable::~PhraseTable() {
	delete backend_;
}

inline Scores PhraseTable::scorePhraseSegmentation(const PhraseSegmentation &ps) const {
	Scores s(nscores_);
	for(PhraseSegmentation::const_iterator pit = ps.begin(); pit != ps.end(); ++pit)
		s += pit->second.get().getScores();
	return s;
}

FeatureFunction::State *PhraseTable::initDocument(const DocumentState &doc, Scores::iterator sbegin) const {
	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();
	Scores s(nscores_);
	for(std::vector<PhraseSegmentation>::const_iterator it = segs.begin(); it != segs.end(); ++it)
		s += scorePhraseSegmentation(*it);
	std::copy(s.begin(), s.end(), sbegin);
	return NULL;
}

void PhraseTable::computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const {
	const PhraseSegmentation &snt = doc.getPhraseSegmentation(sentno);
	Scores s = scorePhraseSegmentation(snt);
	std::copy(s.begin(), s.end(), sbegin);
}

FeatureFunction::StateModifications *PhraseTable::estimateScoreUpdate(const DocumentState &doc, const SearchStep &step, const State *state,
		Scores::const_iterator psbegin, Scores::iterator sbegin) const {
	Scores s(psbegin, psbegin + getNumberOfScores());
	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	for(std::vector<SearchStep::Modification>::const_iterator it = mods.begin(); it != mods.end(); ++it) {
		//uint sentno = it->get<0>();
		//uint from = it->get<1>();
		//uint to = it->get<2>();
		PhraseSegmentation::const_iterator ps_it = it->get<3>();
		PhraseSegmentation::const_iterator to_it = it->get<4>();
		const PhraseSegmentation &proposal = it->get<5>();
		
		while(ps_it != to_it) {
			s -= ps_it->second.get().getScores();
			++ps_it;
		}
		
		s += scorePhraseSegmentation(proposal);
	}
	std::copy(s.begin(), s.end(), sbegin);
	return NULL;
}

FeatureFunction::StateModifications *PhraseTable::updateScore(const DocumentState &doc, const SearchStep &step, const State *state,
		FeatureFunction::StateModifications *estmods, Scores::const_iterator psbegin, Scores::iterator estbegin) const {
	return estmods;
}

boost::shared_ptr<const PhrasePairCollection> PhraseTable::getPhrasesForSentence(const std::vector<Word> &sentence) const {
	using namespace boost::lambda;
	BOOST_LOG_SEV(logger_, verbose) << "getPhrasesForSentence " << sentence;
	boost::shared_ptr<PhrasePairCollection> ptc(new PhrasePairCollection(*this, sentence.size(), random_));	
	CoverageBitmap cov(sentence.size());

	CoverageBitmap uncovered(sentence.size());
	uncovered.set();

	for(uint i = 0; i < sentence.size(); i++) {
		Moses::PhraseDictionaryTree::PrefixPtr ptr = backend_->GetRoot();
		cov.reset();
		std::vector<Word> srcphrase;
		for(uint j = 0; j < maxPhraseLength_ && i + j < sentence.size(); j++) {
			ptr = backend_->Extend(ptr, sentence[i + j]);
			if(!ptr)
				break;

			srcphrase.push_back(sentence[i + j]);
			cov.set(i + j);

			std::vector<Moses::StringTgtCand> tgtcand;
			std::vector<std::string> alignments;
			if(loadAlignments_)
				backend_->GetTargetCandidates(ptr, tgtcand, alignments);
			else {
				backend_->GetTargetCandidates(ptr, tgtcand);
				alignments.resize(tgtcand.size());
			}
			
			if(!tgtcand.empty())
				uncovered -= cov;

			std::vector<std::string>::const_iterator ait = alignments.begin();
			for(std::vector<Moses::StringTgtCand>::const_iterator it = tgtcand.begin();
					it != tgtcand.end(); ++it, ++ait) {
				std::vector<Word> tgtphrase(it->first.size());
				std::vector<std::vector<Word> > annotations(annotationCount_,
					std::vector<Word>(it->first.size()));
				for(uint i = 0; i < it->first.size(); i++) {
					std::istringstream is(*it->first[i]);
					if(!getline(is, tgtphrase[i], '|')) {
						BOOST_LOG_SEV(logger_, error) << "Problem parsing target phrase: "
							<< *it->first[i];
						BOOST_THROW_EXCEPTION(FileFormatException());
					}
					for(uint j = 0; j < annotationCount_; j++)
						if(!getline(is, annotations[j][i], '|')) {
							BOOST_LOG_SEV(logger_, error) << "Problem parsing target phrase: "
								<< *it->first[i];
							BOOST_THROW_EXCEPTION(FileFormatException());
						}
				}
				std::vector<Phrase> annotationPhrases;
				annotationPhrases.reserve(annotationCount_);
				for(uint i = 0; i < annotationCount_; i++)
					annotationPhrases.push_back(Phrase(annotations[i]));

				assert(it->second.size() == nscores_);
				Scores s;
				std::transform(it->second.begin(), it->second.end(), std::back_inserter(s), bind(log, _1));
				WordAlignment wa(srcphrase.size(), tgtphrase.size(), *ait);
				PhrasePair pp(PhrasePairData(*this, srcphrase, tgtphrase, annotationPhrases, wa, s));
				ptc->addPhrasePair(cov, pp);
			}
		}
	}
	
	// add OOV phrase pairs
	for(CoverageBitmap::size_type i = uncovered.find_first(); i != CoverageBitmap::npos; i = uncovered.find_next(i)) {
		cov.reset();
		cov.set(i);
		ptc->addPhrasePair(cov, PhrasePair(*this, sentence[i], Scores(nscores_, 0)));
	}

	return ptc;
}

