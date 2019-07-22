#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cassert>

#include "Entity.h"
#include "parse.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


Entity::Entity()
{
  Entity::clear();
}


Entity::~Entity()
{
}


void Entity::clear()
{
  strings.clear();
  stringVectors.clear();
  stringMap.clear();
  intVectors.clear();
  ints.clear();
  bools.clear();
}


void Entity::init(const vector<unsigned>& fieldCounts)
{
  strings.resize(fieldCounts[CORRESPONDENCE_STRING]);
  stringVectors.resize(fieldCounts[CORRESPONDENCE_STRING_VECTOR]);
  intVectors.resize(fieldCounts[CORRESPONDENCE_INT_VECTOR]);
  ints.resize(fieldCounts[CORRESPONDENCE_INT], -1);
  bools.resize(fieldCounts[CORRESPONDENCE_BOOL]);
  doubles.resize(fieldCounts[CORRESPONDENCE_DOUBLE]);
}


bool Entity::setCommandLineDefaults(
  const list<CommandLineEntry>& arguments)
{
  for (auto& arg: arguments)
  {
    if (! Entity::parseValue(arg.corrType, arg.defaultValue, arg.no, true))
      return false;
  }
  return true;
}


bool Entity::parseValue(
  const CorrespondenceType corrType,
  const string& value,
  const unsigned no,
  const bool boolExplicitFlag)
{
  if (corrType == CORRESPONDENCE_STRING)
  {
    strings[no] = value;
  }
  else if (corrType == CORRESPONDENCE_STRING_VECTOR)
  {
    parseDelimitedString(value, ",", stringVectors[no]);
  }
  else if (corrType == CORRESPONDENCE_INT)
  {
    if (! parseInt(value, ints[no]))
    {
      cout << "Bad integer" << endl;
      return false;
    }
  }
  else if (corrType == CORRESPONDENCE_BOOL)
  {
    if (boolExplicitFlag)
    {
      if (! parseBool(value, bools[no]))
      {
        cout << "Bad boolean" << endl;
        return false;
      }
    }
    else
    {
      // Opposity of default value.
      bools[no] = ! bools[no];
    }
  }
  else
  {
    cout << "Bad correspondence type " << endl;
    return false;
  }

  return true;
}


bool Entity::parseField(
  const list<CorrespondenceEntry>& fields,
  const string& tag,
  const string& value)
{
  for (const auto& field: fields)
  {
    if (field.tag == tag)
      return Entity::parseValue(field.corrType, value, field.no, true);
  }
  return false;
}


bool Entity::readTagFile(
  const string& fname,
  const list<CorrespondenceEntry>& fields,
  const vector<unsigned>& fieldCounts)
{
  Entity::init(fieldCounts);
  
  ifstream fin;
  fin.open(fname);
  string line;
  while (getline(fin, line))
  {
    line.erase(remove(line.begin(), line.end(), '\r'), line.end());
    if (line == "" || line.front() == '#')
      continue;

    const string err = "File " + fname + ": Bad line '" + line + "'";

    const auto sp = line.find(" ");
    if (sp == string::npos || sp == 0 || sp == line.size()-1)
    {
      cout << err << endl;
      break;
    }

    const string& tag = line.substr(0, sp);
    const string& value = line.substr(sp+1);

    if (! Entity::parseField(fields, tag, value))
    {
      cout << err << endl;
      fin.close();
      return false;
      break;
    }
  }
  fin.close();
  return true;
}


bool Entity::readSeriesFile(
  const string& fname,
  const list<CorrespondenceEntry>& fields,
  const vector<unsigned>& fieldCounts,
  const unsigned no)
{
  Entity::init(fieldCounts);
  
  ifstream fin;
  fin.open(fname);
  string line;

  getline(fin, line);
  line.erase(remove(line.begin(), line.end(), '\r'), line.end());

  if (line.empty())
  {
    fin.close();
    cout << "Error reading correction file: '" << line << "'" << endl;
    return false;
  }

  // Read the single header tag.
  const auto sp = line.find(" ");
  const string& tag = line.substr(0, sp);
  const string& value = line.substr(sp+1);

  if (! Entity::parseField(fields, tag, value))
  {
    cout << "Error reading correction file: '" << line << "'" << endl;
    fin.close();
    return false;
  }

  int number;
  while (getline(fin, line))
  {
    line.erase(remove(line.begin(), line.end(), '\r'), line.end());
    if (line.empty())
      break;

    if (! parseInt(line, number))
    {
      cout << "Bad integer" << endl;
      break;
    }

    intVectors[no].push_back(number);
  }
  fin.close();
  return true;
}


bool Entity::readCommaFile(
  const string& fname,
  const vector<unsigned>& fieldCounts,
  const unsigned count)
{
  Entity::init(fieldCounts);
  
  ifstream fin;
  fin.open(fname);
  string line;
  vector<string> v;

  while (getline(fin, line))
  {
    line.erase(remove(line.begin(), line.end(), '\r'), line.end());
    if (line == "" || line.front() == '#')
      continue;

    const string err = "File " + fname + ": Bad line '" + line + "'";

    parseDelimitedString(line, ",", v);
    if (v.size() != count+1)
    {
      cout << err << endl;
      fin.close();
      return false;
    }

    auto it = stringMap.find(v[0]);
    if (it != stringMap.end())
      return false;

    vector<string>& sm = stringMap[v[0]];
    for (unsigned i = 1; i < count; i++)
      sm.push_back(v[i]);
  }
  fin.close();
  return true;
}


