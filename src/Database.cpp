#include <iostream>
#include <iomanip>
#include <sstream>

#include "Database.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


Database::Database()
{
  carEntries.clear();
  trainEntries.clear();
  offNameMap.clear();
}


Database::~Database()
{
}


void Database::logCar(const CarEntry& car)
{
  offNameMap[car.officialName] = carEntries.size();
  carEntries.push_back(car);
}


void Database::logTrain(const TrainEntry& train)
{
  trainEntries.push_back(train);
}


void Database::getPerfectPeaks(
  const string& trainName,
  const string& country,
  vector<Peak>& peaks,
  const int speed) const
{
  UNUSED(trainName);
  UNUSED(country);
  UNUSED(peaks);
  UNUSED(speed);
}


int Database::lookupCarNumber(const string& offName) const
{
  auto it = offNameMap.find(offName);
  if (it == offNameMap.end())
    return -1;
  else
    return it->second;
}


string Database::lookupTrainName(const unsigned trainNo) const
{
  if (trainNo >= trainEntries.size())
    return "Bad index";
  else
    return trainEntries[trainNo].name;
}


void Database::lookupCar(
  const DatabaseCar& dbCar,
  const string& country,
  const int year,
  list<int>& carList) const
{
  UNUSED(dbCar);
  UNUSED(country);
  UNUSED(year);
  UNUSED(carList);
}


int Database::lookupTrain(
  const vector<list<int>>& carTypes,
  unsigned& firstCar,
  unsigned& lastCar,
  unsigned& carsMissing) const
{
  UNUSED(carTypes);
  UNUSED(firstCar);
  UNUSED(lastCar);
  UNUSED(carsMissing);
  return 0;
}

