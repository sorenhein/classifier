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

  // Peaks between abutting interval lists tend to be large and thus
  // to be "real" peaks.

  struct PeakEntry
  {
    Peak * peakPtr;
    Peak * nextLargePeakPtr;
    Peak * prevLargePeakPtr;
  };

  struct Gap
  {
    unsigned lower;
    unsigned upper;
    unsigned count;
  };

  typedef bool (PeakDetect::*PeakFncPtr)(
    const PeakEntry& pe1, const PeakEntry& pe2) const;

  typedef bool (PeakDetect::*CandFncPtr)(
    const Peak * p1, const Peak * p2) const;


    unsigned len;
    unsigned offset;
    list<Peak> peaks;
    PeakData scales;
    Peak scalesList;

    unsigned numCandidates;
    unsigned numTentatives;


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
      const vector<PeakEntry>& peaksAnnot,
      const vector<unsigned>& peakNos,
      const vector<unsigned>& peakIndices,
      CarDetect& car) const;

    bool findLastTwoOfFourWheeler(
      const unsigned start,
      const unsigned end,
      const bool rightGapPresent,
      const vector<PeakEntry>& peaksAnnot,
      const vector<unsigned>& peakNos,
      const vector<unsigned>& peakIndices,
      CarDetect& car) const;

    bool findLastThreeOfFourWheeler(
      const unsigned start,
      const unsigned end,
      const vector<PeakEntry>& peaksAnnot,
      const vector<unsigned>& peakNos,
      const vector<unsigned>& peakIndices,
      CarDetect& car,
      unsigned& numWheels) const;

    void fixTwoWheels(
      vector<PeakEntry>& peaksAnnot,
      const vector<unsigned>& peakIndices,
      const unsigned p0,
      const unsigned p1) const;

    void fixThreeWheels(
      vector<PeakEntry>& peaksAnnot,
      const vector<unsigned>& peakIndices,
      const unsigned p0,
      const unsigned p1,
      const unsigned p2) const;

    void fixFourWheels(
      vector<PeakEntry>& peaksAnnot,
      const vector<unsigned>& peakIndices,
      const unsigned p0,
      const unsigned p1,
      const unsigned p2,
      const unsigned p3) const;

    void updateCars(
      vector<CarDetect>& cars,
      CarDetect& car);

    bool findCars(
      const unsigned start,
      const unsigned end,
      const bool leftGapPresent,
      const bool rightGapPresent,
      vector<PeakEntry>& peaksAnnot,
      vector<CarDetect>& cars);

    bool bothTall(
      const PeakEntry& pe1,
      const PeakEntry& pe2) const;

    bool bothSeed(
      const Peak * p1,
      const Peak * p2) const;

    bool areBogeyGap(
      const PeakEntry& pe1,
      const PeakEntry& pe2) const;

    bool formBogeyGap(
      const Peak * p1,
      const Peak * p2) const;

    bool formOpenBogeyGap(
      const Peak * p1,
      const Peak * p2) const;

    void guessDistance(
      const vector<PeakEntry>& peaksAnnot,
      const PeakFncPtr fptr,
      unsigned& distLower,
      unsigned& distUpper,
      unsigned& count) const;

    void guessDistance(
      const list<Peak *>& candidates,
      const CandFncPtr fptr,
      Gap& gap) const;

    unsigned countWheels(const vector<PeakEntry>& peaksAnnot) const;

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

    void markSinglePeaks(
      vector<PeakEntry>& peaksAnnot,
      list<Peak *>& candidates);

    void markBogeys(list<Peak *>& candidates);

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
