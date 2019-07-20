#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cassert>

#include "Entity.h"
#include "../util/parse.h"


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


bool Entity::readFile(
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

    bool seenFlag = false;
    bool errFlag = false;
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
            cout << err << endl;
            errFlag = true;
            break;
          }
        }
        else if (field.corrType == CORRESPONDENCE_BOOL)
        {
          if (! parseBool(value, bools[field.no]))
          {
            cout << err << endl;
            errFlag = true;
            break;
          }
        }
        else
        {
          errFlag = true;
          cout << "File " << fname << ": Bad correspondence type " <<
            field.corrType << endl;
          break;
        }

        seenFlag = true;
        break;
      }
    }

    if (! seenFlag && ! errFlag)
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

