#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "write.h"

#define ORIG_DIR "/raw/"
#define EXTENSION ".dat"
#define OFFSET "_offset_"


void writeBinary(
  const string& origname,
  const string& newdirname,
  const unsigned offset,
  const vector<float>& sequence)
{
  if (sequence.size() == 0)
    return;

  string name = origname;
  auto p1 = name.find(ORIG_DIR);
  if (p1 == string::npos)
    return;

  auto p2 = name.find(EXTENSION);
  if (p1 == string::npos)
    return;

  name.insert(p2, OFFSET + to_string(offset));
  name.replace(p1, string(ORIG_DIR).size(), "/" + newdirname + "/");

cout << "BINWROTE " << name << endl;

  ofstream fout(name, std::ios::out | std::ios::binary);
  fout.write(reinterpret_cast<const char *>(sequence.data()),
    static_cast<long int>(sequence.size() * sizeof(float)));
  fout.close();
}

