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

CompStats sensorStats;
CompStats trainStats;
PeakStats peakStats;
Timers timers;


int main(int argc, char * argv[])
{
  Control control;
  SensorDB sensorDB;
  TrainDB trainDB;
  TraceDB traceDB;
  setup(argc, argv, control, sensorDB, trainDB, traceDB);

  TraceData traceData;
  while (traceDB.next(traceData))
  {
    traceData.countrySensor = sensorDB.country(traceData.sensor);
    traceData.trainNoTrue = trainDB.lookupNumber(traceData.trainTrue);

    const unsigned thid = 0;
    run(control, trainDB, traceData, thid);

  }

  sensorStats.write("sensorstats.txt", "Sensor");
  trainStats.write("trainstats.txt", "Train");
  peakStats.write("peakstats.txt");

  cout << timers.str(2) << endl;
}

