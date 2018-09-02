#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

#include "Database.h"
#include "read.h"
#include "struct.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


Database::Database()
{
  carEntries.clear();
  trainEntries.clear();
  offCarMap.clear();
  offTrainMap.clear();

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


bool Database::select(
  const string& countries,
  const unsigned minAxles,
  const unsigned maxAxles)
{
  selectedTrains.clear();

  // Make a map of country strings
  map<string, int> countryMap;
  if (countries != "ALL")
  {
    const size_t c = countDelimiters(countries, ",");
    vector<string> vCountries(c+1);
    vCountries.clear();
    tokenize(countries, vCountries, ",");
    for (auto& v: vCountries)
      countryMap[v] = 1;
  }

  for (auto& train: trainEntries)
  {
    if (countries != "ALL" &&
        countryMap.find(train.officialName) == countryMap.end())
      continue;

    const unsigned l = train.axles.size();
    if (l < minAxles || l > maxAxles)
      continue;

    selectedTrains.push_back(train.officialName);
  }

  return (selectedTrains.size() > 0);
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
    t.reverseFlag = false;
    offTrainMap[t.officialName] = trainEntries.size()-1;
  }
  else
  {
    // Normal.
    trainEntries.push_back(train);
    TrainEntry& t = trainEntries.back();
    t.officialName = officialName + "_N";
    t.reverseFlag = false;
    offTrainMap[t.officialName] = trainEntries.size()-1;

    // Reversed.
    TrainEntry tr = train;

    const int aLast = tr.axles.back();
    const unsigned l = tr.axles.size();
    const vector<int> axles = tr.axles;
    for (unsigned i = 0; i < l; i++)
      tr.axles[i] = aLast - axles[l-i-1];

    tr.officialName = officialName + "_R";
    tr.reverseFlag = true;
    trainEntries.push_back(tr);
    offTrainMap[tr.officialName] = trainEntries.size()-1;
  }
}


bool Database::logSensor(const SensorData& sdata)
{
  auto it = sensors.find(sdata.name);
  if (it != sensors.end())
  {
    cout << "Sensor " << sdata.name << " already logged\n";
    return false;
  }

  DatabaseSensor& sensor = sensors[sdata.name];
  sensor.country = sdata.country;
  sensor.type = sdata.type;
  return true;
}


unsigned Database::axleCount(const unsigned trainNo) const
{
  return trainEntries[trainNo].axles.size();
}


bool Database::getPerfectPeaks(
  const string& trainName,
  vector<PeakPos>& peaks) const // In m
{
  const int trainNo = Database::lookupTrainNumber(trainName);
  return Database::getPerfectPeaks(static_cast<unsigned>(trainNo), peaks);
}


bool Database::getPerfectPeaks(
  const unsigned trainNo,
  vector<PeakPos>& peaks) const // In m
{
  if (trainNo >= trainEntries.size())
    return false;

  PeakPos peak;
  peak.value = 1.f;
  peaks.clear();
  
  for (auto it: trainEntries[trainNo].axles)
  {
    peak.pos = it / 1000.; // Convert from mm to m
    peaks.push_back(peak);
  }

  return true;
}


const Clusters * Database::getClusters(
  const unsigned trainNo,
  const unsigned clusterSize) const
{
  const unsigned tno = static_cast<unsigned>(trainNo);
  if (tno >= trainEntries.size())
  {
    cout << "Train number is too large\n";
    return nullptr;
  }

  if (clusterSize < 2 ||
      clusterSize > trainEntries[tno].clusterList.size() + 1)
  {
    return nullptr;
  }

  return &trainEntries[tno].clusterList[clusterSize-2];
}


const CarEntry * Database::lookupCar(const int carNo) const
{
  if (carNo <= 0 || carNo >= static_cast<int>(carEntries.size()))
  {
    cout << "Bad car number" << endl;
    return nullptr;
  }
  else
    return &carEntries[static_cast<unsigned>(carNo)];
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
    return static_cast<int>(it->second);
}


string Database::lookupTrainName(const unsigned trainNo) const
{
  if (trainNo >= trainEntries.size())
    return "Bad index";
  else
    return trainEntries[trainNo].officialName;
}


string Database::lookupSensorCountry(const string& sensor) const
{
  auto it = sensors.find(sensor);
  if (it == sensors.end())
    return "";
  else
    return it->second.country;
}


string Database::lookupTrainCountry(const unsigned trainNo) const
{
  // TODO: Just pick the first one for now.  The simulation loop
  // should probably consider all trains in all countries.
  if (trainNo >= trainEntries.size())
    return "Bad index";
  else
    return trainEntries[trainNo].countries[0];
}


bool Database::trainIsInCountry(
  const unsigned trainNo,
  const string& country) const
{
  if (trainNo >= trainEntries.size())
    return false;

  for (auto& c: trainEntries[trainNo].countries)
  {
    if (country == c)
      return true;
  }
  return false;
}


bool Database::trainsShareCountry(
  const unsigned trainNo1,
  const unsigned trainNo2) const
{
  const TrainEntry& t1 = trainEntries[trainNo1];
  const TrainEntry& t2 = trainEntries[trainNo2];

  for (auto& c1: t1.countries)
  {
    for (auto& c2: t2.countries)
    {
      if (c1 == c2)
        return true;
    }
  }
  return false;
}


bool Database::trainIsReversed(const unsigned trainNo) const
{
  return trainEntries[trainNo].reverseFlag;
}


void Database::printAxlesCSV(const TrainEntry& t) const
{
  cout << t.officialName << ";";
  for (auto it: t.axles)
    cout << fixed << setprecision(3) << it << ";";

  cout << endl;
}

