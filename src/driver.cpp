#include <iostream>

#include "database/Control.h"
#include "database/SensorDB.h"
#include "database/TrainDB.h"
#include "database/TraceDB.h"

#include "stats/CompStats.h"
#include "stats/PeakStats.h"
#include "stats/Timers.h"

#include "setup.h"
#include "run.h"

Timers timers;


int main(int argc, char * argv[])
{
  Control control;
  SensorDB sensorDB;
  TrainDB trainDB;
  TraceDB traceDB;
  setup(argc, argv, control, sensorDB, trainDB, traceDB);

  CompStats sensorStats, trainStats;
  PeakStats peakStats;
  
  TraceData traceData;
  while (traceDB.next(traceData))
  {
    traceData.countrySensor = sensorDB.country(traceData.sensor);
    traceData.trainNoTrue = trainDB.lookupNumber(traceData.trainTrue);

    run(control, trainDB, traceData, sensorStats, trainStats, peakStats);

  }

  sensorStats.write("sensorstats.txt", "Sensor");
  trainStats.write("trainstats.txt", "Train");
  peakStats.write("peakstats.txt");

  cout << timers.str(2) << endl;
}

