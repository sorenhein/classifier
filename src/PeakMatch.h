#ifndef TRAIN_PEAKMATCH_H
#define TRAIN_PEAKMATCH_H

#include <list>
#include <vector>
#include <string>

#include "Peak.h"
#include "PeakStats.h"

#include "struct.h"

// This is only for output statistics.  It was particularly useful
// when we were looking only at peak qualities, but now that we try
// to detect entire cars, it is less important.


using namespace std;


class PeakMatch
{
  private:

    void pos2time(
      const vector<PeakPos>& posTrue,
      const double speed,
      vector<PeakTime>& timesTrue) const;

    bool advance(
      const list<Peak>& peaks,
      list<Peak>::const_iterator& peak) const;

    double simpleScore(
      const list<Peak>& peaks,
      const vector<PeakTime>& timesTrue,
      const double offsetScore,
      double& shift);

    void setOffsets(
      const list<Peak>& peaks,
      const vector<PeakTime>& timesTrue,
      vector<double>& offsetList) const;

    bool findMatch(
      const list<Peak>& peaks,
      const vector<PeakTime>& timesTrue,
      double& shift);

    PeakType findCandidate(
      const list<Peak>& peaks,
      const double time,
      const double shift) const;

    void printPeaks(
      const list<Peak>& peaks,
      const vector<PeakTime>& timesTrue) const;

  public:

    PeakMatch();

    ~PeakMatch();

    void reset();

    void logPeakStats(
      const list<Peak>& peaks,
      const vector<PeakPos>& posTrue,
      const string& trainTrue,
      const double speedTrue,
      PeakStats& peakStats);

};

#endif

