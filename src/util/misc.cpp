#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "parse.h"
#include "misc.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


void getFilenames(
  const string& dirName,
  vector<string>& textfiles,
  const string& terminateMatch)
{
  if (! std::filesystem::exists(dirName))
  {
    cout << "Bad directory " << dirName << endl;
    return;
  }

  for (const auto& entry: std::filesystem::directory_iterator(dirName))
  {
    const auto& path = entry.path();

    string name = path.string();
    replace(name.begin(), name.end(), '\\', '/');

    string ext = path.extension().string();
    toUpper(ext);
    if (ext != ".TXT" && ext != ".DAT")
      continue;

    if (terminateMatch == "")
      textfiles.push_back(name);
    else if (name.find(terminateMatch) != string::npos)
    {
      textfiles.push_back(name);
      break;
    }
  }
}


bool readBinaryTrace(
  const string& filename,
  vector<float>& samples)
{
  ifstream fin(filename, std::ios::binary);
  fin.unsetf(std::ios::skipws);
  fin.seekg(0, std::ios::end);
  const unsigned filesize = static_cast<unsigned>(fin.tellg());
  fin.seekg(0, std::ios::beg);
  samples.resize(filesize/4);

  fin.read(reinterpret_cast<char *>(samples.data()),
    static_cast<long int>(samples.size() * sizeof(float)));
  fin.close();
  return true;
}



float ratioCappedUnsigned(
  const unsigned num,
  const unsigned denom,
  const float rMax)
{
  return ratioCappedFloat(
    static_cast<float>(num),
    static_cast<float>(denom),
    rMax);
}


float ratioCappedFloat(
  const float num,
  const float denom,
  const float rMax)
{
  if (denom == 0)
    return rMax;

  const float invMax = 1.f / rMax;

  if (num == 0)
    return invMax;

  const float f = num / denom;
  if (f > rMax)
    return rMax;
  else if (f < invMax)
    return invMax;
  else
    return f;
}

