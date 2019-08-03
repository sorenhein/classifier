#ifndef TRAIN_PEAKMATCH_H
#define TRAIN_PEAKMATCH_H

#include <list>
#include <vector>
#include <string>

#include "Peak.h"
#include "PeakPool.h"

class PeakStats;

// This is only for output statistics.  It was particularly useful
// when we were looking only at peak qualities, but now that we try
// to detect entire cars, it is less important.


using namespace std;


class PeakMatch
{
  private:

    struct PeakWrapper
    {
      Peak const * peakPtr;
      float bestDistance;
      int match;
    };

    list<PeakWrapper> peaksWrapped;

    vector<Peak const *> trueMatches;

    void pos2time(
      const vector<float>& posTrue,
      const double speed,
      vector<float>& timesTrue) const;

    bool advance(list<PeakWrapper>::iterator& peak) const;

    float simpleScore(
      const vector<float>& timesTrue,
      const float offsetScore,
      const bool logFlag,
      float& shift);

    void setOffsets(
      const PeakPool& peaks,
      const vector<float>& timesTrue,
      vector<float>& offsetList) const;

    bool findMatch(
      const PeakPool& peaks,
      const vector<float>& timesTrue,
      float& shift);

    void correctTimesTrue(vector<float>& timesTrue) const;

    void printPeaks(
      const PeakPool& peaks,
      const vector<float>& timesTrue) const;

  public:

    PeakMatch();

    ~PeakMatch();

    void reset();

    void logPeakStats(
      const PeakPool& peaks,
      const vector<float>& posTrue,
      // const string& trainTrue,
      const double speedTrue,
      PeakStats& peakStats);

};

#endif

