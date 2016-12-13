#include "models/YourModel.h"

#include "DocumentState.h"
#include "SearchStep.h"

// constructor
YourModel::YourModel(
	const Parameters &params
) :	logger_("YourModel")
{
	LOG(logger_, debug, "Constructing YourModel");
	/* ... */
}


// initialise the state and calculate the initial Your score
FeatureFunction::State *YourModel::initDocument(
	const DocumentState &doc,
	Scores::iterator sbegin
) const {
	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();
	for(uint i = 0; i < segs.size(); i++) {
		/* loop over all sentences in the document,
		   computing your initial score, or if you use a state object
		   of your own, doing something to your state
		   before letting it do the computation
		*/
	}

	*sbegin = Float(0);
	return NULL;
}

// calculate the new score score given the proposed modifications to the document
FeatureFunction::StateModifications
*YourModel::estimateScoreUpdate(
	const DocumentState &doc,
	const SearchStep &step,
	const FeatureFunction::State *state,
	Scores::const_iterator psbegin,
	Scores::iterator sbegin
) const {
	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	for(std::vector<SearchStep::Modification>::const_iterator
		it = mods.begin();
		it != mods.end();
		++it
	) {
		/* ... */
	}

	*sbegin = Float(0);
	return NULL;
}

FeatureFunction::StateModifications *YourModel::updateScore(
	const DocumentState &doc,
	const SearchStep &step,
	const FeatureFunction::State *state,
	FeatureFunction::StateModifications *estmods,
	Scores::const_iterator psbegin,
	Scores::iterator estbegin
) const {
	return estmods;
}

/* only needed if you have state of your own */
// FeatureFunction::State
// *YourModel::applyStateModifications(
	// FeatureFunction::State *oldState,
	// FeatureFunction::StateModifications *modif
// ) const {
	// /* ... */
// }

void YourModel::computeSentenceScores(
	const DocumentState &doc,
	uint sentno,
	Scores::iterator sbegin
) const {
	*sbegin = Float(0);
}
