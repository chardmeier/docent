/*
 *  SemanticSimilarityModel.cpp
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
#include "SemanticSimilarityModel.h"
#include "MMAXDocument.h"

#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>
#include <malloc.h>

/*
  selected words:
  - phrNo = position of phrase in target language
  - wordNo = position of word in current phrase
  - srcWord = source language word
  - trgWord = target language word
  - srcNo = position or source language word in source sentence
*/

struct SelectedWordVector
{
	SelectedWordVector(
		uint pn,
		uint wn,
		std::string s,
		std::string t,
		uint sn,
		uint size
	) :	srcWord(""),
		trgWord(""),
		phrNo(0),
		wordNo(0),
		srcNo(0),
		vec(0),
		similarity(0)
	{
		phrNo = pn;
		wordNo = wn;
		srcWord = s;
		trgWord = t;
		srcNo = sn;
		vec.reserve(size);
		similarity = 0;
	};

	std::string srcWord, trgWord;
	uint phrNo, wordNo, srcNo;
	std::vector<float> vec;
	float similarity;
};


struct SemanticSimilarityModelState
:	public FeatureFunction::State,
	public FeatureFunction::StateModifications
{
	SemanticSimilarityModelState(uint nsents)
	:	logger_("SemanticSimilarityModel"),
		currentScore(0)
	{
		selectedWords.resize(nsents);
		posTags.resize(nsents);
		MaxWordSize = 50;
	};

	std::vector< std::vector< SelectedWordVector > > selectedWords;
	std::vector< std::vector< std::string > > posTags;
	std::vector<float> sim;

	float *M;
	char *vocab;
	long long words, size;
	uint MaxWordSize;

	mutable Logger logger_;
	Float currentScore;

	void LoadVectors(
		const std::string file_name
	) {
		FILE *f;
		f = fopen(file_name.c_str(), "rb");
		if(f == NULL) {
			printf("Input file not found\n");
			return;
		}
		fscanf(f, "%lld", &words);
		fscanf(f, "%lld", &size);

		M = (float *)malloc((long long)words * (long long)size * sizeof(float));
		if(M == NULL) {
			LOG(logger_, error, "Cannot allocate memory: "
				<< (long long)words * size * sizeof(float) / 1048576 << "MB, "
				<< words << "	" << size
			);
		}
		long long a, b;
		for(b = 0; b < words; b++) {
			a = 0;
			while(1) {
				vocab[b * MaxWordSize + a] = fgetc(f);
				if(feof(f) || (vocab[b * MaxWordSize + a] == ' '))
					break;
				if((a < MaxWordSize) && (vocab[b * MaxWordSize + a] != '\n'))
					a++;
			}
			vocab[b * MaxWordSize + a] = 0;
			for(a = 0; a < size; a++)
				fread(&M[a + b * size], sizeof(float), 1, f);
			float len = 0;
			for(a = 0; a < size; a++)
				len += M[a + b * size] * M[a + b * size];
			len = sqrt(len);
			for(a = 0; a < size; a++)
				M[a + b * size] /= len;
		}
		fclose(f);
	};

	long long FindVocabularyPosition(
		const std::string word
	) {
		long long b;
		for(b = 0; b < words; b++) {
			if(!strcmp(&vocab[b * MaxWordSize], word.c_str()))
				return b;
		}
		return -1;
	}

	std::vector<float> FindVector(
		const std::string word,
		std::vector<float> &vec
	) {
		vec.clear();
		long long b = FindVocabularyPosition(word);
		if(b >= 0)
			for(int a = 0; a < size; a++)
				vec.push_back(M[a + b * size]);
		return vec;
	}

	std::vector<float> FindVector(
		const long long b,
		std::vector<float> &vec
	) {
		vec.clear();
		for(int a = 0; a < size; a++)
			vec.push_back(M[a + b * size]);
		return vec;
	}

	Float score() {
		currentScore = 0;
		for(uint i = 0; i != selectedWords.size(); ++i) {
			for(uint j = 0; j != selectedWords[i].size(); ++j) {
				currentScore += selectedWords[i][j].similarity;
			}
		}
		return currentScore;
	};

	std::string GetWords(
		uint sentno
	) {
		std::string words = "";
		for(uint i=0; i<selectedWords[sentno].size(); i++) {
			words += selectedWords[sentno][i].trgWord+" ";
		}
		return words;
	}

	void GetHistory(
		std::vector<SelectedWordVector*> &history,
		uint sentNo,
		const uint size
	) {
		while(history.size() < size) {
			std::vector<SelectedWordVector>::iterator it=selectedWords[sentNo].end();
			while(it != selectedWords[sentNo].begin()) {
				it--;
				history.push_back(&(*it));
				if(history.size() == size)
					return;
			}
			if(sentNo == 0) break;
			sentNo--;
		}
	}

	void GetHistory(
		std::vector<SelectedWordVector*> &history,
		uint sentNo,
		const uint idx,
		const uint size
	) {
		std::vector<SelectedWordVector>::iterator
			it = selectedWords[sentNo].begin()+idx;
		while(history.size() < size) {
			while(it!=selectedWords[sentNo].begin()) {
				it--;
				history.push_back(&(*it));
				if(history.size() == size)
					return;
			}
			if(sentNo == 0)
				break;
			sentNo--;
			it=selectedWords[sentNo].end();
		}
	}

	void GetFuture(
		std::vector<SelectedWordVector*> &future,
		uint sentNo,
		const uint idx,
		const uint size,
		const uint stopSentNo,
		const uint stopPhrNo
	) {
		std::vector<SelectedWordVector>::iterator
			it = selectedWords[sentNo].begin()+idx;
		while(future.size() < size) {
			while(it != selectedWords[sentNo].end()) {
				if(sentNo == stopSentNo && it->phrNo == stopPhrNo)
					return;
				future.push_back(&(*it));
				if(future.size() == size)
					return;
				it++;
			}
			sentNo++;
			if(sentNo >= selectedWords.size())
				return;
			if(sentNo > stopSentNo)
				return;
			it = selectedWords[sentNo].begin();
		}
	}

	void UpdateFutureScores(
		uint sentNo,
		uint idx,
		const uint size,
		const uint stopSentNo,
		const uint stopPhrNo
	) {
		uint updated = 0;
		while(updated < size) {
			while(idx < selectedWords[sentNo].size()) {
				if(sentNo == stopSentNo && selectedWords[sentNo][idx].phrNo == stopPhrNo)
					return;
				UpdateSimilarityScore(sentNo,idx,size);
				updated++;
				idx++;
			}
			sentNo++;
			if(sentNo >= selectedWords.size())
				return;
			if(sentNo > stopSentNo)
				return;
			idx = 0;
		}
	};

	void ClearWords(
		const uint sentNo
	) {
		selectedWords[sentNo].clear();
	};

	void CopyWords(
		const SemanticSimilarityModelState& state,
		const uint sentno,
		const uint start,
		const uint end,
		const int diff
	) {
		for(uint i=0; i<state.selectedWords[sentno].size(); i++) {
			if(state.selectedWords[sentno][i].phrNo >= start) {
				if(state.selectedWords[sentno][i].phrNo < end) {
					selectedWords[sentno].push_back(state.selectedWords[sentno][i]);
					if(diff != 0)
						selectedWords[sentno][selectedWords[sentno].size()-1].phrNo += diff;
				}
			}
		}
	};

	void AddPosTag(
		const uint sentno,
		const uint wordno,
		const std::string& pos
	) {
		posTags[sentno].push_back(pos);
	};

	uint AddWord(
		const uint sentno,
		const uint phrno,
		const AnchoredPhrasePair &app,
		const std::string pos,
		const int historySize
	) {
		WordAlignment wa = app.second.get().getWordAlignment();
		PhraseData sd = app.second.get().getSourcePhrase().get();
		PhraseData td = app.second.get().getTargetPhrase().get();

		uint wordno = app.first.find_first();

		uint addCount = 0;
		for(uint j = 0; j < sd.size(); ++j) {
			// TODO: we could support other conditins here as well!
			if(posTags[sentno][wordno] == pos) {
				for(WordAlignment::const_iterator
					wit = wa.begin_for_source(j);
					wit != wa.end_for_source(j);
					++wit
				) {
					std::string wordPair = sd[j] + "_" + td[*wit];
					long long b = FindVocabularyPosition(wordPair);
					if(b < 0)
						continue;
					SelectedWordVector word(phrno,*wit,sd[j],td[*wit],wordno,size);
					FindVector(b,word.vec);
					selectedWords[sentno].push_back(word);
					selectedWords[sentno].back().similarity = MaxSimilarityWithHistory(
						sentno,
						selectedWords[sentno].size()-1,
						historySize
					);
					// currentScore += word.similarity;
					addCount++;
					//LOG(logger_, debug, "add word " << td[*wit] << " aligned to " << sd[j]);
				}
			}
			wordno++;
		}
		return addCount;
	};

	void UpdateSimilarityScore(
		const uint sentNo,
		const uint idx,
		const uint size
	) {
		selectedWords[sentNo][idx].similarity = MaxSimilarityWithHistory(
			sentNo,
			idx,
			size
		);
	};

	float MaxSimilarityWithHistory(
		uint sentNo,
		const uint idx,
		const uint size
	) {
		std::vector<SelectedWordVector*> history;
		GetHistory(history,sentNo,idx,size);
		float maxSim = 0;
		while(!history.empty()) {
			SelectedWordVector *last = history.back();
			float sim = CosinusSimilarity(
				selectedWords[sentNo][idx].vec,
				last->vec
			);
			if(sim > maxSim || maxSim == 0) maxSim = sim;
			history.pop_back();
		}
		return maxSim;
	}

	float CosinusSimilarity(
		const std::vector<float> &vec1,
		const std::vector<float> &vec2
	) {
		float len = 0;
		uint a;

		for(a = 0; a < size; a++)
			len += vec1[a] * vec1[a];
		len = sqrt(len);

		float sim = 0;
		for(a = 0; a < size; a++)
			sim += vec1[a] * vec2[a] / len;
		// return log(sim);
		return sim;
	}

	virtual SemanticSimilarityModelState *clone() const {
		return new SemanticSimilarityModelState(*this);
	}
};

