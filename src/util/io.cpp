#include <iostream>
#include <fstream>
#include <filesystem>

#include "io.h"


void toUpper(string& text);


void toUpper(string& text)
{
  for (unsigned i = 0; i < text.size(); i++)
    text.at(i) = static_cast<char>(toupper(static_cast<int>(text.at(i))));
}


bool getFilenames(
  const string& dirName,
  vector<string>& textfiles,
  const string& terminateMatch)
{
  if (! std::filesystem::exists(dirName))
  {
    cout << "Bad directory " << dirName << endl;
    return false;
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
  return true;
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


