#ifndef TRAIN_TRACEDB_H
#define TRAIN_TRACEDB_H

#include <vector>
#include <map>
#include <atomic>

#include "Entity.h"

using namespace std;

class SensorDB;


enum TraceFieldStrings
{
  TRACE_FILENAME = 0,
  TRACE_DISPLACEMENT_STRING = 1,
  TRACE_TYPE = 2,
  TRACE_AXLES_STRING = 3,
  TRACE_SPEED_STRING = 4, // In km/h
  TRACE_ACCEL_STRING = 5,
  TRACE_REVERSED_STRING = 6,
  TRACE_ACCEL_RMS_STRING = 7,
  TRACE_ACCEL_NORM_STRING = 8,
  TRACE_SATURATED_STRING = 9,
  TRACE_PARTIAL_STRING = 10,
  TRACE_SHIFT_STRING = 11,
  TRACE_ASSET = 12,
  TRACE_SENSOR = 13,
  TRACE_POSITION = 14,
  TRACE_TEMPERATURE_STRING = 15,
  TRACE_VOLTAGE_STRING = 16,
  TRACE_OFFICIAL_NAME = 17,
  TRACE_DATE = 18,
  TRACE_TIME = 19,
  TRACE_COUNTRY = 20,
  TRACE_STRINGS_SIZE = 21
};

enum TraceFieldInts
{
  TRACE_YEAR = 0,
  TRACE_AXLES = 1,
  TRACE_INTS_SIZE = 2
};

enum TraceFieldBools
{
  TRACE_REVERSED = 0,
  TRACE_BOOLS_SIZE = 1
};

enum TraceFieldDoubles
{
  TRACE_SAMPLE_RATE = 0,
  TRACE_DISPLACEMENT = 1,
  TRACE_SPEED = 2, // In m/s
  TRACE_ACCEL = 3,
  TRACE_DOUBLES_SIZE = 4
};

struct TraceData
{
  string filenameFull;
  string filename;
  unsigned traceNoGlobal;
  unsigned traceNoInRun;
  string sensor;
  string time;
  string countrySensor; // Needs sensorDB, so set externally
  string trainTrue;
  unsigned trainNoTrue; // Needs trainDB, so set externally
  double speed; 
  double sampleRate;
  unsigned year;
};


class TraceDB
{
  private:

    vector<unsigned> fieldCounts;

    vector<Entity> entries;

    map<string, unsigned> traceMap;

    vector<string> filenames;

    atomic<int> currTrace;


    bool deriveOrigin(
      Entity& entry,
      const SensorDB& sensorDB) const;

    bool deriveName(Entity& entry) const;

    bool derivePhysics(Entity& entry) const;

    bool complete(
      Entity& entry,
      const SensorDB& sensorDB) const;


  public:

    TraceDB();

    ~TraceDB();

    void reset();

    bool readFile(
      const string& fname,
      const SensorDB& sensorDB);

    vector<string>& getFilenames();

    bool next(TraceData& traceData);
};

#endif
