#ifndef TRAIN_READ_H
#define TRAIN_READ_H

using namespace std;

struct Control;
class SensorDB;
class TrainDB;
class TraceDB;


bool readControlFile(
  Control& control,
  const string& fname);

bool readTraceTruth(
  const string& fname,
  const SensorDB& sensorDB,
  const TrainDB& trainDB,
  TraceDB& tdb);

#endif
