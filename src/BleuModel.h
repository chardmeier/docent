#ifndef docent_BleuModel_h
#define docent_BleuModel_h

#include "Docent.h"
#include "FeatureFunction.h"

class BleuModel : public FeatureFunction {
	
private: 
	
	typedef std::vector<std::string> Tokens_;
	typedef std::vector<Tokens_> Sents_;
	typedef boost::unordered_map<Tokens_,uint> TokenHash_;		
	Logger logger_;

	std::vector<uint> refLength_; // Vector containing total ref length for each doc
	std::vector<std::vector<uint> > refSentLengths_; // Vector of vectors containing ref sentence lengths for each doc	
	std::vector<std::vector<TokenHash_> > refNgramCounts_; // Vector of vectors containing Ngram counts for each ref sentence for each doc

public:
	BleuModel(const Parameters &params);

	virtual ~BleuModel();

	virtual FeatureFunction::State *initDocument(const DocumentState &doc, Scores::iterator sbegin) const;
	virtual StateModifications *estimateScoreUpdate(const DocumentState &doc, const SearchStep &step, const State *state,
		Scores::const_iterator psbegin, Scores::iterator sbegin) const;
	virtual StateModifications *updateScore(const DocumentState &doc, const SearchStep &step, const State *state,
		StateModifications *estmods, Scores::const_iterator psbegin, Scores::iterator estbegin) const;

	virtual FeatureFunction::State *applyStateModifications(FeatureFunction::State *oldState, FeatureFunction::StateModifications *modif) const;

	virtual void computeSentenceScores(const DocumentState &doc, uint sentno, Scores::iterator sbegin) const;

	virtual uint getNumberOfScores() const {
		return 1;
	}

	std::vector<uint> calculateClippedCounts(const Tokens_ candidate_tokens, const uint sent_no, const uint doc_no) const;
	void updateState(struct BleuModelState &current_state, const struct BleuModelModifications &bleu_state_mods) const;
	void calculateBLEU(struct BleuModelState &state, Float &s) const;
	void printTokens(Tokens_ tokens) const;

};

#endif
