#ifndef TRAIN_PEAKSEEDS_H
#define TRAIN_PEAKSEEDS_H

#include <list>
#include <string>

#include "Peak.h"
#include "PeakPool.h"

// Here we mark some peaks that look highly likely to be actual
// wheel peaks.  The algorithm is possible over the top, but simple
// level thresholding probably doesn't work so well.  Here we first
// determine intervals ranging from one negative minimum to another,
// possible with other negative minima in between them.  Then we
// nest those intervals inside one another as appropriate.  Finally
// we find those peaks that are "watersheds", i.e. those peaks that
// are particularly large.
//
// No doubt there are other and better ways to do this.  The current
// algorithm arose out of an attempt to derive the entire peak structure,
// but this bit of the algorithm seems reasonably robust.


using namespace std;

class PeakPtrs;


class PeakSeeds
{
  private:

    struct Interval
    {
      unsigned start;
      unsigned end;
      unsigned len;
      float depth;
      float minLevel;

      Peak * leftPtr;
      Peak * rightPtr;

      Interval()
      {
        start = 0;
        end = 0;
        len = 0;
        depth = 0.f;
        minLevel = 0.f;

        leftPtr = nullptr;
        rightPtr = nullptr;
      };
    };

    struct IntervalAnnotated
    {
      Interval * intervalPtr;
      unsigned indexStart;
      unsigned indexEnd;
      unsigned indexLen;
      bool usedFlag;

      IntervalAnnotated()
      {
        intervalPtr = nullptr;
        indexStart = 0;
        indexEnd = 0;
        indexLen = 0;
        usedFlag = false;
      };
    };


    list<Interval> intervals;

    vector<list<Interval *>> nestedIntervals;

    void logInterval(
      Peak * peak1,
      Peak * peak2,
      const float lowerMinValue,
      const float value,
      const float scale);

    void makeIntervals(
      PeakPool& peaks,
      const float scale);

    void setupReferences(
      vector<IntervalAnnotated>& intByStart,
      vector<IntervalAnnotated>& intByEnd,
      vector<IntervalAnnotated>& intByLength);

    void nestIntervals(
      vector<IntervalAnnotated>& intByStart,
      vector<IntervalAnnotated>& intByEnd,
      vector<IntervalAnnotated>& intByLength);

    void makeNestedIntervals();

    void mergeSimilarLists();

    void pruneListEnds();

    void markSeeds(
      PeakPool& peaks,
      PeakPtrs& peaksToSelect,
      Peak& peakAvg) const;

    void rebalanceCenters(
      PeakPool& peaks,
      vector<Peak>& peakCenters) const;


  public:

    PeakSeeds();

    ~PeakSeeds();

    void reset();

    void mark(
      PeakPool& peaks,
      vector<Peak>& peakCenters,
      const unsigned offset,
      const float scale);

    string str(
      const unsigned offset,
      const string& text) const;

};

#endif

