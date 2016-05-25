/*
 *  PhrasePair.h
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

#ifndef docent_PhrasePair_h
#define docent_PhrasePair_h

#include "Docent.h"

#include <vector>

#include <boost/flyweight.hpp>
#include <boost/flyweight/no_tracking.hpp>
#include <boost/iterator_adaptors.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/serialization/split_member.hpp>


typedef std::pair<char, char> AlignmentPair;

class WordAlignment {
private:
	typedef boost::dynamic_bitset<> MatrixType_;

	uint nsrc_;
	uint ntgt_;
	MatrixType_ matrix_;

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		ar & nsrc_;
		ar & ntgt_;
		ar & matrix_;
	}

public:
	class const_iterator :
	public boost::iterator_facade<
		const_iterator,
		const uint,
		boost::bidirectional_traversal_tag,
		const uint
	> {
	private:
		friend class WordAlignment;
		friend class boost::iterator_core_access;

		const MatrixType_ &matrix_;

		uint from_;
		uint nelems_;
		uint stride_;
		MatrixType_::size_type base_;

		const_iterator(
			const MatrixType_ &matrix,
			uint from,
			uint nelems,
			uint stride,
			bool begin
		) :	matrix_(matrix),
			from_(from),
			nelems_(nelems),
			stride_(stride)
		{
			if(begin) {
				base_ = from_;
				for(uint i = 0; i < nelems_; i++, base_ += stride_)
					if(matrix_[base_])
						break;
			} else
				base_ = from_ + nelems_ * stride_;
		}

		const uint dereference() const {
			return (base_ - from_) / stride_;
		}

		void increment() {
			while(base_ < from_ + nelems_ * stride_) {
				base_ += stride_;
				if(base_ < matrix_.size() && matrix_[base_])
					break;
			}
		}

		void decrement() {
			while(base_ > stride_) {
				base_ -= stride_;
				if(matrix_[base_])
					break;
			}
		}

		bool equal(const const_iterator &o) const {
			return o.base_ == base_ && o.from_ == from_ &&
				o.nelems_ == nelems_ && o.stride_ == stride_;
		}
	};

	WordAlignment(
		uint nsrc, uint ntgt
	) : nsrc_(nsrc), ntgt_(ntgt), matrix_(nsrc * ntgt) {}

	WordAlignment(
		uint nsrc, uint ntgt,
		const std::vector<AlignmentPair> &alignments
	);

	WordAlignment(
		uint nsrc, uint ntgt,
		const std::string &alignment
	);

	bool hasLink(uint s, uint t) const {
		return matrix_[t * nsrc_ + s];
	}

	void setLink(uint s, uint t) {
		matrix_.set(t * nsrc_ + s);
	}

	uint getSourceSize() const {
		return nsrc_;
	}

	uint getTargetSize() const {
		return ntgt_;
	}

	const_iterator begin_for_source(uint s) const {
		return const_iterator(matrix_, s, ntgt_, nsrc_, true);
	}

	const_iterator end_for_source(uint s) const {
		return const_iterator(matrix_, s, ntgt_, nsrc_, false);
	}

	const_iterator begin_for_target(uint t) const {
		return const_iterator(matrix_, t * nsrc_, nsrc_, 1, true);
	}

	const_iterator end_for_target(uint t) const {
		return const_iterator(matrix_, t * nsrc_, nsrc_, 1, false);
	}
};


class PhrasePairData {
private:
	std::vector<uint> coverage_;
	Phrase sourcePhrase_;
	Phrase targetPhrase_;
	std::vector<Phrase> targetAnnotations_;
	WordAlignment alignment_;
	Scores scores_;
	bool oovFlag_;

public:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		ar & coverage_;
		ar & sourcePhrase_;
		ar & targetPhrase_;
		ar & targetAnnotations_;
		ar & alignment_;
		ar & scores_;
		ar & oovFlag_;
	}

	PhrasePairData(
		const std::vector<Word> &sourcePhrase,
		const std::vector<Word> &targetPhrase,
		const std::vector<Phrase> &targetAnnotations,
		const WordAlignment &alignment, const Scores &scores
	) :	coverage_(1, sourcePhrase.size()),
		sourcePhrase_(sourcePhrase),
		targetPhrase_(targetPhrase),
		targetAnnotations_(targetAnnotations),
		alignment_(alignment),
		scores_(scores),
		oovFlag_(false)
	{}

	PhrasePairData(
		const std::vector<uint> &coverage,
		const std::vector<Word> &sourcePhrase, const std::vector<Word> &targetPhrase,
		const std::vector<Phrase> &targetAnnotations,
		const WordAlignment &alignment, const Scores &scores
	) :	coverage_(coverage),
		sourcePhrase_(sourcePhrase),
		targetPhrase_(targetPhrase),
		targetAnnotations_(targetAnnotations),
		alignment_(alignment),
		scores_(scores),
		oovFlag_(false)
	{}

	PhrasePairData(const Word &oov, const Scores &scores
	) :	coverage_(1, 1),
		sourcePhrase_(1, oov),
		targetPhrase_(1, oov),
		alignment_(1, 1),
		scores_(scores),
		oovFlag_(true)
	{
		alignment_.setLink(0, 0);
	}

	//Needed for serialization
	PhrasePairData() :alignment_(1, 1) {
		alignment_.setLink(0, 0);
	}

	const std::vector<uint> &getCoverage() const {
		return coverage_;
	}

	Phrase getSourcePhrase() const {
		return sourcePhrase_;
	}

	Phrase getTargetPhrase() const {
		return targetPhrase_;
	}

	Phrase getTargetAnnotations(uint level) const {
		static Phrase EMPTY_PHRASE(std::vector<Word>(1, ""));
		if(oovFlag_)
			return EMPTY_PHRASE;
		else
			return targetAnnotations_[level];
	}

	Phrase getTargetPhraseOrAnnotations(
		int annotationLevel,
		bool tokenFlag
	) const {
		if(annotationLevel==-1||(oovFlag_&& tokenFlag))
			return getTargetPhrase();
		else
			return getTargetAnnotations(annotationLevel);
	}

	const WordAlignment &getWordAlignment() const {
		return alignment_;
	}

	const Scores &getScores() const {
		return scores_;
	}

	bool isOOV() const {
		return oovFlag_;
	}

	bool operator==(const PhrasePairData &o) const;

	friend std::size_t hash_value(const PhrasePairData &p);
};


std::size_t hash_value(const PhrasePairData &p);

typedef boost::flyweight<PhrasePairData, boost::flyweights::no_tracking> PhrasePair;
typedef std::pair<CoverageBitmap, PhrasePair> AnchoredPhrasePair;
typedef std::list<AnchoredPhrasePair> PhraseSegmentation;

template<class PhrasePairIterator>
inline uint countTargetWords(PhrasePairIterator from_it, PhrasePairIterator to_it) {
	uint wordCount = 0;
	for(PhrasePairIterator it = from_it; it != to_it; ++it)
		wordCount += it->second.get().getTargetPhrase().get().size();
	return wordCount;
}

template<class PhrasePairContainer>
inline uint countTargetWords(PhrasePairContainer cont) {
	return countTargetWords(cont.begin(), cont.end());
}

struct CompareAnchoredPhrasePairs :
public std::binary_function<
	const AnchoredPhrasePair,
	const AnchoredPhrasePair,
	bool
> {
	typedef boost::tuples::tuple<
		const CoverageBitmap &,
		const PhraseData &,
		const PhraseData &
	> PhrasePairKey;

	bool operator()(const PhrasePairKey &a, const PhrasePairKey &b) const {
		return a < b;
	}

	bool operator()(const AnchoredPhrasePair &a, const PhrasePairKey &b) const {
		return operator()(peel(a), b);
	}

	bool operator()(const PhrasePairKey &a, const AnchoredPhrasePair &b) const {
		return operator()(a, peel(b));
	}

	bool operator()(const AnchoredPhrasePair &a, const AnchoredPhrasePair &b) const {
		return operator()(peel(a), peel(b));
	}

private:
	PhrasePairKey peel(const AnchoredPhrasePair &a) const {
		using namespace boost;
		return make_tuple(
			cref(a.first),
			cref(a.second.get().getSourcePhrase().get()),
			cref(a.second.get().getTargetPhrase().get())
		);
	}
};

std::ostream &operator<<(std::ostream &os, const std::vector<Word> &phrase);
std::ostream &operator<<(std::ostream &os, const AnchoredPhrasePair &ppair);
std::ostream &operator<<(std::ostream &os, const PhraseSegmentation &ppair);

#endif
