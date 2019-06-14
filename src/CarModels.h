#ifndef TRAIN_CARMODELS_H
#define TRAIN_CARMODELS_H

#include <vector>
#include <string>
#include <sstream>

#include "CarDetect.h"
#include "CarPeaks.h"

using namespace std;


struct ModelData
{
  // TODO Unused/unusable?
  unsigned index;

  bool fullFlag;
  unsigned lenPP; // Length peak-to-peak, so excluding end gaps

  bool gapLeftFlag;
  unsigned gapLeft;

  bool gapRightFlag;
  unsigned gapRight;

  bool containedFlag;
  unsigned containedIndex;
  bool containedReverseFlag;

  bool symmetryFlag;

  void reset()
  {
    fullFlag = false;
    gapLeftFlag = false;
    gapRightFlag = false;
    containedFlag = false;
    symmetryFlag = false;
  };

  string str() const
  {
    stringstream ss;
    ss << setw(12) << left << "index" << index << "\n" <<
      setw(12) << "p-p length" << 
        lenPP << "\n" << 
      setw(12) << "gap left" <<
        (gapLeftFlag ? to_string(gapLeft) : "-") << "\n" << 
      setw(12) << "gap right" <<
        (gapRightFlag ? to_string(gapRight) : "-") << "\n" << 
      setw(12) << "contained" <<
        (containedFlag ? to_string(containedIndex) +
          (containedReverseFlag ? "R" : "") : "-") << endl;
    return ss.str();
  };
};


class CarModels
{
  private:

    struct CarModel
    {
      CarDetect carSum;
      CarPeaks peaksSum;

      CarDetect carAvg;
      CarPeaks peaksAvg;

      ModelData data;

      unsigned number;

      void reset()
      {
        carSum.reset();
        peaksSum.reset();
        carAvg.reset();
        peaksAvg.reset();
        data.reset();
        number = 0;
      };
    };

    vector<CarModel> models;

    void characterize();

    void average(const unsigned index);


  public:

    CarModels();

    ~CarModels();

    void reset();

    void add(
      const CarDetect& car,
      const unsigned index);

    void recalculate(const list<CarDetect>& cars);

    bool empty(const unsigned indexIn) const;
    unsigned size() const;
    unsigned available();
    unsigned count(const unsigned indexIn) const;
    bool hasAnEndGap() const;

    void getTypical(
      unsigned& bogieTypical,
      unsigned& longTypical,
      unsigned& sideTypical) const;

    void getCar(
      CarDetect& car,
      const unsigned index) const;

    ModelData const * getData(const unsigned index) const;

    void getCarPoints(
      const unsigned index,
      list<unsigned>& carPoints) const;

    bool findDistance(
      const CarDetect& car,
      const bool partialFlag,
      const unsigned index,
      MatchData& match) const;

    bool findClosest(
      const CarDetect& car,
      const bool partialFlag,
      MatchData& match) const;

    bool matchesDistance(
      const CarDetect& car,
      const float& limit,
      const bool partialFlag,
      MatchData& match) const;

    bool matchesDistance(
      const CarDetect& car,
      const float& limit,
      const bool partialFlag,
      const unsigned index,
      MatchData& match) const;

    bool rightBogiePlausible(const CarDetect& car) const;
    bool sideGapsPlausible(const CarDetect& car) const;
    bool twoWheelerPlausible(const CarDetect& car) const;
    bool threeWheelerPlausible(const CarDetect& car) const;
    bool gapsPlausible(const CarDetect& car) const;

    unsigned getGap(
      const unsigned mno,
      const bool reverseFlag,
      const bool specialFlag,
      const bool skippedFlag,
      const unsigned peakNo) const;

    string str() const;
};

#endif

