#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "TraceDB.h"
#include "Database.h"
#include "Align.h"
#include "read.h"
#include "struct.h"

#define SEPARATOR ";"


TraceDB::TraceDB()
{
}


TraceDB::~TraceDB()
{
}


bool TraceDB::deriveComponents(
  const string& fname,
  const Database& db,
  TraceEntry& entry)
{
  const size_t c = countDelimiters(fname, "_");
  if (c != 4)
  {
    cout << "Can't parse filename " << fname << endl;
    return false;
  }

  vector<string> v;
  v.clear();
  tokenize(fname, v, "_");

  entry.date = v[0]; 
  entry.time = v[1]; 
  entry.sensor = v[2]; 
  entry.country = db.lookupSensorCountry(v[2]);
  return true;
}


string TraceDB::deriveName(
  const string& country,
  const TraceTruth& truth)
{
  if (truth.trainName == "ICE4_12")
    return "ICE4_" + 
      country + "_" +
      to_string(truth.numAxles) + "_" + 
      (truth.reverseFlag ? "R" : "N");
  if (truth.trainName == "BOB")
    return "MERIDIAN_" + 
      country + "_" +
      to_string(truth.numAxles) + "_" + 
      (truth.reverseFlag ? "R" : "N");
  else
    return truth.trainName + "_" + 
      country + "_" +
      to_string(truth.numAxles) + "_" + 
      (truth.reverseFlag ? "R" : "N");
}


bool TraceDB::log(
  const TraceTruth& truth,
  const Database& db)
{
  auto it = entries.find(truth.filename);
  if (it != entries.end())
  {
    cout << "File truth for " << truth.filename << " already logged\n";
    return false;
  }
  
  TraceEntry& entry = entries[truth.filename];

  if (! TraceDB::deriveComponents(truth.filename, db, entry))
  {
    cout << "Could not parse filename " << truth.filename << "\n";
    return false;
  }

  entry.trainTruth.trainName = 
    TraceDB::deriveName(entry.country, truth);
  entry.trainTruth.numAxles = truth.numAxles;
  entry.trainTruth.speed = truth.speed;
  entry.trainTruth.accel = truth.accel;

  if (db.lookupTrainNumber(entry.trainTruth.trainName) == -1)
  {
    cout << "Train " << entry.trainTruth.trainName <<
      " does not exist\n";
  }

  return true;
}


bool TraceDB::log(
  const string& fname,
  const vector<Alignment>& align,
  const unsigned peakCount)
{
  const string basename = TraceDB::basename(fname);
  auto it = entries.find(basename);
  if (it == entries.end())
  {
    cout << "File truth for " << basename << " not logged\n";
    return false;
  }

  it->second.numPeaksDetected = peakCount;

  const unsigned n = (align.size() > 5 ? 5 : align.size());
  for (unsigned i = 0; i < n; i++)
    it->second.align.push_back(align[i]);
  return true;
}


string TraceDB::basename(const string& fname) const
{
  if (fname == "")
    return "";

  const auto p = fname.find_last_of('/');
  if (p == string::npos || p == fname.size()-1)
   return fname;

  return fname.substr(p+1);
}


string TraceDB::lookupSensor(const string& fname) const
{
  const string basename = TraceDB::basename(fname);
  auto it = entries.find(basename);
  if (it == entries.end())
  {
    cout << "Sensor for " << basename << " not logged\n";
    return false;
  }

  return it->second.sensor;
}


void TraceDB::printCSV(
  const string& fname,
  const Database& db) const
{
  ofstream fout;
  fout.open(fname);

  string s = "Date" + string(SEPARATOR) +
    "Time" + SEPARATOR +
    "Sensor" + SEPARATOR +
    "Name" + SEPARATOR +
    "Axles" + SEPARATOR +
    "Speed" + SEPARATOR +
    "Accel" + SEPARATOR +
    "Peaks";

  for (unsigned i = 0; i < 5; i++)
    s += string(SEPARATOR) + 
      "Name" + SEPARATOR +
      "Dist" + SEPARATOR +
      "Add" + SEPARATOR +
      "Del";

  fout << s << "\n";

  for (auto& it: entries)
  {
    const TraceEntry& entry = it.second;

    s = entry.date + SEPARATOR +
      entry.time + SEPARATOR + 
      entry.sensor + SEPARATOR + 
      entry.trainTruth.trainName + SEPARATOR + 
      to_string(entry.trainTruth.numAxles) + SEPARATOR + 
      to_string(entry.trainTruth.speed) + SEPARATOR + 
      to_string(entry.trainTruth.accel) + SEPARATOR +
      to_string(entry.numPeaksDetected);

    for (unsigned i = 0; i < entry.align.size(); i++)
      s += SEPARATOR + 
        db.lookupTrainName(entry.align[i].trainNo) + SEPARATOR +
        to_string(entry.align[i].distMatch) + SEPARATOR +
        to_string(entry.align[i].numAdd) + SEPARATOR +
        to_string(entry.align[i].numDelete);

    fout << s << "\n";
  }
  fout << "\n";
  fout.close();
}

