#ifndef TRAIN_PEAKDETECT_H
#define TRAIN_PEAKDETECT_H

#include <vector>
#include <list>
#include <string>

#include "Peak.h"
#include "struct.h"

using namespace std;


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



    float integral(
      const vector<float>& samples,
      const unsigned i0,
      const unsigned i1) const;

    void annotate();

    bool check(const vector<float>& samples) const;

    float estimateScale(vector<float>& data) const;

    void estimateScales();

    void runKmeansOnce(
      const Koptions& koptions,
      vector<Peak>& clusters,
      float& distance);

    void collapsePeaks(
      list<Peak>::iterator peak1,
      list<Peak>::iterator peak2);

    void reduceSmallAreas(const float areaLimit);

    void eliminateKinks();

    void pos2time(
      const vector<PeakPos>& posTrue,
      const double speed,
      vector<PeakTime>& timesTrue) const;

    double simpleScore(
      const vector<PeakTime>& timesTrue,
      const double offsetScore);

    bool findMatch(const vector<PeakTime>& timesTrue);

  public:

    PeakDetect();

    ~PeakDetect();

    void reset();

    void log(
      const vector<float>& samples,
      const unsigned offsetSamples);

    void reduce(
      const vector<PeakPos>& posTrue,
      const double speedTrue);

    void makeSynthPeaks(vector<float>& synthPeaks) const;

    void getPeakTimes(vector<PeakTime>& times) const;

    void print() const;
};

#endif