SemanticSimilarityModel::SemanticSimilarityModel(
	const Parameters &params
) :	logger_("SemanticSimilarityModel")
{
	selectedPOS = params.get<std::string>("selected-pos");
	word2vecFile = params.get<std::string>("word2vec");
	HistorySize = 20;
	MaxWordSize = 50;
	loadVectors(word2vecFile);
}

void SemanticSimilarityModel::loadVectors(
	const std::string file_name
) {
	FILE *f;
	f = fopen(file_name.c_str(), "rb");
	if(f == NULL) {
		printf("Input file not found\n");
		return;
	}
	fscanf(f, "%lld", &words);
	fscanf(f, "%lld", &size);

	vocab = (char *)malloc(
		(long long)words * MaxWordSize * sizeof(char)
	);
	M = (float *)malloc(
		(long long)words * (long long)size * sizeof(float)
	);
	if(M == NULL) {
		LOG(logger_, error, "Cannot allocate memory: "
			<< (long long)words * size * sizeof(float) / 1048576 << "MB, "
			<< words << "	" << size
		);
	}
	long long a, b;
	for(b = 0; b < words; b++) {
		a = 0;
		while(1) {
			vocab[b * MaxWordSize + a] = fgetc(f);
			if(feof(f) || (vocab[b * MaxWordSize + a] == ' '))
				break;
			if((a < MaxWordSize) && (vocab[b * MaxWordSize + a] != '\n'))
				a++;
		}
		vocab[b * MaxWordSize + a] = 0;
		for(a = 0; a < size; a++)
			fread(&M[a + b * size], sizeof(float), 1, f);
		float len = 0;
		for(a = 0; a < size; a++)
			len += M[a + b * size] * M[a + b * size];
		len = sqrt(len);
		for(a = 0; a < size; a++)
			M[a + b * size] /= len;
	}
	fclose(f);
};


