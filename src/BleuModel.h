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

	uint referenceLength_;
	std::vector<uint> referenceLengths_;	
	std::vector<TokenHash_> referenceNgramCounts_;

public:
	BleuModel(const Parameters &params);

	// each of these virtual functions must be defined if this class inherits from FeatureFunction, otherwise we get errors. if functions are not declared errors occur at compile-time, if functions are declared but not defined errors appear at linking time
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

	std::vector<uint> calculateClippedCounts(const Tokens_ candidate_tokens, const uint sent_no) const;
	void updateState(struct BleuModelState &current_state, const struct BleuModelModifications &bleu_state_mods) const;
	//void findBLEU(const Sents_ &sents, Float &s) const;
	void calculateBLEU(struct BleuModelState &state, Float &s) const;

};

#endif
