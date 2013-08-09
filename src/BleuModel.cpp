#include "BleuModel.h"
#include "SearchStep.h"
#include "DocumentState.h"
#include "PhrasePair.h"
#include "PiecewiseIterator.h"
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <numeric>

struct BleuModelState : public FeatureFunction::State {
	
	// clipped counts for each sentence for each value of n (1<=n<=4). inner vector is over sentences; outer is over n.	
	std::vector<std::vector<uint> > clipped_counts;
	// length of each sentence is the candidate translation.	
	std::vector<uint> candidate_lengths;

	virtual BleuModelState *clone() const {
		return new BleuModelState(*this);
	}
};

struct BleuModelModifications : public FeatureFunction::StateModifications {
	
	// each entry in the modifications vector contains a sentence number and a vector of tokens	
	std::vector<std::pair<uint,std::vector<std::string> > > state_mods;

};

// constructor
BleuModel::BleuModel(const Parameters &params) : logger_("BleuModel"){

	//LOG(logger_, debug, "BleuModel::BleuModel");

	std::string fileName = params.get<std::string>("reference-file", "");
	//LOG(logger_, debug, "Reading file: " << fileName);
	std::ifstream infile(fileName.c_str());			

	referenceLength_ = 0;

	std::string sentence;
	// loop over the reference file, putting each line in the variable "sentence"	
	while (std::getline(infile, sentence))
	{		
		//LOG(logger_, debug, "Reading sentence: " << sentence);
	
		// create a vector of tokens by splitting the current line at whitespaces		
		Tokens_ tokens;
		std::istringstream iss(sentence);
		std::string current_token;		
		uint sentence_length = 0; 		
		while(iss >> current_token){			
			tokens.push_back(current_token);
			sentence_length++;
			referenceLength_++;
		}
		referenceLengths_.push_back(sentence_length);

		//LOG(logger_, debug, "Finished reading sentence: " << sentence);

		// TokenHash_ to store the n-gram counts for the current sentence
		TokenHash_ sentence_counts;		

		//LOG(logger_, debug, "Size of tokens vector = " << tokens.size());

		// loop over values of n from 1 to 4 (these are the n-grams used in calculating the BLEU score)
		for(uint n=0; n<4; n++){
			//LOG(logger_, debug, "n = " << n);				
			if(tokens.size()>n){		
				// loop over the n-grams in the current sentence.		
				for(uint i = 0; i < tokens.size()-n; i++) {
					//LOG(logger_, debug, "i = " << i);				
				
					// create a blank n-gram of the correct size and add the first token
					Tokens_ ngram(n+1);
					ngram[0] = tokens[i];					
					//std::string ngram = tokens[i];
					for(uint j=1; j<=n; j++){
						//LOG(logger_, debug, "j = " << j);					
						ngram[j] = tokens[i+j];				
					}
					// test if the n-gram has already been seen, and if not create an entry for it with count 1				
					if (sentence_counts.find(ngram) == sentence_counts.end()) {
						sentence_counts.insert(std::make_pair(ngram,1));
					}
					// otherwise simply add one to its count
					else {
						sentence_counts[ngram]++;
					}
				}
			}
		}

		//LOG(logger_, debug, "Finished recording n-grams for sentence: " << sentence);
		
		// add the current sentence counts to the referenceNgramCounts_ variable
		referenceNgramCounts_.push_back(sentence_counts);
		
	}

	
	//for(std::vector<uint>::iterator it = referenceLengths_.begin(); it != referenceLengths_.end(); ++it) {
    	//	LOG(logger_, debug, "Sentence length: " << *it);      
	//}	

	//for(std::vector<TokenHash_>::iterator it = referenceNgramCounts_.begin(); it != referenceNgramCounts_.end(); ++it) {
    	//	LOG(logger_, debug, "TokenHash_ contains: ");
	//	BOOST_FOREACH( TokenHash_::value_type v, *it ) {
    	//		LOG(logger_, debug, v.first << " " << v.second);      
	//	}
	//}

};

// destructor
BleuModel::~BleuModel() {}

