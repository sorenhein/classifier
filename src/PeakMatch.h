#ifndef TRAIN_PEAKMATCH_H
#define TRAIN_PEAKMATCH_H

#include <list>
#include <vector>
#include <string>

#include "Peak.h"
#include "PeakPool.h"

#include "struct.h"

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
      double bestDistance;
      int match;
    };

    list<PeakWrapper> peaksWrapped;

    vector<Peak const *> trueMatches;

    void pos2time(
      const vector<double>& posTrue,
      const double speed,
      vector<double>& timesTrue) const;

    bool advance(list<PeakWrapper>::iterator& peak) const;

    double simpleScore(
      const vector<double>& timesTrue,
      const double offsetScore,
      const bool logFlag,
      double& shift);

    void setOffsets(
      const PeakPool& peaks,
      const vector<double>& timesTrue,
      vector<double>& offsetList) const;

    bool findMatch(
      const PeakPool& peaks,
      const vector<double>& timesTrue,
      double& shift);

    void correctTimesTrue(vector<double>& timesTrue) const;

    void printPeaks(
      const PeakPool& peaks,
      const vector<double>& timesTrue) const;

  public:

    PeakMatch();

    ~PeakMatch();

    void reset();

    void logPeakStats(
      const PeakPool& peaks,
      const vector<double>& posTrue,
      const string& trainTrue,
      const double speedTrue,
      PeakStats& peakStats);

};

#endif

