#ifndef TRAIN_PEAKSTRUCTURE_H
#define TRAIN_PEAKSTRUCTURE_H

#include <vector>
#include <list>
#include <string>

#include "struct.h"

using namespace std;

class CarDetect;
class CarModels;
class Peak;

enum PeakSource
{
  PEAK_SOURCE_FIRST = 1,
  PEAK_SOURCE_INNER = 2,
  PEAK_SOURCE_LAST = 4,
  PEAK_SOURCE_SIZE = 8
};


class PeakStructure
{
  private:

    struct Profile
    {
      // Peaks that are labeled both w.r.t. bogeys and wheels
      // (left bogey left/right wheel, right bogey left/right wheel).
      vector<unsigned> bogeyWheels;
      // Peaks that are only labeled as wheels (left/right).
      vector<unsigned> wheels;
      // Unlabeled peaks by quality: ***, **, *, poor quality.
      vector<unsigned> stars;
      unsigned sumGreat;
      unsigned sum;
    };

    enum PeakQuality
    {
      PEAK_QUALITY_GREAT = 0,
      PEAK_QUALITY_GOOD = 1,
      PEAK_QUALITY_ACCEPTABLE = 2,
      PEAK_QUALITY_POOR = 3,
      PEAK_QUALITY_SIZE = 4
    };

    struct PeakCondition
    {
      PeakSource source;
      unsigned start;
      unsigned end;
      bool leftGapPresent;
      bool rightGapPresent;
    };


    unsigned offset;
    Profile profile;

    unsigned source;
    unsigned matrix[3][24];


    void resetProfile();

    bool matchesModel(
      const CarModels& models,
      const CarDetect& car,
      unsigned& index,
      float& distance) const;

    bool findNumberedWheeler(
      const CarModels& models,
      const PeakCondition& condition,
      const vector<unsigned>& peakNos,
      const vector<Peak *>& peakPtrs,
      const unsigned numWheels,
      CarDetect& car) const;

    bool findFourWheeler(
      const CarModels& models,
      const PeakCondition& condition,
      const vector<unsigned>& peakNos,
      const vector<Peak *>& peakPtrs,
      CarDetect& car) const;

    bool findLastTwoOfFourWheeler(
      const CarModels& models,
      const PeakCondition& condition,
      const vector<unsigned>& peakNos,
      const vector<Peak *>& peakPtrs,
      CarDetect& car) const;

    bool findLastThreeOfFourWheeler(
      const CarModels& models,
      const unsigned start,
      const unsigned end,
      const vector<unsigned>& peakNos,
      const vector<Peak *>& peakPtrs,
      CarDetect& car,
      unsigned& numWheels) const;

    bool findLastThreeOfFourWheelerNew(
      const CarModels& models,
      const PeakCondition& condition,
      const vector<unsigned>& peakNos,
      const vector<Peak *>& peakPtrs,
      CarDetect& car) const;

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

    void updatePeaks(
      vector<Peak *>& peakPtrsNew,
      vector<Peak *>& peakPtrsUnused,
      const unsigned numPeaks) const;

    void downgradeAllPeaks(vector<Peak *>& peakPtrsNew) const;

    void getGreatWheels(
      const vector<Peak *>& peakPtrs,
      const vector<unsigned>& peakNos,
      vector<Peak *>& peakPtrsNew,
      vector<unsigned>& peakNosNew,
      vector<Peak *>& peakPtrsUnused) const;

    void getGoodWheels(
      const vector<Peak *>& peakPtrs,
      const vector<unsigned>& peakNos,
      vector<Peak *>& peakPtrsNew,
      vector<unsigned>& peakNosNew,
      vector<Peak *>& peakPtrsUnused) const;

    void updateCars(
      CarModels& models,
      vector<CarDetect>& cars,
      CarDetect& car) const;

    bool findCars(
      const unsigned start,
      const unsigned end,
      const bool leftGapPresent,
      const bool rightGapPresent,
      CarModels& models,
      vector<CarDetect>& cars,
      list<Peak *>& candidates); // const;

    string x(const unsigned v) const;

    void makeProfile(const vector<Peak *>& peakPtrs);

    string strProfile(const unsigned origin) const;

    bool findCarsNew(
      const PeakCondition& condition,
      CarModels& models,
      vector<CarDetect>& cars,
      list<Peak *>& candidates); // const;

    void findWholeCars(
      CarModels& models,
      vector<CarDetect>& cars,
      list<Peak *>& candidates) const;

    void findWholeInnerCars(
      CarModels& models,
      vector<CarDetect>& cars,
      list<Peak *>& candidates); // const;

    void findWholeFirstCar(
      CarModels& models,
      vector<CarDetect>& cars,
      list<Peak *>& candidates); // const;

    void findWholeLastCar(
      CarModels& models,
      vector<CarDetect>& cars,
      list<Peak *>& candidates); // const;


    void updateCarDistances(
      const CarModels& models,
      vector<CarDetect>& cars) const;

    float getFirstPeakTime() const;

    void printPeak(
      const Peak& peak,
      const string& text) const;

    void printRange(
      const unsigned start,
      const unsigned end,
      const string& text) const;

    void printWheelCount(
      const list<Peak *>& candidates,
      const string& text) const;

    void printCars(
      const vector<CarDetect>& cars,
      const string& text) const;

    void printCarStats(
      const CarModels& models,
      const string& text) const;

    void printPeaksCSV(const vector<PeakTime>& timesTrue) const;

  public:

    PeakStructure();

    ~PeakStructure();

    void reset();

    void markCars(
      CarModels& models,
      vector<CarDetect>& cars,
      list<Peak *>& candidates,
      const unsigned offsetIn);

  void printPaths() const;
};

#endif
