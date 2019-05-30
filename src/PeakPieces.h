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

    void eraseSmallPieces();

    void eraseSmallMaxima();

    void split();

    void unjitter();



  public:

    PeakPieces();

    ~PeakPieces();

    void reset();

    void make(const vector<unsigned>& distances);

    bool empty() const;

    void guessBogieGap(Gap& wheelGap) const;

    bool extendBogieGap(Gap& wheelGap) const;

    void guessShortGap(
      Gap& wheelGap,
      Gap& shortGap,
      bool& wheelGapNewFlag) const;

    void guessLongGap(
      Gap& wheelGap,
      Gap& shortGap,
      Gap& longGap) const;

    string str(const string& text) const;
    string strModality(const string& text) const;

};

#endif
