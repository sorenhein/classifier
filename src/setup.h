#ifndef TRAIN_SETUP_H
#define TRAIN_SETUP_H

class Control;
class TrainDB;
class TraceDB;

using namespace std;


void setup(
  int argc,
  char * argv[],
  Control& control,
  TrainDB& trainDB,
  TraceDB& traceDB);

void setupStats();

#endif
