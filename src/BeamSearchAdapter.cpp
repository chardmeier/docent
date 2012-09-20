/*
 *  BeamSearchAdapter.cpp
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

#include "BeamSearchAdapter.h"
#include "PhrasePair.h"
#include "PhrasePairCollection.h"

// moses includes
#include "Hypothesis.h"
#include "Manager.h"
#include "Parameter.h"
#include "Sentence.h"
#include "StaticData.h"
#include "TranslationSystem.h"
#include "WordsRange.h"

#include <algorithm>
#include <iostream>
#include <iterator>

#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>

BeamSearchAdapter::BeamSearchAdapter(const std::string &moses_ini) :
		logger_("BeamSearchAdapter") {
	Moses::Parameter *mosesParams = new Moses::Parameter(); // will be owned by Moses::StaticData
	mosesParams->LoadParam(moses_ini);

	// Passing an empty executable path may make moses crash in certain configurations
	if(!Moses::StaticData::LoadDataStatic(mosesParams, ""))
		abort(); // TODO: should throw here
}

PhraseSegmentation BeamSearchAdapter::search(boost::shared_ptr<const PhrasePairCollection> ppairs, const std::vector<Word> &sentence) const {
	if(sentence.empty())
		return PhraseSegmentation();

	std::stringstream sntstream;
	std::copy(sentence.begin(), sentence.end() - 1, std::ostream_iterator<Word>(sntstream, " "));
	sntstream << sentence.back();

	boost::scoped_ptr<Moses::Sentence> msent(new Moses::Sentence());
	std::vector<Moses::FactorType> ftype(1, 0);
	//msent.Read(sntstream, ftype);
	msent->CreateFromString(ftype, sntstream.str(), "|");
	 
	const Moses::TranslationSystem &system =
		Moses::StaticData::Instance().GetTranslationSystem(Moses::TranslationSystem::DEFAULT);
	boost::scoped_ptr<Moses::Manager> manager(new Moses::Manager(0, *msent, Moses::StaticData::Instance().GetSearchAlgorithm(), &system));
	manager->ProcessSentence();
	const Moses::Hypothesis *hypo = manager->GetBestHypothesis();
	
	CompareAnchoredPhrasePairs comparePhrasePairs;
	typedef std::vector<AnchoredPhrasePair> PPVector;
	PPVector ppvec;
	ppairs->copyPhrasePairs(std::back_inserter(ppvec));
	std::sort(ppvec.begin(), ppvec.end(), comparePhrasePairs);
	PhraseSegmentation seg;
	if(hypo == NULL)
		LOG(logger_, error) << "No answer from moses.";
	while(hypo && hypo->GetPrevHypo() != NULL) {
		CoverageBitmap cov(sentence.size());
		const Moses::WordsRange &mrange = hypo->GetCurrSourceWordsRange();
		for(uint i = mrange.GetStartPos(); i <= mrange.GetEndPos(); i++)
			cov.set(i);
		
		PhraseData srcpd;
		const Moses::Phrase *msrcphr = hypo->GetSourcePhrase();
		for(uint i = 0; i < msrcphr->GetSize(); i++)
			srcpd.push_back(msrcphr->GetFactor(i, 0)->GetString());

		PhraseData tgtpd;
		const Moses::Phrase &mtgtphr = hypo->GetCurrTargetPhrase();
		for(uint i = 0; i < mtgtphr.GetSize(); i++)
			tgtpd.push_back(mtgtphr.GetFactor(i, 0)->GetString());

		PPVector::const_iterator it = std::lower_bound(ppvec.begin(), ppvec.end(),
			CompareAnchoredPhrasePairs::PhrasePairKey(cov, srcpd, tgtpd),
			comparePhrasePairs);
		seg.push_front(*it);

		hypo = hypo->GetPrevHypo();
	}
	
	return seg;
}
