#ifndef TRAIN_PRINT_H
#define TRAIN_PRINT_H

#include <vector>

#include "struct.h"

using namespace std;


void printPeakPosCSV(
  const vector<PeakPos>& peaks,
  const int level);

void printPeakSampleCSV(
  const vector<PeakSample>& peaks,
  const int level);

void printCorrelation(
  const vector<double> actual,
  const vector<double> estimate);

#endif
