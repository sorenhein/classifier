#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "TraceDB.h"
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
  return true;
}


string TraceDB::deriveName(const TraceTruth& truth)
{
  if (truth.trainName == "ICE4_12")
    return "ICE4_" + 
      to_string(truth.numAxles) + "_" + 
      (truth.reverseFlag ? "_R" : "_N");
  else
    return truth.trainName + "_" + 
      to_string(truth.numAxles) + "_" + 
      (truth.reverseFlag ? "_R" : "_N");
}


bool TraceDB::log(const TraceTruth& truth)
{
  auto it = entries.find(truth.filename);
  if (it != entries.end())
  {
    cout << "File truth for " << truth.filename << " already logged\n";
    return false;
  }
  
  TraceEntry& entry = entries[truth.filename];

  if (! TraceDB::deriveComponents(truth.filename, entry))
  {
    cout << "Could not parse filename " << truth.filename << "\n";
    return false;
  }

  entry.trainTruth.trainName = TraceDB::deriveName(truth);
  entry.trainTruth.numAxles = truth.numAxles;
  entry.trainTruth.speed = truth.speed;
  entry.trainTruth.accel = truth.accel;

  return true;
}


bool TraceDB::log(
  const string& fname,
  const vector<Alignment>& align)
{
  auto it = entries.find(fname);
  if (it == entries.end())
  {
    cout << "File truth for " << fname << " not logged\n";
    return false;
  }

  const unsigned n = (align.size() > 5 ? 5 : align.size());
  for (unsigned i = 0; i < n; i++)
    it->second.align.push_back(align[i]);
  return true;
}


void TraceDB::printCSV(const string& fname) const
{
  ofstream fout;
  fout.open(fname);

  string s = "Date" + string(SEPARATOR) +
    "Time" + SEPARATOR +
    "Sensor" + SEPARATOR +
    "Name" + SEPARATOR +
    "Axles" + SEPARATOR +
    "Speed" + SEPARATOR +
    "Accel";

  for (unsigned i = 0; i < 5; i++)
    s += string(SEPARATOR) + 
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
      to_string(entry.trainTruth.accel);

    for (unsigned i = 0; i < entry.align.size(); i++)
      s += SEPARATOR + 
        to_string(entry.align[i].dist) + SEPARATOR +
        to_string(entry.align[i].numAdd) + SEPARATOR +
        to_string(entry.align[i].numDelete);

    fout << s << "\n";
  }
  fout << "\n";
  fout.close();
}

