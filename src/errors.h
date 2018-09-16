#ifndef TRAIN_ERRORS_H
#define TRAIN_ERRORS_H

using namespace std;

enum Code
{
  ERR_REGRESS = 0,
  ERR_SURVIVORS = 1,
  ERR_SHORT_ACCEL_TRACE = 2,
  ERR_NO_PEAKS = 3,
  ERR_PEAK_COLLAPSE = 4,
  ERR_ALGO_PEAKSTATS = 5,
  ERR_ALGO_PEAKSCORES = 6
};


#endif

