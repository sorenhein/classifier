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
  
      CarDetectNumbers gapNumbers;
    };

    vector<CarModel> models;

    void average(const unsigned index);


  public:

    CarModels();

    ~CarModels();

    void reset();

    // Makes a new, empty model at the end of the list.
    void append();

    // Increments the last model in the list.
    void operator += (const CarDetect& car);

    void add(
      const CarDetect& car,
      const unsigned index);

    unsigned size() const;

    bool fillSides(CarDetect& car) const;

    void getCar(
      CarDetect& car,
      const unsigned index) const;

    bool findClosest(
      const CarDetect& car,
      float& distance,
      unsigned& index) const;

    bool rightBogeyPlausible(const CarDetect& car) const;
    bool sideGapsPlausible(const CarDetect& car) const;
    bool gapsPlausible(const CarDetect& car) const;

    string str() const;
};

#endif

