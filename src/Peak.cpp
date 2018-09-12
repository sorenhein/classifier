#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

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
  maxFlag = false;
  len = 0.f;
  range = 0.f;
  areaCum = 0.f;
  clusterNo = 0;
}


void Peak::logSentinel(const float valueIn)
{
  value = valueIn;
}


void Peak::log(
  const unsigned indexIn,
  const float valueIn,
  const float areaFullIn,
  const Peak& peakPrev)
{
  // The "full" area is the (signed) integral relative to samples[0]
  // in the current interval.  This is also the change in areaCum.
  // The stored area is the (absolute) integral relative to the 
  // lower of the starting and ending point.

  index = indexIn;
  value = valueIn;
  maxFlag = (value > peakPrev.value);
  len = static_cast<float>(index - peakPrev.index);
  range = abs(value - peakPrev.value);
  areaCum = peakPrev.areaCum + areaFullIn;
}


void Peak::logCluster(const unsigned cno)
{
  clusterNo = cno;
}


void Peak::update(const Peak& peakPrev)
{
// cout << "update:\n";
// cout << "index " << index << ", " << peakPrev.index << "\n";
// cout << "prevLen " << peakPrev.len << "\n";
// cout << "value " << value << ", " << peakPrev.value << ", range " <<
  // peakPrev.range << "\n\n";

  len = static_cast<float>(index - peakPrev.index + peakPrev.len);
  range = abs(value - peakPrev.value)+ peakPrev.range;
}


unsigned Peak::getIndex() const
{
  return index;
}


bool Peak::getMaxFlag() const
{
  return maxFlag;
}


float Peak::getValue() const
{
  return value;
}


float Peak::getArea(const Peak& peakPrev) const
{
  // Assumes p2 has a lower index.
  return abs(areaCum - peakPrev.areaCum - 
    (index - peakPrev.index) * min(value, peakPrev.value));
}


bool Peak::check(
  const Peak& p2,
  const unsigned offset) const
{
  bool flag = true;
  vector<bool> issues(7, false);

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

  if (abs(areaCum - p2.areaCum) > RELATIVE_LIMIT * abs(areaCum))
  {
    issues[5] = true;
    flag = false;
  }

  if (clusterNo != p2.clusterNo)
  {
    issues[7] = true;
    flag = false;
  }


  if (! flag)
  {
    cout << Peak::strHeader();
    cout << Peak::str(offset);
    cout << p2.str(offset);

    cout <<
      setw(6) << (issues[0] ? "*" : "") <<
      setw(5) << (issues[1] ? "*" : "") <<
      setw(9) << (issues[2] ? "*" : "") <<
      setw(7) << (issues[3] ? "*" : "") <<
      setw(7) << (issues[4] ? "*" : "") <<
      setw(8) << (issues[5] ? "*" : "") <<
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
  areaCum += p2.areaCum;
}


void Peak::operator /= (const unsigned no)
{
  if (no > 0)
  {
    value /= no;
    len /= no;
    range /= no;
    areaCum /= no;
  }
}

string Peak::strHeader() const
{
  stringstream ss;
  
  ss <<
    setw(6) << "Index" <<
    setw(5) << "Type" <<
    setw(9) << "Value" <<
    setw(7) << "Len" <<
    setw(7) << "Range" <<
    setw(8) << "Area" <<
    setw(8) << "Cluster" <<
    "\n";
  return ss.str();
}


string Peak::str(
  const float areaOut,
  const unsigned offset) const
{
  stringstream ss;

  ss <<
    setw(6) << right << index + offset <<
    setw(5) << (maxFlag ? "max" : "min") <<
    setw(9) << fixed << setprecision(2) << value <<
    setw(7) << fixed << setprecision(2) << len <<
    setw(7) << fixed << setprecision(2) << range <<
    setw(8) << fixed << setprecision(2) << areaOut <<
    setw(8) << right << clusterNo << 
    "\n";
  return ss.str();
}



string Peak::str(
  const Peak& peakPrev,
  const unsigned offset) const
{
  // Prints the "delta" in area.
  return Peak::str(Peak::getArea(peakPrev), offset);
}



string Peak::str(const unsigned offset) const
{
  // Prints the absolute, running area.
  return Peak::str(areaCum, offset);
}

