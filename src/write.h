#ifndef TRAIN_WRITE_H
#define TRAIN_WRITE_H

#include <vector>
#include <string>

using namespace std;


void writeBinary(
  const string& filename,
  const unsigned offset,
  const vector<float>& sequence);

#endif
