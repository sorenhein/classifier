#ifndef TRAIN_PEAKSTRUCTURE_H
#define TRAIN_PEAKSTRUCTURE_H

#include <list>
#include <string>

#include "CarDetect.h"
#include "PeakPool.h"

#include "struct.h"

using namespace std;

class CarModels;
class Peak;


class PeakStructure
{
  private:

    struct PeakRange
    {
      // The car after the range
      list<CarDetect>::const_iterator carAfter; 
      PeakSource source;
      unsigned start;
      unsigned end;
      bool leftGapPresent;
      bool rightGapPresent;
      bool stuckFlag;
      string text;

      string str(const unsigned off = 0) const
      {
        return text + ": " +
          (leftGapPresent ? "(gap)" : "(no gap)") + " " +
          to_string(start + off) + "-" +
          to_string(end + off) + " " +
          (rightGapPresent ? "(gap)" : "(no gap)") + "\n";
      };
    };

    enum FindCarType
    {
      FIND_CAR_MATCH = 0,
      FIND_CAR_DOWNGRADE = 1,
      FIND_CAR_SIZE = 2
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
      const PeakRange& range,
      const PeakPtrVector& peakPtrs,
      CarDetect& car) const;

    bool findLastThreeOfFourWheeler(
      const CarModels& models,
      const PeakRange& range,
      const PeakPtrVector& peakPtrs,
      CarDetect& car) const;

    bool findFourWheeler(
      const CarModels& models,
      const PeakRange& range,
      const PeakPtrVector& peakPtrs,
      CarDetect& car) const;

    bool findNumberedWheeler(
      const CarModels& models,
      const PeakRange& range,
      const PeakPtrVector& peakPtrs,
      const unsigned numWheels,
      CarDetect& car) const;

    void markUpPeaks(
      PeakPtrVector& peakPtrsNew,
      const unsigned numPeaks) const;

    void markDownPeaks(PeakPtrVector& peakPtrs) const;

    void getWheelsByQuality(
      const PeakPtrVector& peakPtrs,
      const PeakQuality quality,
      PeakPtrVector& peakPtrsNew,
      PeakPtrVector& peakPtrsUnused) const;

    void updateCarDistances(
      const CarModels& models,
      list<CarDetect>& cars) const;

    void updateModels(
      CarModels& models,
      CarDetect& car) const;

    bool findCarByQuality(
      const PeakRange& range,
      CarModels& models,
      CarDetect& car,
      PeakPool& peaks,
      FindCarType& findFlag) const;

    bool getClosest(
      const list<unsigned>& carPoints,
      const PeakPool& peaks,
      const PeakFncPtr& fptr,
      PPciterator& pit,
      PeakPtrVector& closestPeaks,
      PeakPtrVector& skippedPeaks) const;

    bool isConsistent(const PeakPtrVector& closestPeaks) const;

    bool findCarByGeometry(
      const PeakRange& range,
      CarModels& models,
      CarDetect& car,
      PeakPool& peaks,
      FindCarType& findFlag) const;

    void makeRanges(
      const list<CarDetect>& cars,
      const PeakPool& peaks,
      list<PeakRange>& ranges) const;

    bool fillPartialSides(
      CarModels& models,
      CarDetect& car1,
      CarDetect& car2) const;

    void seekGaps(
      PPciterator pitLeft,
      PPciterator pitRight,
      const PeakPool& peaks,
      PeakRange& range) const;

    bool isWholeCar(const PeakPtrVector& pv) const;

    void findWholeCars(
      CarModels& models,
      list<CarDetect>& cars,
      PeakPool& peaks) const;

    list<PeakRange>::iterator updateRanges(
      CarModels& models,
      const list<CarDetect>& cars,
      const list<CarDetect>::iterator& carIt,
      list<PeakRange>::iterator& rit,
      list<PeakRange>& ranges,
      const FindCarType& findFlag) const;

    bool updateImperfections(
      const unsigned num,
      const bool firstCarFlag,
      Imperfections& imperf) const;

    void printRange(
      const PeakRange& range,
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
    
    bool markImperfections(
      const list<CarDetect>& cars,
      const PeakPool& peaks,
      Imperfections& imperf) const;
};

#endif
