#include <iostream>
#include <thread>

#include "database/Control.h"
#include "database/TrainDB.h"
#include "database/TraceDB.h"

#include "util/Scheduler.h"

#include "stats/CompStats.h"
#include "stats/PeakStats.h"
#include "stats/Timers.h"

#include "setup.h"
#include "run.h"


// Can also keep them local and pass them to runThread, but in
// Visual Studio, for some reason the compiler does not like it
// if the list of thread arguments becomes too long.

Control control;
TrainDB trainDB;
TraceDB traceDB;

CompStats sensorStats;
CompStats trainStats;
PeakStats peakStats;

Scheduler scheduler;

vector<Timers> timers;


void runThread(unsigned thid);


int main(int argc, char * argv[])
{
  setup(argc, argv, control, trainDB, traceDB);

  timers.resize(control.numThreads());
  timers[0].start(TIMER_ALL_THREADS);

  vector<thread *> threads;
  threads.resize(control.numThreads());
  for (unsigned thid = 0; thid < control.numThreads(); thid++)
    threads[thid] = new thread(&runThread, thid);

  for (unsigned thid = 0; thid < control.numThreads(); thid++)
  {
    threads[thid]->join();
    delete threads[thid];
  }

  timers[0].stop(TIMER_ALL_THREADS);

  sensorStats.write(control.sensorstatsFile(), "Sensor");
  trainStats.write(control.trainstatsFile(), "Train");
  peakStats.write(control.peakstatsFile());

  // Consolidate the thread timers.
  for (unsigned i = 1; i < control.numThreads(); i++)
    timers[0] += timers[i];

  cout << "Number of threads: " << control.numThreads() << endl << endl;
  cout << timers[0].str(2) << endl;
}


void runThread(unsigned thid)
{
  TraceData traceData;
  scheduler.setMax(traceDB.numActive());

  unsigned noInRun;
  while (scheduler.next(noInRun))
  {
    traceDB.getData(noInRun, traceData);
    run(control, trainDB, traceData, thid);
  }
}

