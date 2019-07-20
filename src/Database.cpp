#include "Database.h"


Database::Database()
{
  carDB.reset();
  trainDB.reset();
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
  return trainDB.selectByAxles(countries, minAxles, maxAxles);
}


bool Database::getPerfectPeaks(
  const unsigned trainNo,
  vector<PeakPos>& peaks) const // In m
{
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


string Database::lookupSensorCountry(const string& sensor) const
{
  auto it = sensors.find(sensor);
  if (it == sensors.end())
    return "Bad sensor name";
  else
    return it->second.country;
}


unsigned Database::axleCount(const unsigned trainNo) const
{
  return trainDB.numAxles(trainNo);
}


int Database::lookupTrainNumber(const string& offName) const
{
  return trainDB.lookupNumber(offName);
}


string Database::lookupTrainName(const unsigned trainNo) const
{
  return trainDB.lookupName(trainNo);
}


bool Database::trainIsInCountry(
  const unsigned trainNo,
  const string& country) const
{
  return trainDB.isInCountry(trainNo, country);
}


bool Database::trainIsReversed(const unsigned trainNo) const
{
  return trainDB.reversed(trainNo);
}

