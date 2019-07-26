#ifndef TRAIN_RUN_H
#define TRAIN_RUN_H

class Control;
class TrainDB;
struct TraceData;


void run(
  const Control& control,
  const TrainDB& trainDB,
  const TraceData& traceData,
  const unsigned thid);

#endif

