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


void CarModels::add(
  const CarDetect& car,
  const unsigned index)
{
  if (index >= models.size())
    THROW(ERR_MODELS_EMPTY, "No model to increment");

  CarModel& m = models[index];

  if (car.isReversed())
  {
    CarDetect revCar = car;
    revCar.reverse();
    m.carSum += revCar;
    m.peaksSum.increment(revCar.getPeaksPtr());
  }
  else
  {
    m.carSum += car;
    m.peaksSum.increment(car.getPeaksPtr());
  }

  m.number++;

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


bool CarModels::empty(const unsigned indexIn) const
{
  return (models[indexIn].number == 0);
}


unsigned CarModels::size() const
{
  return models.size();
}


unsigned CarModels::available()
{
  // First look for an unused one.
  for (unsigned mno = 0; mno < models.size(); mno++)
  {
    if (CarModels::empty(mno))
      return mno;
  }

  // If that fails, add one.
  models.emplace_back(CarModel());
  return models.size() - 1;
}


void CarModels::average(const unsigned index)
{
  CarModel& m = models[index];
  m.carAvg = m.carSum;
  m.carAvg.averageGaps(m.number);

  m.peaksAvg = m.peaksSum;
  m.peaksAvg.average(m.number);
}


void CarModels::getCar(
  CarDetect& car,
  const unsigned index) const
{
  car = models[index].carAvg;
}


bool CarModels::findDistance(
  const CarDetect& car,
  const bool partialFlag,
  const unsigned index,
  MatchData& match) const
{
  const CarModel& m = models[index];
  if (m.number == 0)
    return false;

  m.carAvg.distanceSymm(car, partialFlag, match);
  return true;
}


bool CarModels::findClosest(
  const CarDetect& car,
  const bool partialFlag,
  MatchData& match) const
{
  if (models.size() == 0)
    return false;

  match.distance = numeric_limits<float>::max();
  MatchData matchRunning;
  for (unsigned i = 0; i < models.size(); i++)
  {
    if (! CarModels::findDistance(car, partialFlag, i, matchRunning))
      continue;

    if (matchRunning.distance < match.distance)
    {
      match.distance = matchRunning.distance;
      match.index = i;
      match.reverseFlag = matchRunning.reverseFlag;
    }
  }
  return true;
}


bool CarModels::matchesDistance(
  const CarDetect& car,
  const float& limit,
  const bool partialFlag,
  MatchData& match) const
{
  if (! CarModels::findClosest(car, partialFlag, match))
    return false;
  else
    return (match.distance <= limit);
}


bool CarModels::matchesDistance(
  const CarDetect& car,
  const float& limit,
  const bool partialFlag,
  const unsigned index,
  MatchData& match) const
{
  if (! CarModels::findDistance(car, partialFlag, index, match))
    return false;
  else
    return (match.distance <= limit);
}


bool CarModels::matchesPartial(
  const PeakPtrVector& peakPtrs,
  const float& limit,
  MatchData& match,
  Peak& peakCand) const
{
  if (peakPtrs.size() != 3)
    THROW(ERR_NO_PEAKS, "No model to print");

  for (auto& m: models)
  {
    if (m.carAvg.distancePartialSymm(peakPtrs, limit, match, peakCand))
      return true;
  }
  return false;
}


bool CarModels::rightBogeyPlausible(const CarDetect& car) const
{
  for (auto& m: models)
  {
    if (m.number > 0 && car.rightBogeyPlausible(m.carAvg))
      return true;
  }
  return false;
}


bool CarModels::sideGapsPlausible(const CarDetect& car) const
{
  for (auto& m: models)
  {
    if (m.number > 0 && car.sideGapsPlausible(m.carAvg))
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
    if (m.number > 0 && car.gapsPlausible(m.carAvg))
      return true;
  }
  return false;
}


unsigned CarModels::getGap(
  const unsigned mno,
  const bool reverseFlag,
  const bool specialFlag,
  const bool skippedFlag,
  const unsigned peakNo) const
{
  if (mno >= models.size())
    return false;

  return models[mno].carAvg.getGap(reverseFlag, specialFlag,
    skippedFlag, peakNo);
}


string CarModels::str() const
{
  if (models.size() == 0)
    THROW(ERR_MODELS_EMPTY, "No model to print");

  stringstream ss;
  ss << models.front().carAvg.strHeaderGaps();

  unsigned mno = 0;
  for (auto& m: models)
  {
    if (m.number > 0)
      ss << m.carAvg.strGaps(mno);
    mno++;
  }

  return ss.str() + "\n";
}

