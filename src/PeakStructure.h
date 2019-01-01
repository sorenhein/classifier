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

    struct PeakRange
    {
      // The car after the range
      list<CarDetect>::const_iterator carAfter; 
      PeakSource source;
      unsigned start;
      unsigned end;
      bool leftGapPresent;
      bool rightGapPresent;
      bool leftOriginal;
      bool rightOriginal;

      string order() const
      {
        if (leftOriginal)
          return "first car";
        else if (rightOriginal)
          return "last car";
        else
          return "intra car";
      };

      string str(const unsigned off = 0) const
      {
        return PeakRange::order() + ": " +
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
      FIND_CAR_NO_MATCH = 2,
      FIND_CAR_SIZE = 3
    };

    typedef FindCarType (PeakStructure::*FindCarPtr)(
      const PeakRange& range,
      const CarModels& models,
      const PeakPool& peaks,
      PeakPtrVector& peakPtrs,
      const PeakIterVector& peakIters,
      const PeakProfile& profile,
      CarDetect& car) const;

    struct FncGroup
    {
      FindCarPtr fptr;
      string name;
    };


    list<Recognizer> recognizers;

    list<FncGroup> findCarFunctions;

    list<PeakRange> ranges;
    list<PeakRange2> ranges2;

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

    FindCarType findCarByQuality(
      const PeakRange& range,
      const CarModels& models,
      const PeakPool& peaks,
      PeakPtrVector& peakPtrs,
      const PeakIterVector& peakIters,
      const PeakProfile& profile,
      CarDetect& car) const;

    FindCarType findEmptyRange(
      const PeakRange& range,
      const CarModels& models,
      const PeakPool& peaks,
      PeakPtrVector& peakPtrs,
      const PeakIterVector& peakIters,
      const PeakProfile& profile,
      CarDetect& car) const;

    bool getClosest(
      const list<unsigned>& carPoints,
      const PeakPool& peaks,
      const PeakFncPtr& fptr,
      PPciterator& pit,
      PeakPtrVector& closestPeaks,
      PeakPtrVector& skippedPeaks) const;

    bool isConsistent(const PeakPtrVector& closestPeaks) const;

    FindCarType findCarByGeometry(
      const PeakRange& range,
      const CarModels& models,
      const PeakPool& peaks,
      PeakPtrVector& peakPtrs,
      const PeakIterVector& peakIters,
      const PeakProfile& profile,
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
      const PeakRange& range,
      const CarModels& models,
      const PeakPool& peaks,
      PeakPtrVector& peakPtrs,
      const PeakIterVector& peakIters,
      const PeakProfile& profile,
      CarDetect& car) const;

    list<PeakRange>::iterator updateRanges(
      CarModels& models,
      const list<CarDetect>& cars,
      const list<CarDetect>::iterator& carIt,
      list<PeakRange>::iterator& rit,
      const FindCarType& findFlag);

    list<PeakRange2>::iterator updateRanges2(
      CarModels& models,
      const list<CarDetect>& cars,
      const list<CarDetect>::iterator& carIt,
      list<PeakRange2>::iterator& rit,
      const FindCarType& findFlag);

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
