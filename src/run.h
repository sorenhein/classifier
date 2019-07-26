#ifndef TRAIN_RUN_H
#define TRAIN_RUN_H

using namespace std;

class Control;
class TrainDB;
class CompStats;
class PeakStats;
struct TraceData;


void run(
  const Control& control,
  const TrainDB& trainDB,
  const TraceData& traceData,
  CompStats& sensorStats,
  CompStats& trainStats,
  PeakStats& peakStats);

#endif

