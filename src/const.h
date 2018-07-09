#ifndef TRAIN_CONST_H
#define TRAIN_CONST_H

#define SAMPLE_RATE 2000 // In Hz

struct PeakPos
{
  int pos; // In mm
  double value;
};

struct PeakSample
{
  int no; // In samples
  double value;
};

#endif
