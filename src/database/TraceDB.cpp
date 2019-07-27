#include <iostream>
#include <fstream>
#include <cassert>

#include "TraceDB.h"
#include "SensorDB.h"
#include "parse.h"
#include "../const.h"

#define YEAR_DEFAULT 2018
#define SAMPLE_RATE_DEFAULT 2000.


TraceDB::TraceDB()
{
  TraceDB::reset();
}


TraceDB::~TraceDB()
{
}


void TraceDB::reset()
{
  traceMap.clear();

  fieldCounts =
  {
    TRACE_STRINGS_SIZE,
    0,
    0,
    0,
    TRACE_INTS_SIZE,
    TRACE_BOOLS_SIZE,
    TRACE_DOUBLES_SIZE
  };

  currTrace = -1;
}


bool TraceDB::deriveOrigin(
  Entity& entry,
  const SensorDB& sensorDB) const
{
  const string& fname = entry.getString(TRACE_FILENAME);

  vector<string> v;
  parseDelimitedString(fname, "_", v);
  if (v.size() != 5)
  {
    cout << "Can't parse trace origin " << fname << endl;
    return false;
  }

  entry.setString(TRACE_DATE, v[0]);
  entry.setString(TRACE_TIME, v[1]);
  entry.setString(TRACE_COUNTRY, sensorDB.country(v[2]));
  return true;

}


bool TraceDB::deriveName(Entity& entry) const
{
  if (! parseInt(entry.getString(TRACE_AXLES_STRING), entry[TRACE_AXLES]))
  {
    cout << "Could not parse axles" << endl;
    return false;
  }

  string s = entry.getString(TRACE_REVERSED_STRING);
  if (s == "1")
    entry.setBool(TRACE_REVERSED, false);
  else if (s == "-1")
    entry.setBool(TRACE_REVERSED, true);
  else
  {
    cout << "Could not parse reverse indicator" << endl;
    return false;
  }

  const string& trainType = entry.getString(TRACE_TYPE);
  string head;
  if (trainType == "ICE4_12")
    head = "ICE4";
  else if (trainType == "BOB")
    head = "MERIDIAN";
  else
    head = trainType;

  const string tail = 
    entry.getString(TRACE_COUNTRY) + "_" +
    entry.getString(TRACE_AXLES_STRING) + "_" +
    (entry.getBool(TRACE_REVERSED) ? "R" : "N");
    
  entry.setString(TRACE_OFFICIAL_NAME, head + "_" + tail);
  return true;
}


bool TraceDB::derivePhysics(Entity& entry) const
{
  if (! parseDouble(entry.getString(TRACE_DISPLACEMENT_STRING),
    entry.getDouble(TRACE_DISPLACEMENT)))
  {
    cout << "Could not parse displacement" << endl;
    return false;
  }

  double d;
  if (! parseDouble(entry.getString(TRACE_SPEED_STRING), d))
  {
    cout << "Could not parse speed" << endl;
    return false;
  }
  else
    entry.setDouble(TRACE_SPEED, d / MS_TO_KMH);

  if (! parseDouble(entry.getString(TRACE_ACCEL_STRING), 
    entry.getDouble(TRACE_ACCEL)))
  {
    cout << "Could not parse acceleration" << endl;
    return false;
  }

  return true;
}


bool TraceDB::complete(
  Entity& entry,
  const SensorDB& sensorDB) const
{
  if (! TraceDB::deriveOrigin(entry, sensorDB))
    return false;

  if (! TraceDB::deriveName(entry))
    return false;

  if (! TraceDB::derivePhysics(entry))
    return false;

  // Set some defaults.
  entry[TRACE_YEAR] = YEAR_DEFAULT;
  entry.setDouble(TRACE_SAMPLE_RATE, SAMPLE_RATE_DEFAULT);

  return true;
}


bool TraceDB::readFile(
  const string& fname,
  const SensorDB& sensorDB)
{
  Entity entry;
  entry.init(fieldCounts);

  bool errFlag;
  ifstream fin;

  fin.open(fname);
  if (! entry.readCommaLine(fin, errFlag, TRACE_VOLTAGE_STRING+1))
  {
    cout << "Could not skip first line of truth file " << fname << endl;
    fin.close();
    return false;
}

  while (entry.readCommaLine(fin, errFlag, TRACE_VOLTAGE_STRING+1))
  {
    traceMap[entry.getString(TRACE_FILENAME)] = entries.size();

    if (! TraceDB::complete(entry, sensorDB))
    {
      cout << "Could not complete trace truth file " << fname << endl;
      fin.close();
      return false;
    }

    entries.push_back(entry);
  }
  fin.close();

  if (errFlag)
  {
    cout << "Could not read trace truth file " << fname << endl;
    return false;
  }
  else
    return true;
}


vector<string>& TraceDB::getFilenames()
{
  return filenames;
}


bool TraceDB::next(TraceData& traceData)
{
  // Atomic.
  int n = ++currTrace;
  if (static_cast<unsigned>(n) >= filenames.size())
    return false;

  const string basename = parseBasename(filenames[n]);
  auto it = traceMap.find(basename);
  assert(it != traceMap.end());
  const unsigned ng = it->second;

  auto& entry = entries[ng];

  traceData.filenameFull = filenames[n];
  traceData.filename = basename;
  traceData.traceNoGlobal = ng;
  traceData.traceNoInRun = n;
  traceData.sensor = entry.getString(TRACE_SENSOR);
  traceData.time = entry.getString(TRACE_TIME);
  traceData.trainTrue = entry.getString(TRACE_OFFICIAL_NAME);
  traceData.speed = entry.getDouble(TRACE_SPEED);
  traceData.sampleRate = entry.getDouble(TRACE_SAMPLE_RATE);
  traceData.year = entry.getInt(TRACE_YEAR);
  return true;
}

