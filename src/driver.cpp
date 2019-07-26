#include <iostream>
#include <thread>

#include "database/Control.h"
#include "database/SensorDB.h"
#include "database/TrainDB.h"
#include "database/TraceDB.h"

#include "stats/CompStats.h"
#include "stats/PeakStats.h"
#include "stats/Timers.h"

#include "setup.h"
#include "run.h"

// Needs to be global as it keeps the counter for multi-threading,
// and this needs to change during execution.
TraceDB traceDB;

CompStats sensorStats;
CompStats trainStats;
PeakStats peakStats;
Timers timers;

#define THREADS 1

void runThread(
  const Control& control,
  const SensorDB& sensorDB,
  const TrainDB& trainDB,
  const unsigned thid);


int main(int argc, char * argv[])
{
  Control control;
  SensorDB sensorDB;
  TrainDB trainDB;
  setup(argc, argv, control, sensorDB, trainDB, traceDB);

  vector<thread *> threads;
  threads.resize(THREADS);
  for (unsigned thid = 0; thid < THREADS; thid++)
    threads[thid] = new thread(&runThread, control, sensorDB, 
      trainDB, thid);

  for (unsigned thid = 0; thid < THREADS; thid++)
  {
    threads[thid]->join();
    delete threads[thid];
  }

  sensorStats.write("sensorstats.txt", "Sensor");
  trainStats.write("trainstats.txt", "Train");
  peakStats.write("peakstats.txt");

  cout << timers.str(2) << endl;
}


void runThread(
  const Control& control,
  const SensorDB& sensorDB,
  const TrainDB& trainDB,
  const unsigned thid)
{
  TraceData traceData;
  while (traceDB.next(traceData))
  {
    traceData.countrySensor = sensorDB.country(traceData.sensor);
    traceData.trainNoTrue = trainDB.lookupNumber(traceData.trainTrue);
    run(control, trainDB, traceData, thid);
  }
}