FeatureFunction::State
*SemanticSimilarityModel::initDocument(
	const DocumentState &doc,
	Scores::iterator sbegin
) const {
	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();

	boost::shared_ptr<const MMAXDocument> mmax = doc.getInputDocument();
	const MarkableLevel &posLevel = mmax->getMarkableLevel("pos");

	SemanticSimilarityModelState *s = new SemanticSimilarityModelState(segs.size());

	// s->LoadVectors(word2vecFile);
	s->size = size;
	s->words = words;
	s->M = M;
	s->vocab = vocab;

	// save all POS tags in the document state
	BOOST_FOREACH(const Markable &m, posLevel) {
		uint snt = m.getSentence();
		const std::string &postag = m.getAttribute("tag");
		const CoverageBitmap &cov = m.getCoverage();
		uint wordno = cov.find_first();
		s->AddPosTag(snt,wordno,postag);
		// LOG(logger_, debug, "POS tag for " << wordno << " in sent " << snt << " = " << postag);
	}

	for(uint i = 0; i < segs.size(); i++) {
		uint phrCount=0;
		BOOST_FOREACH(const AnchoredPhrasePair &app, segs[i]) {
			s->AddWord(i,phrCount,app,selectedPOS,HistorySize);
			phrCount++;
		}
	}

	s->currentScore = s->score();
	*sbegin = s->currentScore;
	return s;
}


