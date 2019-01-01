#ifndef TRAIN_PEAKSTRUCTURE_H
#define TRAIN_PEAKSTRUCTURE_H

#include <list>
#include <string>

#include "CarDetect.h"
#include "PeakPool.h"
#include "PeakRange.h"

#include "struct.h"

using namespace std;

class CarModels;
class PeakProfile;
class Peak;


class PeakStructure
{
  private:

    enum FindCarType
    {
      FIND_CAR_MATCH = 0,
      FIND_CAR_DOWNGRADE = 1,
      FIND_CAR_NO_MATCH = 2,
      FIND_CAR_SIZE = 3
    };

    typedef FindCarType (PeakStructure::*FindCarPtr)(
      const CarModels& models,
      const PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    struct FncGroup
    {
      FindCarPtr fptr;
      string name;
    };


    list<FncGroup> findCarFunctions;

    list<PeakRange> ranges2;

    unsigned offset;


    bool matchesModel(
      const CarModels& models,
      const CarDetect& car,
      unsigned& index,
      float& distance) const;

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

    void updateCarDistances(
      const CarModels& models,
      list<CarDetect>& cars) const;

    void updateModels(
      CarModels& models,
      CarDetect& car) const;

    FindCarType findPartialCarByQualityNew(
      const CarModels& models,
      const PeakFncPtr& fptr,
      const unsigned numWheels,
      PeakRange& range,
      CarDetect& car) const;

    FindCarType findPartialFirstCarByQuality(
      const CarModels& models,
      const PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    FindCarType findCarByQualityNew(
      const CarModels& models,
      const PeakFncPtr& fptr,
      PeakRange& range,
      CarDetect& car) const;

    FindCarType findCarByGreatQuality(
      const CarModels& models,
      const PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    FindCarType findCarByGoodQuality(
      const CarModels& models,
      const PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    FindCarType findEmptyRange(
      const CarModels& models,
      const PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    bool isConsistent(const PeakPtrVector& closestPeaks) const;

    FindCarType findCarByGeometry(
      const CarModels& models,
      const PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    void makeRanges(
      const list<CarDetect>& cars,
      const PeakPool& peaks);

    bool fillPartialSides(
      CarModels& models,
      CarDetect& car1,
      CarDetect& car2) const;

    bool isWholeCar(const PeakPtrVector& pv) const;

    FindCarType findCarByOrder(
      const CarModels& models,
      const PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    list<PeakRange>::iterator updateRanges2(
      CarModels& models,
      const list<CarDetect>& cars,
      const list<CarDetect>::iterator& carIt,
      list<PeakRange>::iterator& rit,
      const FindCarType& findFlag);

    bool updateImperfections(
      const unsigned num,
      const bool firstCarFlag,
      Imperfections& imperf) const;

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
