#ifndef TRAIN_MISC_H
#define TRAIN_MISC_H

#include <vector>

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

void printVectorCSV(
  const string& title,
  const vector<float>& peaks,
  const int level);

#endif
