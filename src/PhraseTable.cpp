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

#include "PhraseTable.h"

#include "DocumentState.h"
#include "PhrasePairCollection.h"
#include "SearchStep.h"

#include "util/file_piece.hh"  // from KenLM
#include "util/file.hh"
#include "util/string_piece.hh"
#include "util/probing_hash_table.hh"

#include "quering.hh"  // from ProbingPT

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/algorithm.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/numeric.hpp>

#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <iterator>


PhraseTable::PhraseTable(
	const Parameters &params,
	Random random
) :	logger_("PhraseTable"),
	random_(random)
{
	filename_ = params.get<std::string>("file");
	nscores_ = params.get<uint>("nscores", 5);
	maxPhraseLength_ = params.get<uint>("max-phrase-length", 7);
	annotationCount_ = params.get<uint>("annotation-count", 0);

	if(!boost::filesystem::is_directory(filename_)) {
		LOG(logger_, error, "phrase table not found: '" << filename_ << "'");
		BOOST_THROW_EXCEPTION(FileFormatException());
	}
	try {
		backend_ = new QueryEngine(filename_.c_str());
	} catch(std::exception &) {
		LOG(logger_, error, "incomplete or invalid phrase table in '" << filename_ << "'");
		BOOST_THROW_EXCEPTION(FileFormatException());
	}
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

FeatureFunction::State
*PhraseTable::initDocument(
	const DocumentState &doc,
	Scores::iterator sbegin
) const {
	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();
	Scores s(nscores_);
	for(std::vector<PhraseSegmentation>::const_iterator it = segs.begin(); it != segs.end(); ++it)
		s += scorePhraseSegmentation(*it);
	std::copy(s.begin(), s.end(), sbegin);
	return NULL;
}

void PhraseTable::computeSentenceScores(
	const DocumentState &doc,
	uint sentno,
	Scores::iterator sbegin
) const {
	const PhraseSegmentation &snt = doc.getPhraseSegmentation(sentno);
	Scores s = scorePhraseSegmentation(snt);
	std::copy(s.begin(), s.end(), sbegin);
}

FeatureFunction::StateModifications
*PhraseTable::estimateScoreUpdate(
	const DocumentState &doc,
	const SearchStep &step,
	const State *state,
	Scores::const_iterator psbegin,
	Scores::iterator sbegin
) const {
	Scores s(psbegin, psbegin + getNumberOfScores());
	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	for(std::vector<SearchStep::Modification>::const_iterator
		it = mods.begin();
		it != mods.end();
		++it
	) {
		PhraseSegmentation::const_iterator ps_it = it->from_it;
		PhraseSegmentation::const_iterator to_it = it->to_it;
		const PhraseSegmentation &proposal = it->proposal;

		while(ps_it != to_it) {
			s -= ps_it->second.get().getScores();
			++ps_it;
		}

		s += scorePhraseSegmentation(proposal);
	}
	std::copy(s.begin(), s.end(), sbegin);
	return NULL;
}

FeatureFunction::StateModifications *PhraseTable::updateScore(
	const DocumentState &doc,
	const SearchStep &step,
	const State *state,
	FeatureFunction::StateModifications *estmods,
	Scores::const_iterator psbegin,
	Scores::iterator estbegin
) const {
	return estmods;
}


boost::shared_ptr<const PhrasePairCollection>
PhraseTable::getPhrasesForSentence(
	const std::vector<Word> &sentence
) const
{
	using namespace boost::lambda;
	LOG(logger_, verbose, "getPhrasesForSentence " << sentence);
	boost::shared_ptr<PhrasePairCollection> ptc(
		new PhrasePairCollection(sentence.size(), random_)
	);
	std::pair<bool, std::vector<target_text> > query_result;
	std::map<uint, std::string> vocabids = backend_->getVocab();

	CoverageBitmap cov(sentence.size());
	CoverageBitmap uncovered(sentence.size());
	uncovered.set();

	for(uint i = 0; i < sentence.size(); ++i) {
		cov.reset();
		std::vector<Word> srcphrase;
		std::ostringstream src("");
		for(uint j = 0;
			j < maxPhraseLength_ && i + j < sentence.size();
			++j
		) {
			if(src.tellp() > 0)
				src << ' ';
			src << sentence[i + j];

			query_result = backend_->query(StringPiece(src.str()));
			srcphrase.push_back(sentence[i + j]);
			cov.set(i + j);

			if(!query_result.first)
				continue;

			std::vector<target_text> finds(query_result.second);
			BOOST_FOREACH(target_text find, finds) {  // All PHRASES
				std::string phrase(getTargetWordsFromIDs(find.target_phrase, &vocabids));
				boost::trim(phrase);
				std::vector<Word> tokens;
				boost::split(tokens, phrase, boost::is_any_of(" "));
				LOG(logger_, verbose, i << "/" << j << " > " << tokens.size() << " TOKENS: [" << tokens << "]");

				std::vector< std::vector<Word> > factors(
					(annotationCount_ + 1),
					std::vector<Word>(tokens.size())
				);
				uint k = 0;
				BOOST_FOREACH(Word token, tokens) {  // All TOKENS (word|annotation1|...)
					std::vector<Word> curr_factors;
					boost::split(curr_factors, token, boost::is_any_of("|"));
					uint l = 0;
					for(std::vector<Word>::iterator
						it = curr_factors.begin();
						it != curr_factors.end();
						++it, ++l
					) {
						if((l == 0) && (it->length() == 0)) {
							LOG(logger_, error, "Problem, empty target phrase: " << token);
							BOOST_THROW_EXCEPTION(FileFormatException());
						}
						if(l > annotationCount_) {
							LOG(logger_, error, "Problem, too many annotations: " << token);
							BOOST_THROW_EXCEPTION(FileFormatException());
						}
						factors[l][k] = *it;
					}
					if(l < (annotationCount_ + 1)) {
						LOG(logger_, error, "Problem, too few annotations: " << token);
						BOOST_THROW_EXCEPTION(FileFormatException());
					}
					++k;
				}

				std::vector<Phrase> annotationPhrases;
				annotationPhrases.reserve(annotationCount_);
				for(uint l = 0; l < annotationCount_; ++l) {
					annotationPhrases.push_back(Phrase(factors[l+1]));
				}

				// Alignment
				std::vector<AlignmentPair> alignments;
				alignments.reserve(find.word_all1.size()/2);
				for(uint l = 0; l < find.word_all1.size(); l = l+2) {
					alignments.push_back(
						AlignmentPair(find.word_all1[l], find.word_all1[l+1])
					);
				}
				WordAlignment wa(srcphrase.size(), tokens.size(), alignments);

				// Scores
				Scores scores;
				BOOST_FOREACH(Float prob, find.prob)
					scores.push_back(std::log(prob));

				PhrasePair pp(PhrasePairData(
					srcphrase, factors[0], annotationPhrases, wa, scores
				));
				ptc->addPhrasePair(cov, pp);
				uncovered -= cov;
			}
		}
	}

	// add OOV phrase pairs
	for(CoverageBitmap::size_type i = uncovered.find_first();
		i != CoverageBitmap::npos;
		i = uncovered.find_next(i)
	) {
		cov.reset();
		cov.set(i);
		ptc->addPhrasePair(cov, PhrasePair(sentence[i], Scores(nscores_, 0)));
	}

	return ptc;
}
