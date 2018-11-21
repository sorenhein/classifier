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


class PeakStructure
{
  private:

    struct PeakCondition
    {
      PeakSource source;
      unsigned start;
      unsigned end;
      bool leftGapPresent;
      bool rightGapPresent;
      string text;
    };


    list<Recognizer> recognizers;

    unsigned offset;


    void setCarRecognizers();

    bool matchesModel(
      const CarModels& models,
      const CarDetect& car,
      unsigned& index,
      float& distance) const;

    bool findLastTwoOfFourWheeler(
      const CarModels& models,
      const PeakCondition& condition,
      const vector<unsigned>& peakNos,
      const vector<Peak *>& peakPtrs,
      CarDetect& car) const;

    bool findLastThreeOfFourWheeler(
      const CarModels& models,
      const PeakCondition& condition,
      const vector<unsigned>& peakNos,
      const vector<Peak *>& peakPtrs,
      CarDetect& car) const;

    bool findFourWheeler(
      const CarModels& models,
      const PeakCondition& condition,
      const vector<unsigned>& peakNos,
      const vector<Peak *>& peakPtrs,
      CarDetect& car) const;

    bool findNumberedWheeler(
      const CarModels& models,
      const PeakCondition& condition,
      const vector<unsigned>& peakNos,
      const vector<Peak *>& peakPtrs,
      const unsigned numWheels,
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

    void getWheelsByQuality(
      const vector<Peak *>& peakPtrs,
      const vector<unsigned>& peakNos,
      const PeakQuality quality,
      vector<Peak *>& peakPtrsNew,
      vector<unsigned>& peakNosNew,
      vector<Peak *>& peakPtrsUnused) const;

    void updateCars(
      CarModels& models,
      vector<CarDetect>& cars,
      CarDetect& car) const;

    void splitPeaks(
      const vector<Peak *>& peakPtrs,
      const PeakCondition& condition,
      PeakCondition& condition1,
      PeakCondition& condition2) const;

    bool findCars(
      const PeakCondition& condition,
      CarModels& models,
      vector<CarDetect>& cars,
      list<Peak *>& candidates); // const;

    void findWholeCars(
      CarModels& models,
      vector<CarDetect>& cars,
      list<Peak *>& candidates) const;

    void findMissingCar(
      const PeakCondition& condition,
      CarModels& models,
      vector<CarDetect>& cars,
      list<Peak *>& candidates); // const;

    void findMissingCars(
      PeakCondition& condition,
      CarModels& models,
      vector<CarDetect>& cars,
      list<Peak *>& candidates); // const;

    void updateCarDistances(
      const CarModels& models,
      vector<CarDetect>& cars) const;

    float getFirstPeakTime() const;



    void printRange(
      const PeakCondition& condition,
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

  public:

    PeakStructure();

    ~PeakStructure();

    void reset();

    void markCars(
      CarModels& models,
      vector<CarDetect>& cars,
      list<Peak *>& candidates,
      const unsigned offsetIn);
};

#endif
