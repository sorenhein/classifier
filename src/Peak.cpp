#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "Peak.h"
#include "Except.h"

// Used for comparing values when checking identity.

#define RELATIVE_LIMIT 1.e-4
#define SAMPLE_RATE 2000.


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
  time = 0.;
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

  clusterNo = numeric_limits<unsigned>::max();
  selectFlag = false;
  match = -1;
  ptype = PEAK_TYPE_SIZE;
}


void Peak::logSentinel(
  const float valueIn,
  const bool maxFlagIn)
{
  value = valueIn;
  maxFlag = maxFlagIn;
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
  time = index / SAMPLE_RATE;
  value = valueIn;
  areaCum = areaCumIn;
  maxFlag = maxFlagIn;
}


void Peak::logCluster(const unsigned cno)
{
  clusterNo = cno;
}


void Peak::logMatch(const int matchIn)
{
  match = matchIn;
}


void Peak::logType(const PeakType ptypeIn)
{
  ptype = ptypeIn;
}


void Peak::update(
  Peak * peakPrev,
  const Peak * peakNext)
{
  // This is used before we delete all peaks from peakFirst (included)
  // up to the present peak (excluded).  
  // peakPrev is the predecessor of peakFirst if this exists.
  // peakNext is the successor of the present peak if this exists.

  if (peakPrev == nullptr)
    THROW(ERR_NO_PEAKS, "No previous peak");

  len = static_cast<float>(index - peakPrev->index);
  range = abs(value - peakPrev->value);
  area = abs(areaCum - peakPrev->areaCum - 
    (index - peakPrev->index) * min(value, peakPrev->value));

  gradient = range / len;
  fill = area / (0.5f * range * len);

  if (peakNext != nullptr)
  {
    // Next area will not in general be set!
    const float areaNext = abs(peakNext->areaCum - areaCum - 
      (peakNext->index - index) * min(peakNext->value, value));
    symmetry = area / areaNext;
  }

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

  if (len > 0)
  {
    gradient = range / len;
    fill = area / (0.5f * range * len);
  }

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

  const float vdiff = (value - p2.value) / scale.value;
  const float vlen = (len - p2.len) / scale.len;
  const float vgrad = (gradient - p2.gradient) / scale.gradient;
  const float vsymm = symmetry - p2.symmetry;

  return vdiff * vdiff + vlen * vlen + vgrad * vgrad + vsymm * vsymm;
}


unsigned Peak::getIndex() const
{
  return index;
}


double Peak::getTime() const
{
  return time;
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


float Peak::getSymmetry() const
{
  return symmetry;
}


unsigned Peak::getCluster() const
{
  return clusterNo;
}


int Peak::getMatch() const
{
  return match;
}


PeakType Peak::getType() const
{
  return ptype;
}


bool Peak::isCluster(const unsigned cno) const
{
  return (cno == clusterNo);
}


bool Peak::isCandidate() const
{
  return (! maxFlag && value < 0.f);
}


float Peak::penalty(const float val) const
{
  const float a = abs(val);
  const float v = (a < 1.f ? 1.f / a : a);
  if (v <= 2.f)
    return 0.f;
  else
    return (v - 2.f) * (v - 2.f);
}


float Peak::measure(const Peak& scale)
{
  measureFlag = true;
  if (len == 0)
  {
    measureVal = 999.99f;
    return measureVal;
  }

  measureVal = Peak::penalty(value / scale.value) +
    Peak::penalty(len / scale.len) +
    Peak::penalty(range / scale.range) +
    Peak::penalty(area / scale.area) +
    Peak::penalty(gradient / scale.gradient);
  return measureVal;
}


float Peak::measure() const
{
  if (! measureFlag)
    THROW(ERR_ALGO_MEASURE, "Measure not set");
  return measureVal;
}


void Peak::select()
{
  selectFlag = true;
}


bool Peak::isSelected() const
{
  return selectFlag;
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
    setw(8) << fixed << setprecision(2) << 100.f * gradient <<
    setw(8) << fixed << setprecision(2) << fill <<
    setw(8) << fixed << setprecision(2) << symmetry <<
    setw(6) << right << (clusterNo == numeric_limits<unsigned>::max() ? 
      "-" : to_string(clusterNo)) <<
    "\n";
  return ss.str();
}

