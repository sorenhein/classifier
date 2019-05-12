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
      FIND_CAR_PARTIAL = 1,
      FIND_CAR_DOWNGRADE = 2,
      FIND_CAR_NO_MATCH = 3,
      FIND_CAR_SIZE = 4
    };

    // Only a few methods actually change peaks.
    typedef FindCarType (PeakStructure::*FindCarPtr)(
      const CarModels& models,
      PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    struct FncGroup
    {
      FindCarPtr fptr;
      string name;
      unsigned number;
    };


    vector<list<FncGroup>> findMethods;

    list<PeakRange> ranges;

    vector<unsigned> hits;

    unsigned offset;


    bool findNumberedWheeler(
      const CarModels& models,
      const PeakRange& range,
      PeakPtrs& peakPtrs,
      const unsigned numWheels,
      CarDetect& car) const;

    void updateCarDistances(
      const CarModels& models,
      list<CarDetect>& cars) const;

    void updateModels(
      CarModels& models,
      CarDetect& car) const;

    bool fillPartialSides(
      CarDetect& car1,
      CarDetect& car2) const;


    FindCarType findCarByOrder(
      const CarModels& models,
      PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    FindCarType findPartialCarByQuality(
      const CarModels& models,
      const PeakFncPtr& fptr,
      const CarPosition carpos,
      PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    FindCarType findPartialFirstCarByQuality(
      const CarModels& models,
      PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    FindCarType findPartialInnerCarByQuality(
      const CarModels& models,
      PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    FindCarType findCarByPeaks(
      const CarModels& models,
      const PeakRange& range,
      PeakPtrs& peakPtrs,
      CarDetect& car) const;

    FindCarType findCarByQuality(
      const CarModels& models,
      const PeakFncPtr& fptr,
      PeakRange& range,
      CarDetect& car) const;

    FindCarType findCarByGreatQuality(
      const CarModels& models,
      PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    FindCarType findCarByGoodQuality(
      const CarModels& models,
      PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    FindCarType findCarByGeometry(
      const CarModels& models,
      PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    FindCarType findEmptyRange(
      const CarModels& models,
      PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    FindCarType findCarByPattern(
      const CarModels& models,
      PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    FindCarType findInnerCarByShort(
      const CarModels& models,
      PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;


    CarListIter updateRecords(
      const PeakRange& range,
      const CarDetect& car,
      CarModels& models,
      list<CarDetect>& cars);

    list<PeakRange>::iterator updateRanges(
      const CarListIter& carIt,
      list<PeakRange>::iterator& rit,
      const FindCarType& findFlag);

    bool loopOverMethods(
      CarModels& models,
      list<CarDetect>& cars,
      PeakPool& peaks,
      list<FncGroup>& findCarMethods);

    void fixSpuriousInterPeaks(
      const list<CarDetect>& cars,
      PeakPool& peaks) const;


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
      Imperfections& imperf) const;
};

#endif
