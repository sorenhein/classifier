#ifndef TRAIN_READ_H
#define TRAIN_READ_H

using namespace std;

struct Control;
class Database;
class SensorDB;
class TrainDB;
class TraceDB;


bool readControlFile(
  Control& control,
  const string& fname);

void readCarFiles(
  Database& db,
  const string& dir);

void readTrainFiles(
  Database& db,
  const string& dir,
  const string& correctionDir);

void readSensorFile(
  Database& db,
  const string& fname);

bool readTraceTruth(
  const string& fname,
  const SensorDB& sensorDB,
  const TrainDB& trainDB,
  TraceDB& tdb);

#endif
