#include "Database.h"


Database::Database()
{
  carDB.reset();
  trainDB.reset();
}


Database::~Database()
{
}


// CAR

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


int Database::lookupCarNumber(const string& offName) const
{
  return carDB.lookupNumber(offName);
}


// CORRECTIONS

void Database::readCorrectionFile(const string& fname)
{
  correctionDB.readFile(fname);
}


// SENSOR

void Database::readSensorFile(const string& fname)
{
  sensorDB.readFile(fname);
}


string Database::lookupSensorCountry(const string& sensor) const
{
  return sensorDB.country(sensor);
}


bool Database::getPerfectPeaks(
  const unsigned trainNo,
  vector<double>& peaks) const // In m
{
  trainDB.getPeakPositions(trainNo, peaks);
  return true;
}


// TRAIN

void Database::readTrainFile(const string& fname)
{
  trainDB.readFile(carDB, correctionDB, fname);
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


bool Database::select(
  const list<string>& countries,
  const unsigned minAxles,
  const unsigned maxAxles)
{
  return trainDB.selectByAxles(countries, minAxles, maxAxles);
}


