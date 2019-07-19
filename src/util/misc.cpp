#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <filesystem>

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


void tokenize(
  const string& text,
  vector<string>& tokens,
  const string& delimiters)
{
  // tokenize splits a string into tokens separated by delimiter.
  // http://stackoverflow.com/questions/236129/split-a-string-in-c

  string::size_type pos, lastPos = 0;

  while (true)
  {
    pos = text.find_first_of(delimiters, lastPos);
    if (pos == std::string::npos)
    {
      pos = text.length();
      tokens.push_back(string(text.data()+lastPos,
        static_cast<string::size_type>(pos - lastPos)));
      break;
    }
    else
    {
      tokens.push_back(string(text.data()+lastPos,
        static_cast<string::size_type>(pos - lastPos)));
    }
    lastPos = pos + 1;
  }
}


unsigned countDelimiters(
  const string& text,
  const string& delimiters)
{
  int c = 0;
  for (unsigned i = 0; i < delimiters.length(); i++)
    c += static_cast<int>
      (count(text.begin(), text.end(), delimiters.at(i)));
  return static_cast<unsigned>(c);
}


bool parseInt(
  const string& text,
  int& value,
  const string& err)
{
  if (text == "")
    return false;

  int i;
  size_t pos;
  try
  {
    i = stoi(text, &pos);
    if (pos != text.size())
    {
      cout << err << endl;
      return false;
    }
  }
  catch (const invalid_argument& ia)
  {
    UNUSED(ia);
    cout << err << endl;
    return false;
  }
  catch (const out_of_range& ia)
  {
    UNUSED(ia);
    cout << err << endl;
    return false;
  }

  value = i;
  return true;
}


bool parseDouble(
  const string& text,
  double& value,
  const string& err)
{
  if (text == "")
    return false;

  double f;
  size_t pos;
  try
  {
    f = stod(text, &pos);
    if (pos != text.size())
    {
      cout << err << endl;
      return false;
    }
  }
  catch (const invalid_argument& ia)
  {
    UNUSED(ia);
    cout << err << endl;
    return false;
  }
  catch (const out_of_range& ia)
  {
    UNUSED(ia);
    cout << err << endl;
    return false;
  }

  value = f;
  return true;
}


bool parseBool(
  const string& text,
  bool& value,
  const string& err)
{
  if (text == "yes")
    value = true;
  else if (text == "no")
    value = false;
  else
  {
    cout << err << endl;
    return false;
  }

  return true;
}


void parseCommaString(
  const string& text,
  vector<string>& fields)
{
  const size_t c = countDelimiters(text, ",");
  fields.resize(c+1);
  fields.clear();
  tokenize(text, fields, ",");
}


bool parseCarSpecifier(
  const string& text,
  const string& err,
  string& offName,
  bool& reverseFlag,
  unsigned& count)
{
  count = 1;
  offName = text;
  reverseFlag = false;

  if (offName.back() == ')')
  {
    auto pos = offName.find("(");
    if (pos == string::npos)
      return false;

    string bracketed = offName.substr(pos+1);
    bracketed.pop_back();

    int countI;
    if (! parseInt(bracketed, countI, err))
      return false;

    count = static_cast<unsigned>(countI);
    offName = offName.substr(0, pos);
  }

  if (offName.back() == ']')
  {
    // Car is "backwards".
    auto pos = offName.find("[");
    if (pos == string::npos)
      return false;

    string bracketed = offName.substr(pos+1);
    if (bracketed != "R]")
      return false;

    offName = offName.substr(0, pos);
    reverseFlag =  true;
  }
  return true;
}


void toUpper(string& text)
{
  for (unsigned i = 0; i < text.size(); i++)
    text.at(i) = static_cast<char>(toupper(static_cast<int>(text.at(i))));
}


bool readTextTrace(
  const string& filename,
  vector<double>& samples)
{
  ifstream fin;
  fin.open(filename);
  string line;
  double v;
  while (getline(fin, line))
  {
    line.erase(remove(line.begin(), line.end(), '\r'), line.end());
    if (line == "" || line.front() == '#')
      continue;

    // The format seems to have a trailing comma.
    if (line.back() == ',')
      line.pop_back();

    const string err = "File " + filename +
      ": Bad line '" + line + "'";
    if (! parseDouble(line, v, err))
    {
      fin.close();
      return false;
    }
    samples.push_back(v);
  }

  fin.close();
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