// initialise the state and calculate the initial BLEU score
FeatureFunction::State *BleuModel::initDocument(const DocumentState &doc, Scores::iterator sbegin) const {

	//LOG(logger_, debug, "BleuModel::initDocument");

	BleuModelState *state = new BleuModelState();

	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();
	uint no_sents = segs.size();

	state->clipped_counts.resize(4);
	for(uint n=0;n<4;n++){
		state->clipped_counts[n].resize(no_sents);
	}

	state->candidate_lengths.resize(no_sents);

	// loop over all sentences in the document
	for(uint sent_no=0; sent_no<no_sents; sent_no++){

		PhraseSegmentation::const_iterator ng_it = segs[sent_no].begin();
		PhraseSegmentation::const_iterator to_it = segs[sent_no].end();

		Tokens_ candidate_tokens;

		// loop over the phrases in the current sentence, adding all the tokens to the candidate_tokens variable		
		while(ng_it != to_it){
		
			for(PhraseData::const_iterator wi = ng_it->second.get().getTargetPhrase().get().begin();
				wi != ng_it->second.get().getTargetPhrase().get().end(); ++wi) {
				candidate_tokens.push_back(*wi);		
				//LOG(logger_, debug, *wi);
			}
			++ng_it;	
		}

		//for(Tokens_::iterator tmp_it = candidate_tokens.begin(); tmp_it!=candidate_tokens.end(); ++tmp_it){
		//	LOG(logger_, debug, *tmp_it);
		//}

		state->candidate_lengths[sent_no] = candidate_tokens.size();
		std::vector<uint> clipped_counts = calculateClippedCounts(candidate_tokens, sent_no);
		for(uint n=0;n<4;n++){
			state->clipped_counts[n][sent_no] = clipped_counts[n];
		}
		
	}
	
	Float &s = *sbegin;	
	calculateBLEU(*state,s);
	return state;

}

// the BLEU score cannot be calculated on a sentence-by-sentence basis (not without some modifications)
void BleuModel::computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const {
	*sbegin = Float(0);
}

