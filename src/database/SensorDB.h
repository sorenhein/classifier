#ifndef TRAIN_SENSORDB_H
#define TRAIN_SENSORDB_H

#include <map>

#include "Entity.h"

using namespace std;


enum SensorFieldStrings
{
  SENSOR_COUNTRY = 0,
  SENSOR_TYPE = 1,
  SENSOR_STRINGS_SIZE = 2
};


class SensorDB
{
  private:

    vector<unsigned> fieldCounts;

    Entity entry;


  public:

    SensorDB();

    ~SensorDB();

    void reset();

    bool readFile(const string& fname);

    const string& country(const string& name) const;
};

#endif
