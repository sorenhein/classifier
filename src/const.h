#ifndef TRAIN_CONST_H
#define TRAIN_CONST_H

#define SAMPLE_RATE 2000 // In Hz

struct PeakPos
{
  int pos; // In mm
  float value;
};

struct PeakSample
{
  int no; // In samples
  float value;
};

#endif