// calculate the new score score given the proposed modifications to the document
FeatureFunction::StateModifications *BleuModel::estimateScoreUpdate(const DocumentState &doc, const SearchStep &step, const FeatureFunction::State *ffstate, Scores::const_iterator psbegin, Scores::iterator sbegin) const {
	
	//LOG(logger_, debug, "BleuModel::estimateScoreUpdate");

	const BleuModelState &state = dynamic_cast<const BleuModelState &>(*ffstate);
	BleuModelModifications *bleu_mods = new BleuModelModifications();

	const std::vector<SearchStep::Modification> &doc_mods = step.getModifications();

	// loop over the document modifications
	std::vector<SearchStep::Modification>::const_iterator mod_it = doc_mods.begin();
	while(mod_it!=doc_mods.end()){

		uint sent_no = mod_it->sentno;		
		std::vector<std::vector<SearchStep::Modification>::const_iterator> mod_iterators;		
		mod_iterators.push_back(mod_it);	

		uint no_mods_this_sentence = 1;
		
		// find all the modifications to the current sentence
		std::vector<SearchStep::Modification>::const_iterator it1 = mod_it;
		while(++it1 != doc_mods.end()){
			if(it1->sentno == sent_no){
				//if(it1->from < mod_it->from){
					//LOG(logger_, debug, "Second modification before first");
				//}
				mod_iterators.push_back(it1);
				no_mods_this_sentence++;
			}
			else{
				break;
			}
		}

		LOG(logger_, debug, "Modifying sentence " << sent_no+1);
		LOG(logger_, debug, "Number of modifications this sentence: " << no_mods_this_sentence);
		
		//for(uint token_no=0;token_no<sents[sent_no].size();token_no++){
		//	LOG(logger_, debug, sents[sent_no][token_no]);
		//}
		
		std::vector<PhraseSegmentation> proposals(no_mods_this_sentence);
		std::vector<PhraseSegmentation::const_iterator> prop_ng_its(no_mods_this_sentence);		

		for(uint mod_id=0;mod_id<no_mods_this_sentence;mod_id++){
			proposals[mod_id] = mod_iterators[mod_id]->proposal;
			prop_ng_its[mod_id] = proposals[mod_id].begin();		
		}

		const PhraseSegmentation &old_seg = doc.getPhraseSegmentation(sent_no);

		PhraseSegmentation::const_iterator ng_it = old_seg.begin();
		PhraseSegmentation::const_iterator to_it = old_seg.end();
		
		Tokens_ modified_tokens;

		uint phrase_no=0;
		
		// construct a vector of the proposed tokens in the modified sentence		
		for(uint mod_id=0;mod_id<no_mods_this_sentence;mod_id++){
			while(phrase_no<mod_iterators[mod_id]->from){			
			//for(phrase_no=0; phrase_no<mod_it->from; phrase_no++){
				LOG(logger_, debug, "Loop 1: Phrase " << phrase_no);		
				for(PhraseData::const_iterator wi = ng_it->second.get().getTargetPhrase().get().begin();
					wi != ng_it->second.get().getTargetPhrase().get().end(); ++wi) {
					modified_tokens.push_back(*wi);		
					LOG(logger_, debug, *wi);
				}
				phrase_no++;				
				++ng_it;
			}
		

			for(phrase_no=mod_iterators[mod_id]->from; phrase_no<mod_iterators[mod_id]->to; phrase_no++){
				LOG(logger_, debug, "Loop 2: Phrase " << phrase_no);
				for(PhraseData::const_iterator wi = prop_ng_its[mod_id]->second.get().getTargetPhrase().get().begin();
					wi != prop_ng_its[mod_id]->second.get().getTargetPhrase().get().end(); ++wi) {
					modified_tokens.push_back(*wi);		
					LOG(logger_, debug, *wi);
				}
				++prop_ng_its[mod_id];
				//++ng_it;
			}

			for(PhraseSegmentation::const_iterator tmp_it = mod_iterators[mod_id]->from_it; tmp_it!=mod_iterators[mod_id]->to_it; ++tmp_it){
				++ng_it;
			}	

		}

		while(ng_it != to_it){
			LOG(logger_, debug, "Loop 3: Phrase " << phrase_no);
			for(PhraseData::const_iterator wi = ng_it->second.get().getTargetPhrase().get().begin();
				wi != ng_it->second.get().getTargetPhrase().get().end(); ++wi) {
				modified_tokens.push_back(*wi);		
				LOG(logger_, debug, *wi);
			}
			phrase_no++;
			++ng_it;
		}	

		for(Tokens_::iterator it = modified_tokens.begin(); it!= modified_tokens.end(); ++it){
			LOG(logger_, debug, *it);
		}

		
		for(uint mod_id=0;mod_id<no_mods_this_sentence;mod_id++){
			++mod_it;
		}

		bleu_mods->state_mods.push_back(std::make_pair(sent_no,modified_tokens));	

	}

	BleuModelState new_state = state;
	updateState(new_state,*bleu_mods);

	Float &s = *sbegin;	
	calculateBLEU(new_state,s);
	return bleu_mods;

}

FeatureFunction::StateModifications *BleuModel::updateScore(const DocumentState &doc, const SearchStep &step, const FeatureFunction::State *ffstate, 			FeatureFunction::StateModifications *estmods, Scores::const_iterator psbegin, Scores::iterator estbegin) const {

	//LOG(logger_, debug, "BleuModel::updateScore");

	return estmods;
}

FeatureFunction::State *BleuModel::applyStateModifications(FeatureFunction::State *oldState, FeatureFunction::StateModifications *modif) const {
	
	//LOG(logger_, debug, "BleuModel::applyStateModifications");	

	BleuModelState &state = dynamic_cast<BleuModelState &>(*oldState);
	BleuModelModifications *mod = dynamic_cast<BleuModelModifications *>(modif);	

	updateState(state, *mod);

	return &state;
}

