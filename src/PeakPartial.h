#ifndef TRAIN_PEAKPARTIAL_H
#define TRAIN_PEAKPARTIAL_H

#include <vector>
#include <string>

#include "Peak.h"
#include "PeakSlots.h"

using namespace std;

class PeakPtrs;


class PeakPartial
{
  private:

    // The model and reverse flag of the partial fit.
    unsigned modelNo;
    bool reverseFlag;

    // true if the partial fit has any fit at all.
    bool modelUsedFlag;

    // true if the partial fit might still fit even better.
    bool aliveFlag;

    // skippedFlag is set if the peak from which we're starting was
    // not in fact found, so we have to count up two gaps and not just one.
    // (We should try to guess the gap of the abutting car, rather than
    // just doubling our own gap.)
    // If we're at peak #1, we'll have to add g0+g1 and not just g1.
    // If we get to two skips, we give up.
    //
    // |    #3    #2        #1    #0    |
    //   g4    g2      g2      g1    g0
    bool skippedFlag;
    
    // true if the last attempted fit worked out.
    bool newFlag;

    struct Component
    {
      Peak const * peak;

      // If a peak is not present, the place to look for it.
      unsigned lower;
      unsigned upper;
      unsigned target;

      // The index in peakPtrsUsed corresponding to the stored peak.
      unsigned indexUsed;
    };

    vector<Component> comps;

    unsigned lastIndex;

    // Number of peaks found.
    unsigned numUsed;

    PeakSlots peakSlots;

    // Some typical quantities that would otherwise get passed around.
    unsigned bogey;
    unsigned mid;
    bool verboseFlag;


    void movePtr(
      const unsigned indexFrom,
      const unsigned indexTo);

    bool dominates(const PeakPartial& p2) const;

    bool samePeaks(const PeakPartial& p2) const;
    bool samePeakValues(const PeakPartial& p2) const;

    void extend(const PeakPartial& p2);

    bool closeEnoughFull(
      const unsigned index,
      const unsigned peakNo,
      const unsigned posNo) const;

    bool closeEnough(
      const unsigned index,
      const unsigned peakNo,
      const unsigned posNo) const;

    bool merge(const PeakPartial& p2);

    Peak const * locatePeak(
      const unsigned lower,
      const unsigned upper,
      const unsigned hint,
      const PeakFncPtr& fptr,
      vector<Peak const *>& peakPtrsUsed,
      unsigned& indexUsed) const;

    Peak const * lookForPeak(
      const unsigned start,
      const unsigned step,
      const float& smallFactor,
      const float& largeFactor,
      const bool upFlag,
      vector<Peak const *>& peakPtrsUsed,
      unsigned& indexUsedOut);

    void recoverPeaks0001(vector<Peak const *>& peakPtrsUsed);
    void recoverPeaks0011(vector<Peak const *>& peakPtrsUsed);
    void recoverPeaks0101(vector<Peak const *>& peakPtrsUsed);
    void recoverPeaks0111New(vector<Peak const *>& peakPtrsUsed);
    void recoverPeaks1011(vector<Peak const *>& peakPtrsUsed);
    void recoverPeaks1100(vector<Peak const *>& peakPtrsUsed);
    void recoverPeaks1101(vector<Peak const *>& peakPtrsUsed);
    void recoverPeaks1110(vector<Peak const *>& peakPtrsUsed);

    void recoverPeaksShared(
      Peak const *& pptrA,
      Peak const *& pptrB,
      const unsigned npA,
      const unsigned npB,
      const unsigned indexUsedA,
      const unsigned indexUsedB,
      const string& source);

    void moveUnused(
      vector<Peak const *>& peaksUsed,
      vector<Peak const *>& peaksUnused) const;

    void getPeaksFromUsed(
      const unsigned bogeyTypical,
      const unsigned longTypical,
      const bool verboseFlag,
      vector<Peak const *>& peaksUsed);

    string strIndex(
      const unsigned peakNo,
      const unsigned offset) const;

    string strEntry(const unsigned n) const;

  public:

    PeakPartial();

    ~PeakPartial();

    void reset();

    void init(
      const unsigned modelNoIn,
      const bool reverseFlagIn,
      const unsigned startIn);

    void registerRange(
      const unsigned peakNo,
      const unsigned lowerIn,
      const unsigned upperIn);

    void registerIndexUsed(
      const unsigned peakNo,
      const unsigned indexUsedIn);

    void registerPtr(
      const unsigned peakNo,
      Peak const * pptr);

    void registerPtr(
      const unsigned peakNo,
      Peak const * pptr,
      const unsigned indexUsedIn);

    void registerFinished();

    bool getRange(
      const unsigned peakNo,
      unsigned& lowerOut,
      unsigned& upperOut) const;

    void getPeak(
      const unsigned peakNo,
      Peak& peak) const;

    bool supersede(const PeakPartial& p2);

    unsigned number() const;
    unsigned count() const;
    unsigned latest() const;
    bool reversed() const;
    bool skipped() const;
    bool alive() const;
    bool used() const;
    bool hasPeak(const unsigned peakNo) const;
    bool hasRange(const unsigned peakNo) const;

    unsigned numMatches() const;

    void makeCodes(
      const PeakPtrs& peaksUsed,
      const unsigned offset);

    void getPeaks(
      const unsigned bogeyTypical,
      const unsigned longTypical,
      vector<Peak const *>& peaksUsed,
      vector<Peak const *>& peaksUnused);

    void printSituation(const bool firstFlag) const;

    string strTarget(
      const unsigned peakNo,
      const unsigned offset) const;

    string strHeader() const;

    string str(const unsigned offset) const;
};

#endif
