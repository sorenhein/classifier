#ifndef TRAIN_PEAKPOOL_H
#define TRAIN_PEAKPOOL_H

/*
   We keep several layers of peaks here.  The first layer is the
   set of raw peaks (actually we process those in place, as there
   are so many and we don't want to copy them).  The next layers
   are various levels of processing, e.g. after eliminating tiny
   areas or certain kinks.

   We keep these layers as it is sometimes necessary to go back and
   undo a certain aggregation locally (for a certain set of samples).  
   For instance, something may have looked like a kink,  but it really 
   was a peak.

   The user calls copy() to make a new layer and to leave the
   processed layer in place at this point.  Other than that,
   processing happens on the current layer and modifies it.
   The current layer mostly behaves like a normal list.

   It is probably possible to do the whole class "in place", i.e.
   to reconstruct peaks from information in neighboring peaks.
   This would save some time and space from copying.
 */


#include <vector>
#include <list>
#include <string>

#include "Peaks.h"
#include "PeakPtrs.h"

using namespace std;

typedef list<PPLciterator> PeakIterList;
typedef PeakIterList::const_iterator PIciterator;


class PeakPool
{
  private:

    list<Peaks> peakLists;

    Peaks * peaks;

    // Could have one for each PeakList as well.
    PeakPtrs _candidates;

    // Average peaks used for quality.
    vector<Peak> averages;

    // We only trim leading transients once.
    bool transientTrimmedFlag;
    unsigned transientLimit;


    void showSplit(
      Piterator& pStart,
      Piterator& pEnd,
      const unsigned offset,
      const string& text) const;

    void collapseAndRemove(
      const Piterator& pStart,
      const Piterator& pEnd);

    void collapseRange(
      const Piterator& pmin,
      const Piterator& pStart,
      const Piterator& pEnd);

    void collapseRange(
      const Piterator& pStart,
      const Piterator& pEnd);

    void removeCandidates(
      const Piterator& pStart,
      const Piterator& pEnd);

    void printRepairedSegment(
      const string& text,
      const Bracket& bracketTop,
      const unsigned offset) const;

    void updateRepairedPeaks(Bracket& bracket);

    bool peakFixable(
      Piterator& foundIter,
      const PeakFncPtr& fptr,
      const Bracket& bracket,
      const bool nextSkipFlag,
      const string &text,
      const unsigned offset,
      const bool forceFlag) const;

    Peak * repairTopLevel(
      Piterator& foundIter,
      const PeakFncPtr& fptr,
      const unsigned offset,
      const bool testFlag,
      const bool forceFlag,
      unsigned& testIndex);

    Peak * repairFromLower(
      Peaks& listLower,
      Piterator& foundIter,
      const PeakFncPtr& fptr,
      const unsigned offset,
      const bool testFlag,
      const bool forceFlag,
      unsigned& testIndex);


  public:

    PeakPool();

    ~PeakPool();

    void clear();

    void copy();

    void logAverages(const vector<Peak>& averagesIn);

    bool pruneTransients(const unsigned firstGoodIndex);

    void mergeSplits(
      const unsigned wheelDist,
      const unsigned offset);

    Peak * repair(
      const Peak& peakHint,
      const PeakFncPtr& fptr,
      const unsigned offset, // TMP
      const bool testFlag,
      const bool forceFlag,
      unsigned& testIndex);


    void makeCandidates();

    PeakPtrs& candidates();
    const PeakPtrs& candidatesConst() const;

    Peaks& top();
    const Peaks& topConst() const;

    bool getClosest(
      const list<unsigned>& carPoints,
      const PeakFncPtr& fptr,
      const PPLciterator& cit,
      const unsigned numWheels,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused) const;

    void print(const unsigned offset) const;
    bool check() const;

    string strCounts() const;
};

#endif