void SemanticSimilarityModel::computeSentenceScores(
	const DocumentState &doc,
	uint sentno,
	Scores::iterator sbegin
) const {
	*sbegin = Float(0);
}

FeatureFunction::StateModifications
*SemanticSimilarityModel::estimateScoreUpdate(
	const DocumentState &doc,
	const SearchStep &step,
	const State *state,
	Scores::const_iterator psbegin,
	Scores::iterator sbegin
) const {
	const SemanticSimilarityModelState
		*prevstate = dynamic_cast<const SemanticSimilarityModelState *>(state);
	SemanticSimilarityModelState *s = prevstate->clone();

	bool firstSent = true;
	uint sentNo = 0;
	uint phrNo = 0;
	int phrNoDiff = 0;

	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();
	const uint docSize = segs.size();

	// run through all modifications and check if we need to update the word list
	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	std::vector<SearchStep::Modification>::const_iterator it = mods.begin();
	while(it != mods.end()) {
		PhraseSegmentation::const_iterator from_it = it->from_it;
		PhraseSegmentation::const_iterator to_it = it->to_it;

		const PhraseSegmentation &current = doc.getPhraseSegmentation(it->sentno);
		phrNo = std::distance(current.begin(), from_it);

		if(firstSent || (sentNo != it->sentno)) {
			sentNo = it->sentno;
			firstSent = false;

			// LOG(logger_, debug, "word list before: " << s->GetWords(sentNo));

			phrNoDiff = 0;

			// start from scratch:
			// copy words from all phrases before the current modification
			s->ClearWords(sentNo);
			s->CopyWords(*prevstate,sentNo,0,phrNo,phrNoDiff);

			// get the LM history before the modification (initialize LM states)
			std::vector<SelectedWordVector*> history;
			s->GetHistory(history,sentNo,HistorySize-1);
		}

		uint oldLength = std::distance(from_it,to_it);

		//-------------------------------------------------------
		// add words in modification
		//-------------------------------------------------------
		// record the words in the proposed modification (and compare to current word list)
		uint modLength = 0;
		BOOST_FOREACH(const AnchoredPhrasePair &app, it->proposal) {
			uint newPhrNo = phrNo+modLength+phrNoDiff;
			s->AddWord(sentNo,newPhrNo,app,selectedPOS,HistorySize);
			modLength++;
		}

		// accumulate length difference between modification and original
		phrNoDiff += modLength - oldLength;

		//------------------------------------------------------------------------------------
		// remove scores for subsequent words but not further than start of next modification
		//------------------------------------------------------------------------------------
		it++;
		uint currentSentNo = sentNo;
		uint nextModSentNo = docSize-1;
		uint nextModFrom = std::distance(
			segs[segs.size()-1].begin(),
			segs[segs.size()-1].end()
		);

		if(it != mods.end()) {
			const PhraseSegmentation &next = doc.getPhraseSegmentation(it->sentno);
			nextModSentNo = it->sentno;
			nextModFrom = std::distance(next.begin(), it->from_it);
		}

		// don't forget to copy remaining selected words in currentSentNo!
		phrNo = std::distance(current.begin(), to_it);
		uint wordIdx = s->selectedWords[sentNo].size();

		if(phrNo < current.size()) {
			if(currentSentNo == nextModSentNo)
				s->CopyWords(
					*prevstate,
					currentSentNo,
					phrNo,
					nextModFrom,
					phrNoDiff
				);
			else
				s->CopyWords(
					*prevstate,
					currentSentNo,
					phrNo,
					current.size(),
					phrNoDiff
				);
		}

		// LOG(logger_, debug, " word list after: " << s->GetWords(currentSentNo));
		s->UpdateFutureScores(
			sentNo,
			wordIdx,
			HistorySize-1,
			nextModSentNo,
			nextModFrom
		);
	}

	*sbegin = s->score();

	if(prevstate->currentScore != s->currentScore)
		LOG(logger_, debug, "new score " << s->currentScore);

	return s;
}

FeatureFunction::StateModifications
*SemanticSimilarityModel::updateScore(
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
*SemanticSimilarityModel::applyStateModifications(
	FeatureFunction::State *oldState,
	FeatureFunction::StateModifications *modif
) const {
	SemanticSimilarityModelState *os = dynamic_cast<SemanticSimilarityModelState *>(oldState);
	SemanticSimilarityModelState *ms = dynamic_cast<SemanticSimilarityModelState *>(modif);

	os->currentScore = ms->currentScore;
	os->selectedWords.swap(ms->selectedWords);

	return oldState;
}
