#ifndef TRAIN_PEAKSTRUCTURE_H
#define TRAIN_PEAKSTRUCTURE_H

#include <vector>
#include <list>
#include <string>

#include "PeakPool.h"

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
      const vector<Peak *>& peakPtrs,
      CarDetect& car) const;

    bool findLastThreeOfFourWheeler(
      const CarModels& models,
      const PeakCondition& condition,
      const vector<Peak *>& peakPtrs,
      CarDetect& car) const;

    bool findFourWheeler(
      const CarModels& models,
      const PeakCondition& condition,
      const vector<Peak *>& peakPtrs,
      CarDetect& car) const;

    bool findNumberedWheeler(
      const CarModels& models,
      const PeakCondition& condition,
      const vector<Peak *>& peakPtrs,
      const unsigned numWheels,
      CarDetect& car) const;

    void markUpPeaks(
      vector<Peak *>& peakPtrsNew,
      const unsigned numPeaks) const;

    void markDownPeaks(vector<Peak *>& peakPtrsUnused) const;

    void updatePeaks(
      vector<Peak *>& peakPtrsNew,
      vector<Peak *>& peakPtrsUnused,
      const unsigned numPeaks) const;

    void downgradeAllPeaks(vector<Peak *>& peakPtrsNew) const;

    void getWheelsByQuality(
      const vector<Peak *>& peakPtrs,
      const PeakQuality quality,
      vector<Peak *>& peakPtrsNew,
      vector<Peak *>& peakPtrsUnused) const;

    void splitPeaks(
      const vector<Peak *>& peakPtrs,
      const PeakCondition& condition,
      PeakCondition& condition1,
      PeakCondition& condition2) const;

    void updateCarDistances(
      const CarModels& models,
      vector<CarDetect>& cars) const;

    void updateCars(
      CarModels& models,
      vector<CarDetect>& cars,
      CarDetect& car) const;

    bool findCarsInInterval(
      const PeakCondition& condition,
      CarModels& models,
      vector<CarDetect>& cars,
      PeakPool& peaks,
      list<Peak *>& candidates) const;

    void findMissingCar(
      const PeakCondition& condition,
      CarModels& models,
      vector<CarDetect>& cars,
      PeakPool& peaks,
      list<Peak *>& candidates) const;

    void findMissingCars(
      PeakCondition& condition,
      CarModels& models,
      vector<CarDetect>& cars,
      PeakPool& peaks,
      list<Peak *>& candidates) const;

    void findWholeCars(
      CarModels& models,
      vector<CarDetect>& cars,
      PeakPool& peaks) const;

    void updateImperfections(
      const unsigned num,
      const bool firstCarFlag,
      Imperfections& imperf) const;

    void printRange(
      const PeakCondition& condition,
      const string& text) const;

    void printWheelCount(
      const PeakPool& peaks,
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
      PeakPool& peaks,
      list<Peak *>& candidates,
      const unsigned offsetIn);
    
    void markImperfections(
      const vector<CarDetect>& cars,
      const PeakPool& peaks,
      const list<Peak *>& candidates,
      Imperfections& imperf) const;
};

#endif
