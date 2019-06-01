#ifndef TRAIN_PEAKPIECES_H
#define TRAIN_PEAKPIECES_H

#include <vector>
#include <list>
#include <string>

#include "PeakPiece.h"

#include "struct.h"

using namespace std;


class PeakPieces
{
  private:

    list<DistEntry> steps;
    list<PeakPiece> pieces;
    DistEntry summary;


    void makeSteps(const vector<unsigned>& distances);

    void collapseSteps();

    void makePieces();

    void eraseSmallPieces(const unsigned smallPieceLimit);

    void eraseSmallMaxima();

    void split();

    void unjitter();

    bool selectShortAmongTwo(
      const PeakPiece& piece1,
      const PeakPiece& piece2,
      const Gap& wheelGap,
      Gap& shortGap) const;

    bool selectShortAmongThree(
      const PeakPiece& piece1,
      const PeakPiece& piece2,
      const PeakPiece& piece3,
      const Gap& wheelGap,
      Gap& shortGap) const;


  public:

    PeakPieces();

    ~PeakPieces();

    void reset();

    void make(
      const vector<unsigned>& distances,
      const unsigned smallPieceLimit = 0);

    bool empty() const;

    void guessBogieGap(Gap& wheelGap) const;

    bool extendBogieGap(Gap& wheelGap) const;

    void guessShortGap(
      const Gap& wheelGap,
      Gap& shortGap) const;

    void guessLongGap(
      Gap& wheelGap,
      Gap& shortGap,
      Gap& longGap) const;

    bool guessGaps(
      const Gap& wheelGap,
      Gap& shortGap,
      Gap& longGap) const;

    string str(const string& text) const;
    string strModality(const string& text) const;

};

#endif
