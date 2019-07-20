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
  if (trainNo >= trainEntries.size())
    return 0;

  return trainEntries[trainNo].axles.size();
}


bool Database::getPerfectPeaks(
  const string& trainName,
  vector<PeakPos>& peaks) const // In m
{
  const int trainNo = Database::lookupTrainNumber(trainName);
  if (trainNo == -1)
    return false;
  else
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

  vector<double> peakPos;
  trainDB.getPeakPositions(trainNo, peakPos);
  /*
  if (peaks.size() != peakPos.size())
  {
    cout << "PPOS size " << peaks.size() << " vs " << peakPos.size() << endl;
  }
  else
  {
    for (unsigned i = 0; i < peakPos.size(); i++)
    {
      if (peakPos[i] == 0.)
      {
        if (peaks[i].pos != 0.)
          cout << i << " PPOS expected zero: " << peaks[i].pos << endl;
      }
      else
      {
        double d = (peakPos[i] - peaks[i].pos) / peakPos[i];
        if (d > 1.e-6)
          cout << i << " PPOS deviation " << d << " between " <<
            peakPos[i] << " and " << peaks[i].pos << endl;
      }
    }
  }
  */

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
    return "Bad sensor name";
  else
    return it->second.country;
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


bool Database::trainIsReversed(const unsigned trainNo) const
{
  if (trainNo >= trainEntries.size())
    return false;

  return trainEntries[trainNo].reverseFlag;
}