bool Entity::readCommaLine(
  ifstream& fin,
  bool& errFlag,
  const unsigned count)
{
  string line;
  vector<string> v;

  while (getline(fin, line))
  {
    line.erase(remove(line.begin(), line.end(), '\r'), line.end());
    if (line == "" || line.front() == '#')
      continue;

    parseDelimitedString(line, ",", v);
    if (v.size() != count)
    {
      errFlag = true;
      return false;
    }

    for (unsigned i = 0; i < count; i++)
      strings[i] = v[i];

    return true;
  }

  errFlag = false;
  return false;
}


bool Entity::parseCommandLine(
  const vector<string>& commandLine,
  const list<CommandLineEntry>& arguments)
{
  if (! Entity::setCommandLineDefaults(arguments))
    return false;

  unsigned i = 0;
  while (i < commandLine.size())
  {
    const string& tag = commandLine[i];
    bool foundFlag = false;
    for (auto& arg: arguments)
    {
      if (arg.singleDash == tag || arg.doubleDash == tag)
      {
        string v;
        if (arg.corrType == CORRESPONDENCE_BOOL)
          v = "";
        else if (i+1 < commandLine.size())
          v = commandLine[i+1];
        else
        {
          cout << "End of argument line error\n";
          return false;
        }

        if (Entity::parseValue(arg.corrType, v, arg.no, false))
        {
          foundFlag = true;
          break;
        }
        else
          return false;
      }

      if (foundFlag)
      {
        if (arg.corrType == CORRESPONDENCE_BOOL)
          i++;
        else
          i += 2;
      }
    }
  }

  return true;
}


int& Entity::operator [](const unsigned no)
{
  assert(no < ints.size());
  return ints[no];
}


const int& Entity::operator [](const unsigned no) const
{
  assert(no < ints.size());
  return ints[no];
}


string& Entity::getString(const unsigned no)
{
  assert(no < strings.size());
  return strings[no];
}


const string& Entity::getString(const unsigned no) const
{
  assert(no < strings.size());
  return strings[no];
}


vector<string>& Entity::getStringVector(const unsigned no)
{
  assert(no < stringVectors.size());
  return stringVectors[no];
}


const vector<string>& Entity::getStringVector(const unsigned no) const
{
  assert(no < stringVectors.size());
  return stringVectors[no];
}


const string& Entity::getMap(
  const string& str,
  const unsigned no) const
{
  auto it = stringMap.find(str);
  assert(it != stringMap.end());
  return it->second[no];
}


vector<int>& Entity::getIntVector(const unsigned no)
{
  assert(no < intVectors.size());
  return intVectors[no];
}


const vector<int>& Entity::getIntVector(const unsigned no) const
{
  assert(no < intVectors.size());
  return intVectors[no];
}


int Entity::getInt(const unsigned no) const
{
  assert(no < ints.size());
  return ints[no];
}


bool Entity::getBool(const unsigned no) const
{
  assert(no < bools.size());
  return bools[no];
}


double& Entity::getDouble(const unsigned no)
{
  assert(no < doubles.size());
  return doubles[no];
}


const double& Entity::getDouble(const unsigned no) const
{
  assert(no < doubles.size());
  return doubles[no];
}


unsigned Entity::sizeString(const unsigned no) const
{
  assert(no < stringVectors.size());
  return stringVectors[no].size();
}


unsigned Entity::sizeInt(const unsigned no) const
{
  assert(no < intVectors.size());
  return intVectors[no].size();
}


void Entity::setString(
  const unsigned no,
  const string& s)
{
  assert(no < strings.size());
  strings[no] = s;
}


void Entity::setStringVector(
  const unsigned no,
  const vector<string>& v)
{
  assert(no < stringVectors.size());
  stringVectors[no] = v;
}


void Entity::setInt(
  const unsigned no,
  const int i)
{
  assert(no < ints.size());
  ints[no] = i;
}


void Entity::setBool(
  const unsigned no,
  const bool b)
{
  assert(no < bools.size());
  bools[no] = b;
}


void Entity::setDouble(
  const unsigned no,
  const double b)
{
  assert(no < doubles.size());
  doubles[no] = b;
}


void Entity::reverseIntVector(const unsigned no)
{
  assert(no < intVectors.size());
  vector<int> axlesOrig = intVectors[no];
  vector<int>& axles = intVectors[no];

  const int aLast = axles.back();
  const unsigned la = axles.size();
  for (unsigned i = 0; i < la; i++)
    axles[i] = aLast - axlesOrig[la-i-1];
}


string Entity::usage(
  const string& basename,
  const list<CommandLineEntry>& arguments) const
{
  stringstream ss;
  ss << "Usage: " << basename << " [options]\n\n'";

  for (auto& arg: arguments)
  {
    vector<string> lines;
    tokenize(arg.documentation, lines, "\n");

    const string tags = arg.singleDash + ", " + arg.doubleDash;
    ss << setw(20) << left << tags << lines[0] << "\n";
    for (unsigned i = 1; i < lines.size(); i++)
      ss << setw(20) << left << "" << lines[i] << "\n";
    ss << "\n";
  }
  return ss.str();
}