std::vector<uint> BleuModel::calculateClippedCounts(const BleuModel::Tokens_ candidate_tokens, const uint sent_no) const {
	
	//LOG(logger_, debug, "BleuModel::calculateClippedCounts");

	TokenHash_ tmp_counts;
	TokenHash_ reference = referenceNgramCounts_[sent_no];

	std::vector<uint> clipped_counts(4);

	// loop over values of n from 1 to 4 (these are the n-grams used in calculating the BLEU score)
	for(uint n=0; n<4; n++){
			
		clipped_counts[n] = 0;
			
		if(candidate_tokens.size()>n){

			for(uint ngram_no=0; ngram_no<candidate_tokens.size()-n; ngram_no++){

				Tokens_ ngram(n+1);

				ngram[0] = candidate_tokens[ngram_no];
				for(uint j=1; j<=n; j++){		
					ngram[j] = candidate_tokens[ngram_no+j];
				}

				// check if n gram is in the reference translation for this sentence	
				if (reference.find(ngram) != reference.end()) {

					// test if the n-gram has already been seen in this sentence, and if not create an entry for it with count 1				
					if (tmp_counts.find(ngram) == tmp_counts.end()) {
						tmp_counts.insert(std::make_pair(ngram,1));
					}
					// otherwise add one to its count
					else {
						tmp_counts[ngram]++;
					}

					// add one to the clipped count for the sentence, as long as the ngram hasn't been seen more times than it appears in the reference
					if(!(tmp_counts[ngram]>reference[ngram])){
						clipped_counts[n]++;
					}

				}
				
			}
		}
	}
	return clipped_counts;
}

void BleuModel::updateState(BleuModelState &state, const BleuModelModifications &bleu_mods) const {
	
	//LOG(logger_, debug, "BleuModel::updateState");

	uint no_modifications = bleu_mods.state_mods.size();	

//for(std::vector<std::pair<uint,std::vector<std::string> > >::iterator it = bleu_state_mods.modifications.begin(); it!=bleu_state_mods.modifications.end(); ++it){
	for(uint current_mod = 0; current_mod<no_modifications; current_mod++){

		uint sent_no = bleu_mods.state_mods[current_mod].first;
		BleuModel::Tokens_ tokens = bleu_mods.state_mods[current_mod].second;

		//uint sent_no = it->first;
		//BleuModel::Tokens_ tokens = it->second;
		//LOG(logger_, debug, "state size: " << state.candidate_lengths.size());		
		state.candidate_lengths[sent_no] = tokens.size();
		std::vector<uint> clipped_counts = calculateClippedCounts(tokens, sent_no);
		for(uint n=0;n<4;n++){
			state.clipped_counts[n][sent_no] = clipped_counts[n];
		}
	}

}

void BleuModel::calculateBLEU(BleuModelState &state, Float &s) const {
	
	LOG(logger_, debug, "BleuModel::calculateBLEU");
	
	double precision_product = 1;
	for(uint n=0; n<4; n++){
		uint no_candidate_ngrams=0;
		for(std::vector<uint>::iterator it = state.candidate_lengths.begin(); it!=state.candidate_lengths.end(); ++it){
			if(*it>n){
				no_candidate_ngrams += *it-n;			
			}
		}
		uint sum_of_clipped_counts = std::accumulate(state.clipped_counts[n].begin(),state.clipped_counts[n].end(),0);	
		double precision = (double)sum_of_clipped_counts/no_candidate_ngrams;
		LOG(logger_, debug, "Precision for n = " << n+1 << ": " << precision*100);
		precision_product *= precision;	
	}

	double BP;

	uint total_candidate_length = std::accumulate(state.candidate_lengths.begin(),state.candidate_lengths.end(),0);

	if(total_candidate_length>referenceLength_){
		BP = 1.0;
	}
	else{
		BP = exp(1-referenceLength_/total_candidate_length);
	}

	s = BP*pow(precision_product,0.25);

	LOG(logger_, debug, "Candiate length = " << total_candidate_length);
	LOG(logger_, debug, "Reference length = " << referenceLength_);
	LOG(logger_, debug, "BP = " << BP);
	LOG(logger_, debug, "BLEU score = " << s);


}
