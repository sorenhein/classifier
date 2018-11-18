#ifndef TRAIN_PEAKDETECT_H
#define TRAIN_PEAKDETECT_H

#include <vector>
#include <list>
#include <string>
#include <sstream>

#include "Peak.h"
#include "CarModels.h"
#include "CarDetect.h"
#include "CarPeaks.h"
#include "struct.h"

using namespace std;

class PeakStats;


class PeakDetect
{
  private:

    unsigned len;
    unsigned offset;
    list<Peak> peaks;
    list<Peak *> candidates;
    CarModels models;


    float integrate(
      const vector<float>& samples,
      const unsigned i0,
      const unsigned i1) const;

    void annotate();

    bool check(const vector<float>& samples) const;

    void logFirst(const vector<float>& samples);
    void logLast(const vector<float>& samples);

    const list<Peak>::iterator collapsePeaks(
      const list<Peak>::iterator peak1,
      const list<Peak>::iterator peak2);

    void reduceSmallPeaks(
      const PeakParam param,
      const float paramLimit,
      const bool preserveFlag);

    void eliminateKinks();

    void estimateScale(Peak& scale);

    float getFirstPeakTime() const;

    void printPeak(
      const Peak& peak,
      const string& text) const;

    void printRange(
      const unsigned start,
      const unsigned end,
      const string& text) const;

    void printWheelCount(const string& text) const;

    void printCars(
      const vector<CarDetect>& cars,
      const string& text) const;

    void printCarStats(const string& text) const;

    void printPeaksCSV(const vector<PeakTime>& timesTrue) const;

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

    void printAllPeaks(const string& text = "") const;
};

#endif
