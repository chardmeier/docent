#ifndef docent_YourModel_h
#define docent_YourModel_h

#include "FeatureFunction.h"

class YourModel : public FeatureFunction {
private:
	Logger logger_;
	// ...
public:
	YourModel(const Parameters &params);

	virtual State *initDocument(
		const DocumentState &doc,
		Scores::iterator sbegin
	) const;
	virtual StateModifications *estimateScoreUpdate(
		const DocumentState &doc,
		const SearchStep &step,
		const State *state,
		Scores::const_iterator psbegin,
		Scores::iterator sbegin
	) const;
	virtual StateModifications *updateScore(
		const DocumentState &doc,
		const SearchStep &step,
		const State *state,
		StateModifications *estmods,
		Scores::const_iterator psbegin,
		Scores::iterator estbegin
	) const;

	/* only needed if you have state of your own */
	// virtual State *applyStateModifications(
		// State *oldState,
		// StateModifications *modif
	// ) const;

	virtual void computeSentenceScores(
		const DocumentState &doc,
		uint sentno,
		Scores::iterator sbegin
	) const;

	/* fix to how many scores your model uses! */
	virtual uint getNumberOfScores() const {
		return 1;
	}
};

#endif
