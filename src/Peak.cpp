#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "Peak.h"
#include "Except.h"

// Used for comparing values when checking identity.

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
  // Absolute quantities
  index = 0;
  value = 0.f;
  areaCum = 0.f;
  maxFlag = false;

  // Derived quantities
  len = 0.f;
  range = 0.f;
  area = 0.f;
  gradient = 0.f;
  fill = 0.f;
  symmetry = 0.f;

  clusterNo = 0;
}


void Peak::logSentinel(const float valueIn)
{
  value = valueIn;
}


void Peak::log(
  const unsigned indexIn,
  const float valueIn,
  const float areaCumIn,
  const bool maxFlagIn)
{
  // The "full" area is the (signed) integral relative to samples[0]
  // in the current interval.  This is also the change in areaCum.
  // The stored area is the (absolute) integral relative to the 
  // lower of the starting and ending point.

  index = indexIn;
  value = valueIn;
  areaCum = areaCumIn;
  maxFlag = maxFlagIn;
}


void Peak::logCluster(const unsigned cno)
{
  clusterNo = cno;
}


void Peak::update(
  Peak * peakPrev,
  const Peak * peakFirst,
  const Peak * peakNext)
{
  // This is used before we delete all peaks from peakFirst (included)
  // up to the present peak (excluded).  
  // peakPrev is the predecessor of peakFirst if this exists.
  // peakNext is the successor of the present peak if this exists.

  if (peakFirst == nullptr)
    THROW(ERR_NO_PEAKS, "No peak");

  const Peak * peakRef = (peakFirst == nullptr ? peakPrev : peakFirst);

  len = static_cast<float>(index - peakRef->index);
  range = abs(value - peakRef->value);
  area = abs(areaCum - peakRef->areaCum - 
    (index - peakRef->index) * min(value, peakRef->value));

  gradient = range / len;
  fill = area / (0.5f * range * len);

  if (peakNext != nullptr)
    symmetry = area / peakNext->area;

  if (peakPrev != nullptr)
    peakPrev->symmetry = peakPrev->area / area;
}


void Peak::annotate(
  const Peak * peakPrev,
  const Peak * peakNext)
{
  // This is used once the basic parameters of the peaks have been set.

  if (peakPrev == nullptr)
  {
    len = static_cast<float>(index);
    range = 0.f;
    area = 0.f;
  }
  else
  {
    len = static_cast<float>(index - peakPrev->index);
    range = abs(value - peakPrev->value);
    area = abs(areaCum - peakPrev->areaCum - 
      (index - peakPrev->index) * min(value, peakPrev->value));
  }

  gradient = range / len;
  fill = area / (0.5f * range * len);

  if (peakNext == nullptr)
    symmetry = 1.f;
  else
  {
    // Next area will not in general be set!
    const float areaNext = abs(peakNext->areaCum - areaCum - 
      (peakNext->index - index) * min(peakNext->value, value));
    symmetry = area / areaNext;
  }
}


