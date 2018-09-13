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
    vector<PeakData> peaks;
    PeakData scales;
    Peak scalesList;

    list<Peak> peakList;


    float integral(
      const vector<float>& samples,
      const unsigned i0,
      const unsigned i1,
      const float refval) const;

    float integralList(
      const vector<float>& samples,
      const unsigned i0,
      const unsigned i1) const;

    bool check(const vector<float>& samples) const;

    bool checkList(const vector<float>& samples) const;

    void reduceToRuns();

    void remakeFlanks(const vector<unsigned>& survivors);

    void estimateAreaRanges(
      float& veryLargeArea,
      float& normalLargeArea) const;

    void estimatePeakSize(float& negativePeakSize) const;

    float estimateScale(vector<float>& data) const;

    void estimateScales();
    void estimateScalesList();

    void normalizePeaks(vector<PeakData>& normalPeaks);

    float metric(
      const PeakData& np1,
      const PeakData& np2) const;

    void runKmeansOnce(
      const Koptions& koptions,
      const vector<PeakData>& normalPeaks,
      vector<PeakData>& clusters,
      vector<unsigned>& assigns,
      float& distance) const;

    void collapsePeaks(
      list<Peak>::iterator peak1,
      list<Peak>::iterator peak2);

    void reduceSmallAreas(const float areaLimit);
    void reduceSmallAreasList(const float areaLimit);

    void reduceNegativeDips(const float peakLimit);

    void reduceTransientLeftovers();

    void eliminateSmallAreas();

    void eliminateKinks();
    void eliminateKinksList();

    void eliminatePositiveMinima();

    void printHeader() const;

    void printPeak(
      const PeakData& peak,
      const unsigned index) const;

  public:

    PeakDetect();

    ~PeakDetect();

    void reset();

    void log(
      const vector<float>& samples,
      const unsigned offsetSamples);

    void logList(
      const vector<float>& samples,
      const unsigned offsetSamples);

    void reduce();
    void reduceNew();

    void makeSynthPeaks(vector<float>& synthPeaks) const;
    void makeSynthPeaksList(vector<float>& synthPeaks) const;

    void getPeakTimes(vector<PeakTime>& times) const;
    void getPeakTimesList(vector<PeakTime>& times) const;

    void print(const bool activeFlag = true) const;

    void printList() const;

    void printList(
      const vector<unsigned>& indices,
      const bool activeFlag = true) const;
};

#endif
