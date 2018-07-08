#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

#include "Database.h"
#include "const.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


Database::Database()
{
  carEntries.clear();
  trainEntries.clear();
  offCarMap.clear();
  offTrainMap.clear();

  sampleRate = SAMPLE_RATE; // Default in Hz

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
  const string officialName = train.officialName;

  if (train.symmetryFlag)
  {
    // One direction only.
    trainEntries.push_back(train);
    TrainEntry& t = trainEntries.back();
    t.officialName = officialName + "_N";
    offTrainMap[t.officialName] = trainEntries.size()-1;
    // Database::printAxlesCSV(t);
  }
  else
  {
    // Normal.
    trainEntries.push_back(train);
    TrainEntry& t = trainEntries.back();
    t.officialName = officialName + "_N";
    offTrainMap[t.officialName] = trainEntries.size()-1;
    // Database::printAxlesCSV(t);

    // Reversed.
    TrainEntry tr = train;

    const int aLast = tr.axles.back();
    const unsigned l = tr.axles.size();
    const vector<int> axles = tr.axles;
    for (unsigned i = 0; i < l; i++)
      tr.axles[i] = aLast - axles[l-i-1];

    tr.officialName = officialName + "_R";
    trainEntries.push_back(tr);
    offTrainMap[tr.officialName] = trainEntries.size()-1;
    // Database::printAxlesCSV(tr);
  }
}


bool Database::getPerfectPeaks(
  const string& trainName,
  vector<Peak>& peaks) const
{
  // Peaks are returned in mm.
  const int trainNo = Database::lookupTrainNumber(trainName);
  if (trainNo == -1)
    return false;

  Peak peak;
  peak.value = 1.f;
  peaks.clear();
  
  for (auto it: trainEntries[trainNo].axles)
  {
    peak.sampleNo = it;
    peaks.push_back(peak);
  }

  return true;
}


const CarEntry * Database::lookupCar(const int carNo) const
{
  if (carNo <= 0 || carNo >= static_cast<int>(carEntries.size()))
  {
    cout << "Bad car number" << endl;
    return nullptr;
  }
  else
    return &carEntries[carNo];
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


void Database::printAxlesCSV(const TrainEntry& t) const
{
  cout << t.officialName << ";";
  for (auto it: t.axles)
    cout << it << ";";

  cout << endl;
}

