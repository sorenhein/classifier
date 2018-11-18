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

    const list<Peak>::iterator collapsePeaks(
      const list<Peak>::iterator peak1,
      const list<Peak>::iterator peak2);

    void reduceSmallRanges(
      const float rangeLimit,
      const bool preserveFlag);
    void reduceSmallAreas(const float areaLimit);

    void eliminateKinks();

    void estimateScales();

    void setOrigPointers();

    void findFirstSize(
      const vector<unsigned>& dists,
      unsigned& lower,
      unsigned& upper,
      unsigned& counted,
      const unsigned lowerCount = 0) const;

    void findFirstSize(
      const vector<unsigned>& dists,
      Gap& gap,
      const unsigned lowerCount = 0) const;

    float getFirstPeakTime() const;

    bool checkQuantity(
      const unsigned actual,
      const unsigned reference,
      const float factor,
      const string& text) const;

    bool matchesModel(
      const CarDetect& car,
      unsigned& index) const;

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
      list<Peak *>& candidates,
      vector<CarDetect>& cars);

    bool bothSeed(
      const Peak * p1,
      const Peak * p2) const;

    bool formBogeyGap(
      const Peak * p1,
      const Peak * p2) const;

    void guessDistance(
      const list<Peak *>& candidates,
      const CandFncPtr fptr,
      Gap& gap) const;

    unsigned countWheels(const list<Peak *>& candidates) const;

    void markWheelPair(
      Peak& p1,
      Peak& p2,
      const string& text) const;

    void markBogeyShortGap(
      Peak& p1,
      Peak& p2,
      const string& text) const;

    void markBogeyLongGap(
      Peak& p1,
      Peak& p2,
      const string& text) const;

    void markSinglePeaks(list<Peak *>& candidates);

    void markBogeys(list<Peak *>& candidates);

    void markShortGaps(
      list<Peak *>& candidates,
      Gap& shortGap);

    void markLongGaps(
      list<Peak *>& candidates,
      const unsigned shortGapCount);

    void findInitialWholeCars(
      list<Peak *>& candidates,
      vector<CarDetect>& cars);

    void makeSeedAverage(
      const list<Peak *>& candidates,
      Peak& seed) const;

    void makeWheelAverages(
      const list<Peak *>& candidates,
      vector<Peak>& wheels) const;

    void makeBogeyAverages(
      const list<Peak *>& candidates,
      vector<Peak>& wheels) const;

    void printPeak(
      const Peak& peak,
      const string& text) const;

    void printScale(
      const Peak& scale,
      const string& text) const;

    void printCarStats(const string& text) const;

    void printCars(
      const vector<CarDetect>& cars,
      const string& text) const;

    void printPeaks(const vector<PeakTime>& timesTrue) const;

  public:

    PeakDetect();

    ~PeakDetect();

    void reset();

    void log(
      const vector<float>& samples,
      const unsigned offsetSamples);

    void reduce();

    void reduceNew();
    void reduceNewer();

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
