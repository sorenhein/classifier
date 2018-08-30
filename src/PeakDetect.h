#ifndef TRAIN_PEAKDETECT_H
#define TRAIN_PEAKDETECT_H

#include <vector>
#include <string>

#include "struct.h"

using namespace std;


class PeakDetect
{
  private:

    unsigned len;
    unsigned offset;
    vector<PeakData> peaks;


    float integral(
      const vector<float>& samples,
      const unsigned i0,
      const unsigned i1,
      const float refval) const;

    bool check(const vector<float>& samples) const;

    void reduceToRuns();

    void remakeFlanks(const vector<unsigned>& survivors);

    void estimateAreaRanges(
      float& veryLargeArea,
      float& normalLargeArea) const;

    void estimatePeakSize(float& negativePeakSize) const;

    void reduceSmallRuns(const float areaLimit);

    void reduceNegativeDips(const float peakLimit);

    void reduceTransientLeftovers();

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

    void reduce();

    void makeSynthPeaks(vector<float>& synthPeaks) const;

    void getPeakTimes(vector<PeakTime>& times) const;

    void print(const bool activeFlag = true) const;

    void printList(
      const vector<unsigned>& indices,
      const bool activeFlag = true) const;
};

#endif
