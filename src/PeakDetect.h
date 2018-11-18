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

  CarModels models;

  struct Gap
  {
    unsigned lower;
    unsigned upper;
    unsigned count;
  };

  enum PrintQuality
  {
    PRINT_SEED = 0,
    PRINT_WHEEL = 1,
    PRINT_BOGEY = 2,
    PRINT_SIZE = 3
  };

  typedef bool (PeakDetect::*CandFncPtr)(
    const Peak * p1, const Peak * p2) const;


    unsigned len;
    unsigned offset;
    list<Peak> peaks;
    list<Peak *> candidates;


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

    void reduceSmallRanges(
      const float rangeLimit,
      const bool preserveFlag);
    void reduceSmallAreas(const float areaLimit);

    void eliminateKinks();

    void estimateScale(Peak& scale);

    bool matchesModel(
      const CarDetect& car,
      unsigned& index,
      float& distance) const;

    bool findFourWheeler(
      const unsigned start,
      const unsigned end,
      const bool leftGapPresent,
      const bool rightGapPresent,
      const vector<unsigned>& peakNos,
      const vector<Peak *>& peakPtrs,
      CarDetect& car) const;

    bool findLastTwoOfFourWheeler(
      const unsigned start,
      const unsigned end,
      const bool rightGapPresent,
      const vector<unsigned>& peakNos,
      const vector<Peak *>& peakPtrs,
      CarDetect& car) const;

    bool findLastThreeOfFourWheeler(
      const unsigned start,
      const unsigned end,
      const vector<unsigned>& peakNos,
      const vector<Peak *>& peakPtrs,
      CarDetect& car,
      unsigned& numWheels) const;

    void markWheelPair(
      Peak& p1,
      Peak& p2,
      const string& text) const;

    void fixTwoWheels(
      Peak& p1,
      Peak& p2) const;

    void fixThreeWheels(
      Peak& p1,
      Peak& p2,
      Peak& p3) const;

    void fixFourWheels(
      Peak& p1,
      Peak& p2,
      Peak& p3,
      Peak& p4) const;

    void updateCars(
      vector<CarDetect>& cars,
      CarDetect& car);

    bool findCars(
      const unsigned start,
      const unsigned end,
      const bool leftGapPresent,
      const bool rightGapPresent,
      vector<CarDetect>& cars);

    void findWholeCars(vector<CarDetect>& cars);
    void findWholeInnerCars(vector<CarDetect>& cars);
    void findWholeFirstCar(vector<CarDetect>& cars);
    void findWholeLastCar(vector<CarDetect>& cars);

    void updateCarDistances(vector<CarDetect>& cars) const;

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
