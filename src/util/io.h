#ifndef TRAIN_IO_H
#define TRAIN_IO_H

#include <vector>
#include <string>

using namespace std;


bool getFilenames(
  const string& dirName,
  vector<string>& textfiles,
  const string& terminateMatch = "");

bool readBinaryTrace(
  const string& filename,
  vector<float>& samples);

void writeBinary(
  const string& filename,
  const unsigned offset,
  const vector<float>& sequence);

void writeBinaryFloat(
  const string& filename,
  const vector<float>& sequence);

void writeBinaryUnsigned(
  const string& filename,
  const vector<unsigned>& sequence);

#endif
