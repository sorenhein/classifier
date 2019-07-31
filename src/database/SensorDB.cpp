#include <iostream>

#include "SensorDB.h"


SensorDB::SensorDB()
{
  SensorDB::reset();
}


SensorDB::~SensorDB()
{
}


void SensorDB::reset()
{
  fieldCounts =
  {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
  };
}


bool SensorDB::readFile(const string& fname)
{
  if (! entry.readCommaFile(fname, fieldCounts, SENSOR_STRINGS_SIZE))
  {
    cout << "Could not read sensor file " << fname << endl;
    return false;
  }
  else
    return true;
}


const string& SensorDB::country(const string& name) const
{
  return entry.getMap(name, SENSOR_COUNTRY);
}

