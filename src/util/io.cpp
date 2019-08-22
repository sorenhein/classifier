#include <iostream>
#include <fstream>

#if defined(__CYGWIN__) && (__GNUC__ < 8)
  #include <experimental/filesystem>
  using namespace std::experimental::filesystem;
#else
  #include <filesystem>
  using namespace std::filesystem;
#endif

#include "io.h"

#define OFFSET "_offset_"


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
  if (! exists(dirName))
  {
    cout << "Bad directory " << dirName << endl;
    return false;
  }

  for (const auto& entry: directory_iterator(dirName))
  {
    const auto& path = entry.path();

    string name = path.string();
    string::size_type n = 0;
    while ((n = name.find("\\", n)) != string::npos)
      name.replace(n, 1, "/");

    string ext = path.extension().string();
    toUpper(ext);
    if (ext != ".TXT" && ext != ".DAT")
      continue;

    if (terminateMatch != "")
    {
      if (name.find(terminateMatch) != string::npos)
      {
        // Only the first such file.
        textfiles.push_back(name);
        break;
      }
    }
    else
      textfiles.push_back(name);
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


void writeBinaryFloat(
  const string& filename,
  const vector<float>& sequence)
{
  if (sequence.empty())
    return;

  ofstream fout(filename, std::ios::out | std::ios::binary);
  fout.write(reinterpret_cast<const char *>(sequence.data()),
    static_cast<long int>(sequence.size() * sizeof(float)));
  fout.close();
}


void writeBinaryUnsigned(
  const string& filename,
  const vector<unsigned>& sequence)
{
  if (sequence.empty())
    return;

  ofstream fout(filename, std::ios::out | std::ios::binary);
  fout.write(reinterpret_cast<const char *>(sequence.data()),
    static_cast<long int>(sequence.size() * sizeof(unsigned)));
  fout.close();
}


void writeString(
  const string& filename,
  const string& content)
{
  if (content.empty())
    return;

  ofstream fout(filename);
  fout << content;
  fout.close();
}

