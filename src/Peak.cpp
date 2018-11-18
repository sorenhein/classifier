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
  left.reset();
  right.reset();

  rangeRatio = 0.f;
  gradRatio = 0.f;

  qualityPeak = 0.f;
  qualityShape = 0.f;

  seedFlag = false;

  wheelFlag = false;
  wheelSide = WHEEL_SIZE;

  bogeySide = BOGEY_SIZE;
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


void Peak::logNextPeak(Peak const * nextPtrIn)
{
  if (nextPtrIn == nullptr)
    return;

  right = nextPtrIn->left;

  if (right.range)
    rangeRatio = left.range / right.range;
  if (right.gradient)
    gradRatio = left.gradient / right.gradient;
}


void Peak::update(const Peak * peakPrev)
{
  // This is used before we delete all peaks from peakFirst (included)
  // up to the present peak (excluded).  
  // peakPrev is the predecessor of peakFirst if this exists.

  if (peakPrev == nullptr)
    return;

  left.len = static_cast<float>(index - peakPrev->index);
  left.range = abs(value - peakPrev->value);
  left.area = abs(areaCum - peakPrev->areaCum - 
    (index - peakPrev->index) * min(value, peakPrev->value));

  left.gradient = left.range / left.len;
  left.fill = left.area / (0.5f * left.range * left.len);
}


void Peak::annotate(const Peak * peakPrev)
{
  // This is used once the basic parameters of the peaks have been set.

  if (peakPrev == nullptr)
  {
    left.len = static_cast<float>(index);
    left.range = 0.f;
    left.area = 0.f;
  }
  else
  {
    left.len = static_cast<float>(index - peakPrev->index);
    left.range = abs(value - peakPrev->value);
    left.area = abs(areaCum - peakPrev->areaCum - 
      (index - peakPrev->index) * min(value, peakPrev->value));
  }

  if (left.len > 0)
  {
    left.gradient = left.range / left.len;
    left.fill = left.area / (0.5f * left.range * left.len);
  }
}


float Peak::distance(
  const Peak& p2,
  const Peak& scale) const
{
  const float vdiff = (value - p2.value) / scale.value;
  const float vlen = (left.len - p2.left.len) / scale.left.len;
  const float vgrad = (left.gradient - p2.left.gradient) / 
    scale.left.gradient;

  return vdiff * vdiff + vlen * vlen + vgrad * vgrad;
}


float Peak::calcQualityPeak(const Peak& scale) const
{
  // level is supposed to be negative.  Large negative peaks are OK.
  const float vdiff = (value > scale.value ?
    (value - scale.value) / scale.value : 0.f);

  // Ranges are always positive
  const float vrange1 = (left.range < scale.left.range ?
    (left.range - scale.left.range) / scale.left.range : 0.f);

  const float vrange2 = (right.range < scale.right.range ?
    (right.range - scale.right.range) / scale.right.range : 0.f);

  const float vgrad1 = (left.gradient - scale.left.gradient) /
    scale.left.gradient;

  const float vgrad2 = (right.gradient - scale.right.gradient) /
    scale.right.gradient;

  return
    vdiff * vdiff +
    vrange1 * vrange1 +
    vrange2 * vrange2 +
    vgrad1 * vgrad1 +
    vgrad2 * vgrad2;
}


float Peak::calcQualityShape(const Peak& scale) const
{
  // Ranges are always positive.
  const float vrange1 = (left.range < 0.5f * scale.left.range ?
    (left.range - scale.left.range) / scale.left.range : 0.f);

  float vrange2 = (right.range < 0.5f * scale.right.range ?
    (right.range - scale.right.range) / scale.right.range : 0.f);

  // Gradients are always positive.
  const float vgrad1 = (left.gradient < scale.left.gradient ?
    (left.gradient - scale.left.gradient) / scale.left.gradient : 0.f);

  const float vgrad2 = (right.gradient < scale.right.gradient ?
    (right.gradient - scale.right.gradient) / scale.right.gradient : 0.f);

  return
    vrange1 * vrange1 +
    vrange2 * vrange2 +
    vgrad1 * vgrad1 +
    vgrad2 * vgrad2;
}


void Peak::calcQualities(const Peak& scale)
{
  qualityPeak = Peak::calcQualityPeak(scale);
  qualityShape = Peak::calcQualityShape(scale);
}


