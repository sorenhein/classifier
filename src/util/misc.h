#ifndef TRAIN_MISC_H
#define TRAIN_MISC_H

#include <vector>

#include "../struct.h"

using namespace std;


float ratioCappedUnsigned(
  const unsigned num,
  const unsigned denom,
  const float rMax);

float ratioCappedFloat(
  const float num,
  const float denom,
  const float rMax);

string percent(
  const unsigned num,
  const unsigned denom,
  const unsigned decimals);

void printPeakPosCSV(
  const vector<double>& peaks,
  const int level);

void printPeakTimeCSV(
  const vector<PeakTime>& peaks,
  const int level);

#endif
