#ifndef TRAIN_PRINT_H
#define TRAIN_PRINT_H

#include <vector>

#include "Database.h"
#include "struct.h"

using namespace std;


void printPeakPosCSV(
  const vector<PeakPos>& peaks,
  const int level);

void printPeakTimeCSV(
  const vector<PeakTime>& peaks,
  const int level);

void printCorrelation(
  const vector<double> actual,
  const vector<double> estimate);

void printAlignment(
  const Alignment& match,
  const string& trainName);

void printMotion(
  const vector<double>& motionEstimate);

void printMatches(
  const Database& db,
  const vector<Alignment>& matches);

void printTopResiduals(const Alignment& match);

#endif
