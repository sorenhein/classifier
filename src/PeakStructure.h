#ifndef TRAIN_PEAKSTRUCTURE_H
#define TRAIN_PEAKSTRUCTURE_H

#include <list>
#include <string>

#include "CarDetect.h"
#include "CarModels.h"
#include "PeakPool.h"
#include "PeakRange.h"
#include "PeakPattern.h"
#include "PeakGeneral.h"

using namespace std;

class PeakProfile;
class Peak;


class PeakStructure
{
  private:

    // Only a few methods actually change peaks.
    typedef FindCarType (PeakStructure::*FindCarPtr)(
      PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    struct FncGroup
    {
      FindCarPtr fptr;
      string name;
      unsigned number;
    };


    CarModels models;

    list<CarDetect> cars;


    vector<list<FncGroup>> findMethods;

    list<PeakRange> ranges;

    vector<unsigned> hits;

    unsigned offset;


    bool findNumberedWheeler(
      const PeakRange& range,
      PeakPtrs& peakPtrs,
      const unsigned numWheels,
      CarDetect& car) const;

    void updateCarDistances();

    void updateModels(CarDetect& car);

    bool fillPartialSides(
      CarDetect& car1,
      CarDetect& car2) const;


    FindCarType findCarByOrder(
      PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    FindCarType findCarByPeaks(
      const PeakRange& range,
      PeakPtrs& peakPtrs,
      CarDetect& car) const;

    FindCarType findCarByQuality(
      PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    FindCarType findEmptyRange(
      PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    FindCarType findCarByPattern(
      PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;

    FindCarType findCarBySpacing(
      PeakPool& peaks,
      PeakRange& range,
      CarDetect& car) const;


    CarListIter updateRecords(
      const PeakRange& range,
      const CarDetect& car);

    list<PeakRange>::iterator updateRanges(
      const CarListIter& carIt,
      list<PeakRange>::iterator& rit,
      const FindCarType& findFlag);

    bool loopOverMethods(
      PeakPool& peaks,
      const list<FncGroup>& findCarMethods,
      const bool onceFlag);

    void fixSpuriousInterPeaks(PeakPool& peaks) const;


    void printWheelCount(
      const PeakPool& peaks,
      const string& text) const;

    void printCars(const string& text) const;

    void printCarStats(const string& text) const;

  public:

    PeakStructure();

    ~PeakStructure();

    void reset();

    void markCars(
      PeakPool& peaks,
      const unsigned offsetIn);

    bool hasGaps() const;
    
    // to clean up

    void getPeaksInfo(PeaksInfo& peaksInfo) const;

    void pushInfo(
      Peak const * pptr,
      const double sampleRate,
      const float tOffset,
      const unsigned carNo,
      unsigned& peakNo,
      unsigned& peakNoInCar,
      PeaksInfo& peaksInfo) const;

    void getPeaksInfo(
      const PeakPool& peaks,
      PeaksInfo& peaksInfo,
      const double sampleRate) const;
};

#endif
