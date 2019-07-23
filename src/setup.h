#ifndef TRAIN_SETUP_H
#define TRAIN_SETUP_H

class Control;
class SensorDB;
class TrainDB;
class TraceDB;

using namespace std;


void setup(
  int argc,
  char * argv[],
  Control& control,
  SensorDB& sensorDB,
  TrainDB& trainDB,
  TraceDB& traceDB);

#endif
