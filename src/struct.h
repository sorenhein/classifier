#ifndef TRAIN_STRUCT_H
#define TRAIN_STRUCT_H

struct PeakPos
{
  double pos; // In m
  double value;
};

struct PeakSample
{
  double time; // In s
  double value;
};

#endif
