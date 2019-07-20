#include "Database.h"


Database::Database()
{
  carDB.reset();
  trainEntries.clear();
  offCarMap.clear();
  offTrainMap.clear();
}


Database::~Database()
{
}


void Database::readCarFile(const string& fname)
{
  carDB.readFile(fname);
}


bool Database::appendAxles(
  const int carNo,
  int& posRunning,
  vector<int>& axles) const
{
  return carDB.appendAxles(carNo, posRunning, axles);
}


void Database::readTrainFile(const string& fname)
{
  trainDB.readFile(carDB, correctionDB, fname);
}


void Database::readCorrectionFile(const string& fname)
{
  correctionDB.readFile(fname);
}


void Database::logTrain(const TrainEntry& train)
{
  const string officialName = train.officialName;

  // Normal direction.
  trainEntries.push_back(train);
  TrainEntry& t = trainEntries.back();
  t.officialName = officialName + "_N";
  t.reverseFlag = false;
  offTrainMap[t.officialName] = trainEntries.size()-1;

  if (! train.symmetryFlag)
  {
    // Reversed as well.
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


bool Database::logSensor(
  const string& name,
  const string& country,
  const string& stype)
{
  auto it = sensors.find(name);
  if (it != sensors.end())
    return false;

  DatabaseSensor& sensor = sensors[name];
  sensor.country = country;
  sensor.type = stype;
  return true;
}


bool Database::select(
  const list<string>& countries,
  const unsigned minAxles,
  const unsigned maxAxles)
{
  // Used in preparation for looping using begin() and end().

  selectedTrains.clear();
  if (countries.empty())
    return false;

  const bool allFlag = (countries.size() == 1 && countries.front() == "ALL");
  map<string, int> countryMap;
  if (! allFlag)
  {
    // Make a map of country strings
    for (auto& c: countries)
      countryMap[c] = 1;
  }

  for (auto& train: trainEntries)
  {
    if (! allFlag && 
        countryMap.find(train.officialName) == countryMap.end())
      continue;

    const unsigned l = train.axles.size();
    if (l < minAxles || l > maxAxles)
      continue;

    selectedTrains.push_back(train.officialName);
  }

  return (! selectedTrains.empty());
}


unsigned Database::axleCount(const unsigned trainNo) const
{
  return trainDB.numAxles(trainNo);
}


bool Database::getPerfectPeaks(
  const unsigned trainNo,
  vector<PeakPos>& peaks) const // In m
{
  // if (trainNo >= trainEntries.size())
    // return false;

  PeakPos peak;
  peak.value = 1.f;
  peaks.clear();
  
  vector<double> peakPos;
  trainDB.getPeakPositions(trainNo, peakPos);

  for (auto d: peakPos)
  {
    peak.pos = d;
    peaks.push_back(peak);
  }

  return true;
}


int Database::lookupCarNumber(const string& offName) const
{
  return carDB.lookupNumber(offName);
}


int Database::lookupTrainNumber(const string& offName) const
{
  auto it = offTrainMap.find(offName);
  if (it == offTrainMap.end())
    return -1;

  const int old = static_cast<int>(it->second);
  if (old != trainDB.lookupNumber(offName))
    cout << "TNUM\n";
  return old;
}


string Database::lookupTrainName(const unsigned trainNo) const
{
  if (trainNo >= trainEntries.size())
    return "Bad index";

  const string old = trainEntries[trainNo].officialName;
  if (old != trainDB.lookupName(trainNo))
    cout << "TNAME\n";
  return old;
}


string Database::lookupSensorCountry(const string& sensor) const
{
  auto it = sensors.find(sensor);
  if (it == sensors.end())
    return "Bad sensor name";
  else
    return it->second.country;
}


bool Database::trainIsInCountry(
  const unsigned trainNo,
  const string& country) const
{
  bool n = trainDB.isInCountry(trainNo, country);

  if (trainNo >= trainEntries.size())
  {
    if (n != false)
      cout << "CTR1\n";
    return false;
  }

  for (auto& c: trainEntries[trainNo].countries)
  {
    if (country == c)
    {
      if (n != true)
        cout << "CTR2\n";
      return true;
    }
  }
  if (n != false)
    cout << "CTR3\n";
  return false;
}


bool Database::trainIsReversed(const unsigned trainNo) const
{
  if (trainNo >= trainEntries.size())
    return false;

  const bool old = trainEntries[trainNo].reverseFlag;
  if (old != trainDB.reversed(trainNo))
    cout << "TREV\n";

  return trainEntries[trainNo].reverseFlag;
}

