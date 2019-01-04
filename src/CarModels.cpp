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


void CarModels::recalculate(const list<CarDetect>& cars)
{
  for (auto& model: models)
    model.reset();

  for (auto& car: cars)
  {
    if (car.isReversed())
    {
      CarDetect revCar = car;
      revCar.reverse();
      CarModels::add(revCar, car.index());
    }
    else
      CarModels::add(car, car.index());
  }
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
  {
    cout << "CarModels::fillSides warning:  2+ models\n";
    return false;
    // THROW(ERR_MODELS_EMPTY, "Haven't learned 2+ models yet: " + 
      // to_string(models.size()));
  }

  // TODO: If this is ever needed, first loop over models to find
  // the closest model and then use this.
}


void CarModels::getCar(
  CarDetect& car,
  const unsigned index) const
{
  car = models[index].carAvg;
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


bool CarModels::matchesDistance(
  const CarDetect& car,
  const float& limit,
  float& distance,
  unsigned& index) const
{
  if (! CarModels::findClosest(car, distance, index))
    return false;
  else
    return (distance <= limit);
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


bool CarModels::twoWheelerPlausible(const CarDetect& car) const
{
  if (! CarModels::sideGapsPlausible(car))
    return false;

  // As we don't have a complete car, we'll at least require the
  // right bogey gap to be similar to something we've seen.

  return CarModels::rightBogeyPlausible(car);
}


bool CarModels::threeWheelerPlausible(const CarDetect& car) const
{
  if (! CarModels::sideGapsPlausible(car))
    return false;

  // As we don't have a complete car, we'll at least require the
  // right bogey gap to be similar to something we've seen.

  if (! CarModels::rightBogeyPlausible(car))
    return false;

  return car.midGapPlausible();
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

  unsigned mno = 0;
  for (auto& m: models)
    ss << m.carAvg.strGaps(mno++);

  return ss.str() + "\n";
}

