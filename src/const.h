#ifndef TRAIN_CONST_H
#define TRAIN_CONST_H

#define SAMPLE_RATE 2000 // In Hz

struct PeakPos
{
  double pos; // In m
  double value;
};

struct PeakSample
{
  int no; // In samples
  double value;
};

#endif
