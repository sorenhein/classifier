#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "Peak.h"

#define RELATIVE_LIMIT 1.e-4


Peak::Peak()
{
  Peak::reset();
}


Peak::~Peak()
{
}


void Peak::reset()
{
  index = 0;
  value = 0.f;
  maxFlag = true;
  len = 0.f;
  range = 0.f;
  area = 0.f;
}


void Peak::log(
  const unsigned indexIn,
  const float valueIn,
  const bool maxFlagIn,
  const float lenIn,
  const float rangeIn,
  const float areaIn)
{
  index = indexIn;
  value = valueIn;
  maxFlag = maxFlagIn;
  len = lenIn;
  range = rangeIn;
  area = areaIn;
}


bool Peak::check(const Peak& p2) const
{
  bool flag = true;
  vector<bool> issues(6, false);

  if (index != p2.index)
  {
    issues[0] = true;
    flag = false;
  }

  if (abs(value - p2.value) > RELATIVE_LIMIT * abs(value))
  {
    issues[1] = true;
    flag = false;
  }

  if (maxFlag != p2.maxFlag)
  {
    issues[2] = true;
    flag = false;
  }

  if (abs(len - p2.len) > RELATIVE_LIMIT * len)
  {
    issues[3] = true;
    flag = false;
  }

  if (abs(range - p2.range) > RELATIVE_LIMIT * range)
  {
    issues[4] = true;
    flag = false;
  }

  if (abs(area - p2.area) > RELATIVE_LIMIT * area)
  {
    issues[5] = true;
    flag = false;
  }

  if (! flag)
  {
    cout << Peak::strHeader();
    cout << Peak::str();
    cout << p2.str();

    cout <<
      setw(5) << right << "" <<
      setw(6) << (issues[0] ? "*" : "") <<
      setw(9) << (issues[1] ? "*" : "") <<
      setw(5) << (issues[2] ? "*" : "") <<
      setw(6) << (issues[3] ? "*" : "") <<
      setw(7) << (issues[4] ? "*" : "") <<
      setw(8) << (issues[5] ? "*" : "") <<
    "\n\n";
  }

  return flag;
}


void Peak::operator += (const Peak& p2)
{
  value += p2.value;
  len += p2.len;
  range += p2.range;
  area += p2.area;
}


void Peak::operator /= (const unsigned no)
{
  if (no > 0)
  {
    value /= no;
    len /= no;
    range /= no;
    area /= no;
  }
}

string Peak::strHeader() const
{
  stringstream ss;
  
  ss <<
    setw(5) << "No." <<
    setw(6) << "Index" <<
    setw(9) << "Value" <<
    setw(5) << "Type" <<
    setw(6) << "Len" <<
    setw(7) << "Range" <<
    setw(8) << "Area" <<
    "\n";
  return ss.str();
}


string Peak::str(
  const unsigned no,
  const unsigned offset) const
{
  stringstream ss;

  ss <<
    setw(5) << right << no <<
    setw(6) << right << index + offset <<
    setw(9) << fixed << setprecision(2) << value <<
    setw(5) << (maxFlag ? "max" : "min") <<
    setw(7) << fixed << setprecision(2) << len <<
    setw(7) << fixed << setprecision(2) << range <<
    setw(7) << fixed << setprecision(2) << area <<
    "\n";
  return ss.str();
}

