#ifndef TRAIN_MISC_H
#define TRAIN_MISC_H

#include <vector>
#include <string>

using namespace std;


void getFilenames(
  const string& dirName,
  vector<string>& textfiles,
  const string& terminateMatch = "");

bool readBinaryTrace(
  const string& filename,
  vector<float>& samples);

float ratioCappedUnsigned(
  const unsigned num,
  const unsigned denom,
  const float rMax);

float ratioCappedFloat(
  const float num,
  const float denom,
  const float rMax);

void toUpper(string& text);

#endif
