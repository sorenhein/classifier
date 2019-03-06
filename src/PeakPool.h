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

#include "PeakPtrs.h"
#include "struct.h"

using namespace std;

typedef list<Peak> PeakList;
typedef PeakList::iterator Piterator;
typedef PeakList::const_iterator Pciterator;

typedef list<PPLciterator> PeakIterList;
typedef PeakIterList::const_iterator PIciterator;


class PeakPool
{
  private:

    list<PeakList> peakLists;

    PeakList * peaks;

    // Could have one for each PeakList as well.
    PeakPtrs _candidates;

    // Average peaks used for quality.
    vector<Peak> averages;

    // We only trim leading transients once.
    bool transientTrimmedFlag;
    unsigned transientLimit;

    struct PiterPair
    {
      Piterator pit;
      bool hasFlag;
    };


    void getBracketingPeaks(
      const list<PeakList>::reverse_iterator& liter,
      const unsigned pindex,
      const bool minFlag,
      PiterPair& pprev,
      PiterPair& pnext) const;

    bool findCloseIter(
      const Peak& peakHint,
      const PiterPair& pprev,
      const PiterPair& pnext,
      Piterator& foundIter) const;

    void locateTopBrackets(
      const PiterPair& pfirstPrev,
      const PiterPair& pfirstNext,
      const Piterator& foundIter,
      Piterator& pprev, 
      Piterator& pnext) const;

    void printRepairData(
      const Piterator& foundIter,
      const Piterator& pprev,
      const Piterator& pnext,
      const Piterator& pfirstPrev,
      const Piterator& pfirstNext,
      const Piterator& pcurrPrev,
      const Piterator& pcurrNext,
      const unsigned offset) const;

    void printRepairedSegment(
      const Piterator& pfirstPrev,
      const Piterator& pfirstNext,
      const unsigned offset) const;

    void updateRepairedPeaks(
      Piterator& pfirstPrev,
      Piterator& pfirstNext);

    bool getHighestMax(
      const Piterator& pb,
      const Piterator& pe,
      Peak *& pmax) const;

    bool getBestMax(
      Piterator& pbmin,
      Piterator& pemin,
      Piterator& pref,
      Piterator& pbest) const;

    bool findTopSurrounding(
      Piterator& foundIter,
      Piterator& pprevSelected,
      Piterator& pnextSelected,
      Piterator& pprevBestMax,
      Piterator& pnextBestMax) const;

    Peak * repairTopLevel(
      Piterator& foundIter,
      const PeakFncPtr& fptr,
      const unsigned offset);

    Peak * repairFromLower(
      Piterator& foundIter,
      const PeakFncPtr& fptr,
      const unsigned offset);

    float getFirstPeakTime() const;


  public:

    PeakPool();

    ~PeakPool();

    void clear();

    bool empty() const;

    unsigned size() const;

    Piterator begin() const;
    Pciterator cbegin() const;

    Piterator end() const;
    Pciterator cend() const;

    Peak& front();
    Peak& back();

    void extend(); // Like emplace_back()

    void copy();

    void logAverages(const vector<Peak>& averagesIn);

    Piterator erase(
      Piterator pit1,
      Piterator pit2);

    Piterator collapse(
      Piterator pit1,
      Piterator pit2);

    bool pruneTransients(const unsigned firstGoodIndex);

    Peak * repair(
      const Peak& peakHint,
      const PeakFncPtr& fptr,
      const unsigned offset); // offset is TMP


    void makeCandidates();

    unsigned countCandidates(const PeakFncPtr& fptr) const;

    unsigned candsize() const;

    unsigned firstCandIndex() const;
    unsigned lastCandIndex() const;

    PeakPtrs& candidates();
    const PeakPtrs& candidatesConst() const;

    // TODO: No doubt there is a more elegant way to do this...


    Piterator prevExcl(
      Piterator& pit,
      const PeakFncPtr& fptr) const;

    Piterator prevExclSoft(
      Piterator& pit,
      const PeakFncPtr& fptr) const;

    Piterator prevIncl(
      Piterator& pit,
      const PeakFncPtr& fptr) const;

    Piterator prevInclSoft(
      Piterator& pit,
      const PeakFncPtr& fptr) const;

    Piterator nextExcl(
      Piterator& pit,
      const PeakFncPtr& fptr) const;

    Piterator nextIncl(
      Piterator& pit,
      const PeakFncPtr& fptr) const;


    bool getClosest(
      const list<unsigned>& carPoints,
      const PeakFncPtr& fptr,
      const PPLciterator& cit,
      const unsigned numWheels,
      PeakPtrs& closestPeaks,
      PeakPtrs& skippedPeaks) const;

    void getSelectedSamples(vector<float>& selected) const;

    void getSelectedTimes(vector<PeakTime>& times) const;


    string strAll(
      const string& text,
      const unsigned& offset) const;

    string strSelectedTimesCSV(const string& text) const;

    string strCounts() const;
};

#endif
