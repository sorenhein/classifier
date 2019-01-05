#ifndef TRAIN_CARMODELS_H
#define TRAIN_CARMODELS_H

#include <vector>
#include <string>

#include "CarDetect.h"
#include "CarPeaks.h"

using namespace std;


class CarModels
{
  private:

    struct CarModel
    {
      CarDetect carSum;
      CarPeaks peaksSum;

      CarDetect carAvg;
      CarPeaks peaksAvg;

      CarPeaksNumbers peaksNumbers;
  
      // TODO Do we still need this per gap?
      CarDetectNumbers gapNumbers;
      unsigned number;

      void reset()
      {
        carSum.reset();
        peaksSum.reset();
        carAvg.reset();
        peaksAvg.reset();
        peaksNumbers.reset();
        gapNumbers.reset();
        number = 0;
      };
    };

    vector<CarModel> models;

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

    void getCar(
      CarDetect& car,
      const unsigned index) const;

    bool findClosest(
      const CarDetect& car,
      MatchData& match) const;

    bool matchesDistance(
      const CarDetect& car,
      const float& limit,
      MatchData& match) const;

    bool rightBogeyPlausible(const CarDetect& car) const;
    bool sideGapsPlausible(const CarDetect& car) const;
    bool twoWheelerPlausible(const CarDetect& car) const;
    bool threeWheelerPlausible(const CarDetect& car) const;
    bool gapsPlausible(const CarDetect& car) const;

    string str() const;
};

#endif