float Peak::distance(
  const Peak& p2,
  const Peak& scale) const
{
  // This distance-squared function is used in K-means clustering.
  // Many possibilities here: len, range etc.
  // Normalized or not, and if so, by what.

  const float vdiff = abs(value - p2.value) / scale.value;
  const float vfill = abs(fill - p2.fill);
  const float vgrad = abs(gradient - p2.gradient) / scale.gradient;
  const float vsymm = area / p2.area;

  return vdiff * vdiff + vfill * vfill + vgrad * vgrad + vsymm * vsymm;
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


float Peak::getAreaCum() const
{
  return areaCum;
}


float Peak::getLength() const
{
  return len;
}


float Peak::getRange() const
{
  return range;
}


float Peak::getArea() const
{
  // Assumes that the differential area has already been annotated.
  // Return differential area to the immediate predecessor.
  return area;
}


float Peak::getArea(const Peak& p2) const
{
  // Calculate differential area to some other peak, not necessarily
  // the immediate predecessor (but a predecessor).
  return abs(areaCum - p2.areaCum -
    (index - p2.index) * min(value, p2.value));
}


void Peak::deviation(
  const unsigned v1,
  const unsigned v2,
  unsigned& issue,
  bool& flag) const
{
  if (v1 != v2)
  {
    issue = 1;
    flag = false;
  }
}


void Peak::deviation(
  const bool v1,
  const bool v2,
  unsigned& issue,
  bool& flag) const
{
  if (v1 != v2)
  {
    issue = 1;
    flag = false;
  }
}


void Peak::deviation(
  const float v1,
  const float v2,
  unsigned& issue,
  bool& flag) const
{
  if (abs(v1-v2) > RELATIVE_LIMIT * abs(v1))
  {
    issue = 1;
    flag = false;
  }
}


bool Peak::check(
  const Peak& p2,
  const unsigned offset) const
{
  bool flag = true;
  vector<unsigned> issues(10, 0);

  Peak::deviation(index, p2.index, issues[0], flag);
  Peak::deviation(maxFlag, p2.maxFlag, issues[1], flag);
  Peak::deviation(value, p2.value, issues[2], flag);
  Peak::deviation(len, p2.len, issues[3], flag);
  Peak::deviation(range, p2.range, issues[4], flag);
  Peak::deviation(area, p2.area, issues[5], flag);
  Peak::deviation(gradient, p2.gradient, issues[6], flag);
  Peak::deviation(fill, p2.fill, issues[7], flag);
  Peak::deviation(symmetry, p2.symmetry, issues[8], flag);
  Peak::deviation(clusterNo, p2.clusterNo, issues[9], flag);

  if (! flag)
  {
    cout << Peak::strHeader();
    cout << Peak::str(offset);
    cout << p2.str(offset);

    cout <<
      setw(6) << (issues[0] ? "*" : "") << // index
      setw(5) << (issues[1] ? "*" : "") << // maxFlag
      setw(9) << (issues[2] ? "*" : "") << // value
      setw(7) << (issues[3] ? "*" : "") << // len
      setw(7) << (issues[4] ? "*" : "") << // range
      setw(8) << (issues[5] ? "*" : "") << // area
      setw(8) << (issues[5] ? "*" : "") << // gradient
      setw(8) << (issues[5] ? "*" : "") << // fill
      setw(8) << (issues[5] ? "*" : "") << // symmetry
      setw(6) << (issues[9] ? "*" : "") << // clusterNo
    "\n\n";
  }

  return flag;
}


void Peak::operator += (const Peak& p2)
{
  // This is not a "real" peak, but an accumulator for statistics.
  value += p2.value;
  areaCum += p2.areaCum;
  len += p2.len;
  range += p2.range;
  area += p2.area;
  gradient += p2.gradient;
  fill += p2.fill;
  symmetry += p2.symmetry;
}


void Peak::operator /= (const unsigned no)
{
  if (no > 0)
  {
    value /= no;
    areaCum /= no;
    len /= no;
    range /= no;
    area /= no;
    gradient /= no;
    fill /= no;
    symmetry /= no;
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
    setw(8) << "Grad" <<
    setw(8) << "Fill" <<
    setw(8) << "Symm" <<
    setw(6) << "Group" <<
    "\n";
  return ss.str();
}


string Peak::str(const unsigned offset) const
{
  stringstream ss;

  ss <<
    setw(6) << right << index + offset <<
    setw(5) << (maxFlag ? "max" : "min") <<
    setw(9) << fixed << setprecision(2) << value <<
    setw(7) << fixed << setprecision(2) << len <<
    setw(7) << fixed << setprecision(2) << range <<
    setw(8) << fixed << setprecision(2) << area <<
    setw(8) << fixed << setprecision(2) << gradient <<
    setw(8) << fixed << setprecision(2) << fill <<
    setw(8) << fixed << setprecision(2) << symmetry <<
    setw(6) << right << clusterNo <<
    "\n";
  return ss.str();
}

