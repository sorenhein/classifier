#ifndef TRAIN_PRINT_H
#define TRAIN_PRINT_H

#include <vector>

#include "database/TrainDB.h"

#include "struct.h"

using namespace std;

struct Alignment;


void printPeakPosCSV(
  const vector<double>& peaks,
  const int level);

void printPeakTimeCSV(
  const vector<PeakTime>& peaks,
  const int level);

void printMatches(const vector<Alignment>& matches);

#endif
