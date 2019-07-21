#ifndef TRAIN_READ_H
#define TRAIN_READ_H

using namespace std;

struct Control;
class SensorDB;
class TrainDB;
class TraceDB;
class Trace2DB;


bool readControlFile(
  Control& control,
  const string& fname);

bool readTraceTruth(
  const string& fname,
  const SensorDB& sensorDB,
  const TrainDB& trainDB,
  TraceDB& tdb,
  Trace2DB& tdb2);

#endif
