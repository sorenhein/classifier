#ifndef TRAIN_ERRORS_H
#define TRAIN_ERRORS_H

using namespace std;

enum Code
{
  ERR_REGRESS = 0,
  ERR_SURVIVORS = 1,
  ERR_SHORT_ACCEL_TRACE = 2,
  ERR_NO_PEAKS = 3,
  ERR_ALGO_PEAK_COLLAPSE = 4,
  ERR_ALGO_PEAKSTATS = 5,
  ERR_ALGO_MANY_FRONT_PEAKS = 6,
  ERR_ALGO_MEASURE = 7,
  ERR_SENSOR_NOT_LOGGED = 8,
  ERR_ALGO_PEAK_INTERVALS = 9,
  ERR_CAR_SIDE_GAP_RESET = 10,
  ERR_MODELS_EMPTY = 11,
  ERR_ALGO_PEAK_MATCH = 12,
  ERR_ALGO_PEAK_CONSISTENCY = 13,
  ERR_ALGO_PEAK_PARAMETER = 14,
  ERR_ALGO_PEAK_STRUCTURE = 15
};


#endif

