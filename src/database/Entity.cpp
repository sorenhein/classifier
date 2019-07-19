#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "Entity.h"
#include "../util/misc.h"


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
          if (! parseInt(value, ints[field.no], err))
          {
            errFlag = true;
            break;
          }
        }
        else if (field.corrType == CORRESPONDENCE_BOOL)
        {
          if (! parseBool(value, bools[field.no], err))
          {
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
  return ints[no];
}


const int& Entity::operator [](const unsigned no) const
{
  return ints[no];
}


const string Entity::getString(const unsigned no)
{
  if (no >= strings.size())
    return "";
  else
    return strings[no];
}


const vector<string> Entity::getStringVector(const unsigned no)
{
  if (no >= stringVectors.size())
    return { "" };
  else
    return stringVectors[no];
}


int Entity::getInt(const unsigned no) const
{
  if (no >= ints.size())
    return -1;
  else
    return ints[no];
}


bool Entity::getBool(const unsigned no) const
{
  if (no >= bools.size())
    return false;
  else
    return bools[no];
}


unsigned Entity::sizeString(const unsigned no) const
{
  if (no >= stringVectors.size())
    return 0;
  else
    return stringVectors[no].size();
}


unsigned Entity::sizeInt(const unsigned no) const
{
  if (no >= intVectors.size())
    return 0;
  else
    return intVectors[no].size();
}


void Entity::setString(
  const unsigned no,
  const string& s)
{
  if (no < strings.size())
    strings[no] = s;
}


void Entity::setStringVector(
  const unsigned no,
  const vector<string>& v)
{
  if (no < stringVectors.size())
    stringVectors[no] = v;
}


void Entity::setInt(
  const unsigned no,
  const int i)
{
  if (no < ints.size())
    ints[no] = i;
}


void Entity::setBool(
  const unsigned no,
  const bool b)
{
  if (no < bools.size())
    bools[no] = b;
}

