#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "TraceDB.h"
#include "struct.h"


TraceDB::TraceDB()
{
}


TraceDB::~TraceDB()
{
}


bool TraceDB::deriveComponents(const string& fname)
{
  // TODO Split on _
  // Use 0, 1, 2
}


string TraceDB::deriveName(const TraceTruth& truth)
{
  // TODO ICE, 48, -1 -> ICE_48_R
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

  if (! TraceDB::deriveComponents(truth.filename))
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
  const TrainData& detected)
{
}


void TraceDB::printCSV(const string& fname) const
{
}

