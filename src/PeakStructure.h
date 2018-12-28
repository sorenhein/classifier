#ifndef TRAIN_PEAKSTRUCTURE_H
#define TRAIN_PEAKSTRUCTURE_H

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
      CarDetect const * carAfter; // The car after the condition
      PeakSource source;
      unsigned start;
      unsigned end;
      bool leftGapPresent;
      bool rightGapPresent;
      string text;

      string str(const unsigned off = 0)
      {
        return text + ": " +
          (leftGapPresent ? "(gap)" : "(no gap)") + " " +
          to_string(start + off) + "-" +
          to_string(end + off) + " " +
          (rightGapPresent ? "(gap)" : "(no gap)") + "\n";
      };
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
      const PeakPtrVector& peakPtrs,
      CarDetect& car) const;

    bool findLastThreeOfFourWheeler(
      const CarModels& models,
      const PeakCondition& condition,
      const PeakPtrVector& peakPtrs,
      CarDetect& car) const;

    bool findFourWheeler(
      const CarModels& models,
      const PeakCondition& condition,
      const PeakPtrVector& peakPtrs,
      CarDetect& car) const;

    bool findNumberedWheeler(
      const CarModels& models,
      const PeakCondition& condition,
      const PeakPtrVector& peakPtrs,
      const unsigned numWheels,
      CarDetect& car) const;

    void markUpPeaks(
      PeakPtrVector& peakPtrsNew,
      const unsigned numPeaks) const;

    void markDownPeaks(PeakPtrVector& peakPtrsUnused) const;

    void updatePeaks(
      PeakPtrVector& peakPtrsNew,
      PeakPtrVector& peakPtrsUnused,
      const unsigned numPeaks) const;

    void downgradeAllPeaks(PeakPtrVector& peakPtrsNew) const;

    void getWheelsByQuality(
      const PeakPtrVector& peakPtrs,
      const PeakQuality quality,
      PeakPtrVector& peakPtrsNew,
      PeakPtrVector& peakPtrsUnused) const;

    void splitPeaks(
      const PeakPtrVector& peakPtrs,
      const PeakCondition& condition,
      PeakCondition& condition1,
      PeakCondition& condition2) const;

    void updateCarDistances(
      const CarModels& models,
      list<CarDetect>& cars) const;

    void updateModels(
      CarModels& models,
      CarDetect& car) const;

    bool findCarByQuality(
      const PeakCondition& condition,
      CarModels& models,
      CarDetect& car,
      PeakPool& peaks) const;

    bool findMissingCar(
      const PeakCondition& condition,
      CarModels& models,
      CarDetect& car,
      PeakPool& peaks) const;

    void makeConditions(
      const list<CarDetect>& cars,
      const PeakPool& peaks,
      list<PeakCondition>& conditions) const;

    void fillPartialSides(
      CarModels& models,
      list<CarDetect>& cars);

    void seekGaps(
      PPciterator pitLeft,
      PPciterator pitRight,
      const PeakPool& peaks,
      PeakCondition& condition) const;

    bool isWholeCar(const PeakPtrVector& pv) const;

    void findWholeCars(
      CarModels& models,
      list<CarDetect>& cars,
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
      const string& text) const;

    void printCars(
      const list<CarDetect>& cars,
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
      list<CarDetect>& cars,
      PeakPool& peaks,
      const unsigned offsetIn);
    
    void markImperfections(
      const list<CarDetect>& cars,
      const PeakPool& peaks,
      Imperfections& imperf) const;
};

#endif
