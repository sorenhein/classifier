#ifndef TRAIN_GEOMETRY_H
#define TRAIN_GEOMETRY_H

#include <vector>
#include <string>

#include "struct.h"

using namespace std;

class Database;


bool nameMatch(
  const string& fullString,
  const string& searchString);

void dumpResiduals(
  const vector<PeakTime>& times,
  const Database& db,
  const unsigned order,
  const vector<Alignment>& matches,
  const string& heading,
  const string& trainTrue,
  const string& trainSelect,
  const unsigned numWheels);

#endif
