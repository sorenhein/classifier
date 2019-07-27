#include <fstream>

#include "write.h"

#define OFFSET "_offset_"


void writeBinary(
  const string& filename,
  const unsigned offset,
  const vector<float>& sequence)
{
  if (sequence.empty())
    return;

  string name = filename;
  auto p = name.find_last_of('.');
  if (p == string::npos)
    return;

  name.insert(p, OFFSET + to_string(offset));

  ofstream fout(name, std::ios::out | std::ios::binary);
  fout.write(reinterpret_cast<const char *>(sequence.data()),
    static_cast<long int>(sequence.size() * sizeof(float)));
  fout.close();
}

