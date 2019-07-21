#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cassert>

#include "Entity.h"
#include "parse.h"


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
}


bool Entity::parseField(
  const list<CorrespondenceEntry>& fields,
  const string& tag,
  const string& value)
{
  for (const auto& field: fields)
  {
    if (field.tag == tag)
    {
      if (field.corrType == CORRESPONDENCE_STRING)
      {
        strings[field.no] = value;
      }
      else if (field.corrType == CORRESPONDENCE_STRING_VECTOR)
      {
        parseCommaString(value, stringVectors[field.no]);
      }
      else if (field.corrType == CORRESPONDENCE_INT)
      {
        if (! parseInt(value, ints[field.no]))
        {
          cout << "Bad integer" << endl;
          return false;
        }
      }
      else if (field.corrType == CORRESPONDENCE_BOOL)
      {
        if (! parseBool(value, bools[field.no]))
        {
          cout << "Bad boolean" << endl;
          return false;
        }
      }
      else
      {
        cout << "Bad correspondence type " << endl;
        return false;
      }

      return true;
    }
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

    const size_t c = countDelimiters(line, ",");
    if (c != count)
    {
      cout << err << endl;
      fin.close();
      return false;
    }

    v.clear();
    tokenize(line, v, ",");

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

