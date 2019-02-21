#ifndef TRAIN_PEAKPARTIAL_H
#define TRAIN_PEAKPARTIAL_H

#include <vector>
#include <string>

using namespace std;

class Peak;


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

    vector<Peak *> peaks;
    unsigned lastIndex;

    // If a peak is not present, the place to look for it.
    vector<unsigned> lower;
    vector<unsigned> upper;
    vector<unsigned> target;

    // The index in peakPtrsUsed corresponding to the stored peak.
    vector<unsigned> indexUsed;

    // Number of peaks found.
    unsigned numUsed;


    bool dominates(const PeakPartial& p2) const;

    bool samePeaks(const PeakPartial& p2) const;

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
      Peak * pptr);

    void registerFinished();

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

    void getPeaks(
      vector<Peak *>& peaksUsed,
      vector<Peak *>& peaksUnused) const;

    void printSituation(
      const vector<Peak *>& peaksUsed,
      const vector<Peak *>& peaksUnused,
      const unsigned offset) const;

    string strTarget(
      const unsigned peakNo,
      const unsigned offset) const;

    string strHeader() const;

    string str(const unsigned offset) const;
};

#endif
