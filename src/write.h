#ifndef TRAIN_WRITE_H
#define TRAIN_WRITE_H

#include <vector>

#include "struct.h"

using namespace std;

void writeBinary(
  const string& origname,
  const string& newdirname,
  const unsigned offset,
  const vector<float>& sequence);

#endif
