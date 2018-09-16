#ifndef TRAIN_PEAKDETECT_H
#define TRAIN_PEAKDETECT_H

#include <vector>
#include <list>
#include <string>

#include "Peak.h"
#include "struct.h"

using namespace std;

class PeakStats;


class PeakDetect
{
  private:

    struct Koptions
    {
      unsigned numClusters;
      unsigned numIterations;
      float convCriterion;
    };

    unsigned len;
    unsigned offset;
    list<Peak> peakList;
    PeakData scales;
    Peak scalesList;



    float integrate(
      const vector<float>& samples,
      const unsigned i0,
      const unsigned i1) const;

    void annotate();

    bool check(const vector<float>& samples) const;

    void logFirst(const vector<float>& samples);
    void logLast(const vector<float>& samples);

    void collapsePeaks(
      const list<Peak>::iterator peak1,
      const list<Peak>::iterator peak2);

    void reduceSmallAreas(const float areaLimit);

    void eliminateKinks();

    float estimateScale(vector<float>& data) const;

    void estimateScales();

    void pickStartingClusters(
      vector<Peak>& clusters,
      const unsigned n) const;

    void runKmeansOnce(
      const Koptions& koptions,
      vector<Peak>& clusters,
      float& distance);

    void pos2time(
      const vector<PeakPos>& posTrue,
      const double speed,
      vector<PeakTime>& timesTrue) const;

    bool advance(list<Peak>::iterator& peak) const;

    double simpleScore(
      const vector<PeakTime>& timesTrue,
      const double offsetScore,
      const bool logFlag,
      double& shift);

    void setOffsets(
      const vector<PeakTime>& timesTrue,
      vector<double>& offsetList) const;

    bool findMatch(
      const vector<PeakTime>& timesTrue,
      double& shift);

    PeakType findCandidate(
      const double time,
      const double shift) const;

  public:

    PeakDetect();

    ~PeakDetect();

    void reset();

    void log(
      const vector<float>& samples,
      const unsigned offsetSamples);

    void reduce();

    void logPeakStats(
      const vector<PeakPos>& posTrue,
      const string& trainTrue,
      const double speedTrue,
      PeakStats& peakStats);

    void makeSynthPeaks(vector<float>& synthPeaks) const;

    void getPeakTimes(vector<PeakTime>& times) const;

    void print() const;
};

#endif
