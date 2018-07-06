#include <iostream>
#include <iomanip>
#include <sstream>

#include "Database.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


Database::Database()
{
  carEntries.clear();
  trainEntries.clear();
  offCarMap.clear();
  offTrainMap.clear();

  sampleRate = 2000; // Default in Hz

  // Put a dummy car at position 0 which is not used.
  // We're going to put reversed cars by the negative of the
  // actual number, and this only works if we exclude 0.
  CarEntry car;
  car.officialName = "Dummy";
  carEntries.push_back(car);
}


Database::~Database()
{
}


void Database::setSampleRate(const int sampleRateIn)
{
  sampleRate = sampleRateIn;
}


void Database::logCar(const CarEntry& car)
{
  offCarMap[car.officialName] = carEntries.size();
  carEntries.push_back(car);
}


void Database::logTrain(const TrainEntry& train)
{
  offTrainMap[train.name] = trainEntries.size();
  trainEntries.push_back(train);
}


bool Database::getPerfectPeaks(
  const string& trainName,
  const string& country,
  vector<Peak>& peaks,
  const float speed,
  const int offset) const
{
  UNUSED(country);

  const int trainNo = Database::lookupTrainNumber(trainName);
  if (trainNo == -1)
    return false;

  peaks.clear();
  int pos = offset; // Start somewhere

  // Distances are in mm.
  // Speed is in km/h.
  // Sample rate is in Hz = samples/s.
  // samples = sample rate * distance / (speed/3.6) 
  const float factor = static_cast<float>(sampleRate * 3.6f) /
    (speed * 1000.f);
 
  const vector<int>& carNos = trainEntries[trainNo].carNumbers;
  const unsigned l = carNos.size();
  Peak peak;
  peak.value = 1.f;

  for (unsigned i = 0; i < l; i++)
  {
    const int carNo = carNos[i];
    if (carNo == 0)
    {
      cout << "Bad car number" << endl;
      return false;
    }
    else if (carNo > 0)
    {
      const CarEntry& carEntry = carEntries[carNo];

      pos += static_cast<int>(carEntry.distFrontToWheel * factor);

      peak.sampleNo = pos;
      peaks.push_back(peak); // First wheel, first pair
      pos += static_cast<int>(carEntry.distWheels * factor);

      peak.sampleNo = pos;
      peaks.push_back(peak); // Second wheel, first pair
      pos += static_cast<int>(carEntry.distPair * factor);

      peak.sampleNo = pos;
      peaks.push_back(peak); // First wheel, second pair
      pos += static_cast<int>(carEntry.distWheels * factor);

      peak.sampleNo = pos;
      peaks.push_back(peak); // Second wheel, second pair
      pos += static_cast<int>(carEntry.distWheelToBack * factor);
    }
    else
    {
      // Car is reversed.
      const CarEntry& carEntry = carEntries[-carNo];

      pos += static_cast<int>(carEntry.distWheelToBack * factor);

      peak.sampleNo = pos;
      peaks.push_back(peak); // Second wheel, second pair
      pos += static_cast<int>(carEntry.distWheels * factor);

      peak.sampleNo = pos;
      peaks.push_back(peak); // First wheel, second pair
      pos += static_cast<int>(carEntry.distPair * factor);

      peak.sampleNo = pos;
      peaks.push_back(peak); // Second wheel, first pair
      pos += static_cast<int>(carEntry.distWheels * factor);

      peak.sampleNo = pos;
      peaks.push_back(peak); // First wheel, first pair
      pos += static_cast<int>(carEntry.distFrontToWheel * factor);
    }
  }
  return true;
}


int Database::lookupCarNumber(const string& offName) const
{
  auto it = offCarMap.find(offName);
  if (it == offCarMap.end())
    return 0; // Invalid number
  else
    return it->second;
}


int Database::lookupTrainNumber(const string& offName) const
{
  auto it = offTrainMap.find(offName);
  if (it == offTrainMap.end())
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

