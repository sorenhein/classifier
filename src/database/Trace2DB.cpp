#include <iostream>
#include <fstream>
#include <cassert>

#include "Trace2DB.h"
#include "SensorDB.h"
#include "parse.h"


Trace2DB::Trace2DB()
{
  Trace2DB::reset();
}


Trace2DB::~Trace2DB()
{
}


void Trace2DB::reset()
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
}


bool Trace2DB::deriveOrigin(
  Entity& entry,
  const SensorDB& sensorDB) const
{
  const string& fname = entry.getString(TRACE_FILENAME);

  const size_t c = countDelimiters(fname, "_");
  if (c != 4)
  {
    cout << "Can't parse trace origin " << fname << endl;
    return false;
  }

  vector<string> v;
  v.clear();
  tokenize(fname, v, "_");

  entry.setString(TRACE_DATE, v[0]);
  entry.setString(TRACE_TIME, v[1]);
  entry.setString(TRACE_COUNTRY, sensorDB.country(v[2]));
  return true;

}


bool Trace2DB::deriveName(Entity& entry) const
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


bool Trace2DB::derivePhysics(Entity& entry) const
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
    entry.setDouble(TRACE_SPEED, d / 3.6);

  if (! parseDouble(entry.getString(TRACE_ACCEL_STRING), 
    entry.getDouble(TRACE_ACCEL)))
  {
    cout << "Could not parse acceleration" << endl;
    return false;
  }

  return true;
}


bool Trace2DB::complete(
  Entity& entry,
  const SensorDB& sensorDB) const
{
  if (! Trace2DB::deriveOrigin(entry, sensorDB))
    return false;

  if (! Trace2DB::deriveName(entry))
    return false;

  if (! Trace2DB::derivePhysics(entry))
    return false;

  return true;
}


bool Trace2DB::readFile(
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

    if (! Trace2DB::complete(entry, sensorDB))
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


unsigned Trace2DB::traceNumber(const string& fname) const
{
  const string basename = parseBasename(fname);
  auto it = traceMap.find(basename);
  assert(it != traceMap.end());

  return it->second;
}


const string& Trace2DB::sensor(const unsigned traceNo) const
{
  assert(traceNo < entries.size());
  return entries[traceNo].getString(TRACE_SENSOR);
}


const string& Trace2DB::time(const unsigned traceNo) const
{
  assert(traceNo < entries.size());
  return entries[traceNo].getString(TRACE_TIME);
}


const string& Trace2DB::train(const unsigned traceNo) const
{
  assert(traceNo < entries.size());
  return entries[traceNo].getString(TRACE_OFFICIAL_NAME);
}


double Trace2DB::speed(const unsigned traceNo) const
{
  assert(traceNo < entries.size());
  return entries[traceNo].getDouble(TRACE_SPEED);
}

