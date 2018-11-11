#include <iostream>
#include <iomanip>
#include <sstream>

#include "CarModels.h"
#include "Except.h"
#include "errors.h"


CarModels::CarModels()
{
  CarModels::reset();
}


CarModels::~CarModels()
{
}


void CarModels::reset()
{
  models.clear();
}


void CarModels::append()
{
  models.emplace_back(CarModel());
}


void CarModels::operator += (const CarDetect& car)
{
  if (models.size() == 0)
    THROW(ERR_MODELS_EMPTY, "No model to increment");

  CarModels::add(car, models.size()-1);
}


void CarModels::add(
  const CarDetect& car,
  const unsigned index)
{
  if (index >= models.size())
    THROW(ERR_MODELS_EMPTY, "No model to increment");

  CarModel& m = models[index];

  m.carSum += car;

  m.peaksSum.increment(car.getPeaksPtr(), m.peaksNumbers);

  car.increment(m.gapNumbers);

  CarModels::average(index);
}


unsigned CarModels::size() const
{
  return models.size();
}


void CarModels::average(const unsigned index)
{
  CarModel& m = models[index];
  m.carAvg = m.carSum;
  m.carAvg.averageGaps(m.gapNumbers);

  m.peaksAvg = m.peaksSum;
  m.peaksAvg.average(m.peaksNumbers);
}


bool CarModels::fillSides(CarDetect& car) const
{
  if (models.size() == 1)
    return car.fillSides(models[0].carAvg);
  else
    THROW(ERR_MODELS_EMPTY, "Haven't learned this yet: " + 
      to_string(models.size()));

  // TODO: If this is ever needed, first loop over models to find
  // the closest model and then use this.
}


bool CarModels::findClosest(
  const CarDetect& car,
  float& distance,
  unsigned& index) const
{
  if (models.size() == 0)
    return false;

  distance = numeric_limits<float>::max();
  for (unsigned i = 0; i < models.size(); i++)
  {
    const CarModel& m = models[i];
    float d = m.carAvg.distance(car);
    if (d < distance)
    {
      distance = d;
      index = i;
    }
  }
  return true;
}


bool CarModels::rightBogeyPlausible(const CarDetect& car) const
{
  for (auto& m: models)
  {
    if (car.rightBogeyPlausible(m.carAvg))
      return true;
  }
  return false;
}


bool CarModels::sideGapsPlausible(const CarDetect& car) const
{
  for (auto& m: models)
  {
    if (car.sideGapsPlausible(m.carAvg))
      return true;
  }
  return false;
}


bool CarModels::gapsPlausible(const CarDetect& car) const
{
  for (auto& m: models)
  {
    if (car.gapsPlausible(m.carAvg))
      return true;
  }
  return false;
}


string CarModels::str() const
{
  if (models.size() == 0)
    THROW(ERR_MODELS_EMPTY, "No model to print");

  stringstream ss;
  ss << models.front().carAvg.strHeaderGaps();

  for (auto& m: models)
    ss << m.carAvg.strGaps();

  return ss.str() + "\n";
}