void Peak::calcQualities(const vector<Peak>& scales)
{
  float distPeakMin = numeric_limits<float>::max();
  float distShapeMin = numeric_limits<float>::max();

  for (auto& scale: scales)
  {
    const float dPeak = Peak::calcQualityPeak(scale);
    if (dPeak < distPeakMin)
      distPeakMin = dPeak;

    const float dShape = Peak::calcQualityShape(scale);
    if (dShape < distShapeMin)
      distShapeMin = dShape;
  }

  qualityPeak = distPeakMin;
  qualityShape = distShapeMin;
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


float Peak::getRange() const
{
  return left.range;
}


float Peak::getArea() const
{
  // Assumes that the differential area has already been annotated.
  // Return differential area to the immediate predecessor.
  return left.area;
}


float Peak::getArea(const Peak& p2) const
{
  // Calculate differential area to some other peak, not necessarily
  // the immediate predecessor (but a predecessor).
  return abs(areaCum - p2.areaCum -
    (index - p2.index) * min(value, p2.value));
}


float Peak::getQualityPeak() const
{
  return qualityPeak;
}


float Peak::getQualityShape() const
{
  return qualityShape;
}


bool Peak::isCandidate() const
{
  return (! maxFlag && value < 0.f);
}


void Peak::setSeed()
{
  seedFlag = true;
}


void Peak::unsetSeed()
{
  seedFlag = false;
}


bool Peak::isSeed() const
{
  return seedFlag;
}


void Peak::markWheel(const WheelType wheelType)
{
  wheelFlag = true;
  wheelSide = wheelType;
}


void Peak::markNoWheel()
{
  wheelFlag = false;
  wheelSide = WHEEL_SIZE;
}


bool Peak::isLeftWheel() const
{
  return (wheelFlag && wheelSide == WHEEL_LEFT);
}


bool Peak::isRightWheel() const
{
  return (wheelFlag && wheelSide == WHEEL_RIGHT);
}


bool Peak::isWheel() const
{
  return wheelFlag;
}


void Peak::markBogey(const BogeyType bogeyType)
{
  bogeySide = bogeyType;
}


bool Peak::isLeftBogey() const
{
  return (bogeySide == BOGEY_LEFT);
}


bool Peak::isRightBogey() const
{
  return (bogeySide == BOGEY_RIGHT);
}


bool Peak::isBogey() const
{
  return (bogeySide != BOGEY_SIZE);
}


bool Peak::greatQuality() const
{
  // TODO #define
  return (qualityPeak <= 0.3f || qualityShape <= 0.2f);
}


bool Peak::goodQuality() const
{
  // TODO #define
  return (qualityShape <= 0.5f);
}


bool Peak::acceptableQuality() const
{
  // TODO #define
  return (qualityShape <= 0.75f);
}


bool Peak::similarGradient(
  const Peak& p1,
  const Peak& p2) const
{
  // We are the peak preceding p1, which precedes p2.
  const float lenNew = left.len + p1.left.len + p2.left.len;
  const float rangeNew = abs(p2.value - value) + left.range;
  const float gradNew = rangeNew / lenNew;

  // TODO #define
  return (gradNew >= 0.9f * left.gradient && 
    gradNew <= 1.1f * left.gradient);
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
  Peak::deviation(value, p2.value, issues[1], flag);
  Peak::deviation(maxFlag, p2.maxFlag, issues[2], flag);

  Peak::deviation(left.len, p2.left.len, issues[3], flag);
  Peak::deviation(left.range, p2.left.range, issues[4], flag);
  Peak::deviation(left.area, p2.left.area, issues[5], flag);
  Peak::deviation(left.gradient, p2.left.gradient, issues[6], flag);
  Peak::deviation(left.fill, p2.left.fill, issues[7], flag);

  if (! flag)
  {
    cout << Peak::strHeader();
    cout << Peak::str(offset);
    cout << p2.str(offset);

    cout <<
      setw(6) << (issues[0] ? "*" : "") << // index
      setw(9) << (issues[1] ? "*" : "") << // value
      setw(5) << (issues[2] ? "*" : "") << // maxFlag
      setw(7) << (issues[3] ? "*" : "") << // len
      setw(7) << (issues[4] ? "*" : "") << // range
      setw(8) << (issues[5] ? "*" : "") << // area
      setw(8) << (issues[6] ? "*" : "") << // gradient
      setw(8) << (issues[7] ? "*" : "") << // flag
    "\n\n";
  }

  return flag;
}


void Peak::operator += (const Peak& p2)
{
  // This is not a "real" peak, but an accumulator for statistics.
  value += p2.value;
  areaCum += p2.areaCum;

  left += p2.left;
  right += p2.right;

  rangeRatio += p2.rangeRatio;
  gradRatio += p2.gradRatio;

  qualityPeak += p2.qualityPeak;
  qualityShape += p2.qualityShape;
}


void Peak::operator /= (const unsigned no)
{
  if (no > 0)
  {
    value /= no;
    areaCum /= no;

    left /= no;
    right /= no;

    rangeRatio /= no;
    gradRatio /= no;

    qualityPeak /= no;
    qualityShape /= no;
  }
}

string Peak::strHeader() const
{
  stringstream ss;
  
  ss <<
    setw(6) << "Index" <<
    setw(9) << "Value" <<
    setw(5) << "Type" <<
    setw(7) << "Len" <<
    setw(7) << "Range" <<
    setw(8) << "Area" <<
    setw(8) << "Grad" <<
    setw(8) << "Fill" <<
    "\n";
  return ss.str();
}


string Peak::strHeaderSum() const
{
  stringstream ss;
  
  ss <<
    setw(6) << "Index" <<
    setw(8) << "Value" <<
    setw(8) << "Range1" <<
    setw(8) << "Range2" <<
    setw(8) << "Grad1" <<
    setw(8) << "Grad2" <<
    setw(8) << "Fill1" <<
    "\n";
  return ss.str();
}


string Peak::strHeaderQuality() const
{
  stringstream ss;
  
  ss <<
    setw(6) << "Index" <<
    setw(8) << "Value" <<
    setw(8) << "Range1" <<
    setw(8) << "Range2" <<
    setw(8) << "Grad1" <<
    setw(8) << "Grad2" <<
    setw(8) << "Squal" <<
    setw(8) << "Quality" <<
    "\n";
  return ss.str();
}


string Peak::str(const unsigned offset) const
{
  stringstream ss;

  ss <<
    setw(6) << std::right << index + offset <<
    setw(9) << fixed << setprecision(2) << value <<
    setw(5) << (maxFlag ? "max" : "min") <<
    setw(7) << fixed << setprecision(2) << left.len <<
    setw(7) << fixed << setprecision(2) << left.range <<
    setw(8) << fixed << setprecision(2) << left.area <<
    setw(8) << fixed << setprecision(2) << 100.f * left.gradient <<
    setw(8) << fixed << setprecision(2) << left.fill <<
    "\n";
  return ss.str();
}


string Peak::strSum(const unsigned offset) const
{
  stringstream ss;

  ss <<
    setw(6) << std::right << index + offset <<
    setw(8) << fixed << setprecision(2) << value <<
    setw(8) << fixed << setprecision(2) << left.range <<
    setw(8) << fixed << setprecision(2) << right.range <<
    setw(8) << fixed << setprecision(2) << 100. * left.gradient <<
    setw(8) << fixed << setprecision(2) << 100. * right.gradient <<
    setw(8) << fixed << setprecision(2) << left.fill <<
    "\n";
  return ss.str();
}


string Peak::strQuality(const unsigned offset) const
{
  stringstream ss;

  ss <<
    setw(6) << std::right << index + offset <<
    setw(8) << fixed << setprecision(2) << value <<
    setw(8) << fixed << setprecision(2) << left.range <<
    setw(8) << fixed << setprecision(2) << right.range <<
    setw(8) << fixed << setprecision(2) << 100. * left.gradient <<
    setw(8) << fixed << setprecision(2) << 100. * right.gradient <<
    setw(8) << fixed << setprecision(2) << qualityShape <<
    setw(8) << fixed << setprecision(2) << qualityPeak << "";

  string str = "  ";
  if (! seedFlag || ! wheelFlag)
  {
    if (Peak::greatQuality())
      str += "***";
    else if (Peak::goodQuality())
      str += "**";
    else if (Peak::acceptableQuality())
      str += "*";
  }
  else if (bogeySide != BOGEY_SIZE)
  {
    if (bogeySide == BOGEY_LEFT)
      str += (wheelSide == WHEEL_LEFT ? "1" : " 2");
    else 
      str += (wheelSide == WHEEL_LEFT ? "  3" : "   4");
  }
  else if (wheelFlag)
  {
    str += "      ";
    str += (wheelSide == WHEEL_LEFT ? "1" : " 2");
  }
  str += "\n";

  if (bogeySide == BOGEY_RIGHT && wheelSide == WHEEL_RIGHT)
    str += string(62, '-') + "\n";

  return ss.str() + str;
}

