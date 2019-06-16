#include <sstream>
#include <iomanip>
#include <limits>

#include "Target.h"


Target::Target()
{
  Target::reset();
}


Target::~Target()
{
}


void Target::reset()
{
  abutLeftFlag = false;
  abutRightFlag = false;
  start = 0;
  end = numeric_limits<unsigned>::max();
  borders = BORDERS_SIZE;
  _indices.clear();
}


bool Target::fillPoints(
  const list<unsigned>& carPoints,
  const unsigned indexBase)
{
  // The car points for a complete four-wheeler include:
  // - Left boundary (no wheel)
  // - Left bogie, left wheel
  // - Left bogie, right wheel
  // - Right bogie, left wheel
  // - Right bogie, right wheel
  // - Right boundary (no wheel)
  //
  // Later on, this could also be a three-wheeler or two-wheeler.

  const unsigned nc = carPoints.size();
  if (nc <= 4)
    return false;

  if (abutLeftFlag)
  {
    _indices.resize(nc-2);

    if (! reverseFlag)
    {
      unsigned pi = 0;
      for (auto i = next(carPoints.begin()); i != prev(carPoints.end()); 
          i++, pi++)
        _indices[pi] = indexBase + * i;
    }
    else
    {
      const unsigned pointLast = carPoints.back();
      unsigned pi = nc-3;
      for (auto i = next(carPoints.begin()); i != prev(carPoints.end()); 
          i++, pi--)
        _indices[pi] = indexBase + pointLast - * i;
    }
  }
  else
  {
    const unsigned pointLast = carPoints.back();
    const unsigned pointLastPeak = * prev(prev(carPoints.end()));
    const unsigned pointFirstPeak = * next(carPoints.begin());

    // Fail if no room for left car.
    if (indexBase + pointFirstPeak <= pointLast)
      return false;

    _indices.resize(nc-2);

    if (reverseFlag)
    {
      // The car has a right gap, so we don't need to flip it.
      unsigned pi = 0;
      for (auto i = next(carPoints.begin()); i != prev(carPoints.end()); 
          i++, pi++)
        _indices[pi] = indexBase + * i - pointLast;
    }
    else
    {
      unsigned pi = nc-3;
      for (auto i = next(carPoints.begin()); i != prev(carPoints.end()); 
          i++, pi--)
        _indices[pi] = indexBase - * i;
    }
  }

  return true;
}


bool Target::fill(
  const unsigned modelNoIn,
  const unsigned weightIn,
  const bool reverseFlagIn,
  const unsigned indexRangeLeft,
  const unsigned indexRangeRight,
  const BordersType bordersIn,
  const list<unsigned>& carPoints)
{
  modelNo = modelNoIn;
  weight = weightIn;
  reverseFlag = reverseFlagIn;
  abutLeftFlag = (indexRangeLeft != 0);
  abutRightFlag = (indexRangeRight != 0);

  if (abutLeftFlag && abutRightFlag)
  {
    start = indexRangeLeft;
    end = indexRangeRight;
  }
  else if (abutLeftFlag)
  {
    start = indexRangeLeft;
    end = start + carPoints.back();
  }
  else if (abutRightFlag)
  {
    start = indexRangeRight - carPoints.back();
    end = indexRangeRight;
  }

  borders = bordersIn;

  const unsigned indexBase = 
     (abutLeftFlag ? indexRangeLeft : indexRangeRight);

  if (! Target::fillPoints(carPoints, indexBase))
    return false;
  else 
    return (_indices.front() >= indexRangeLeft &&
        (indexRangeRight == 0 || _indices.back() <= indexRangeRight));
}


void Target::revise(
  const unsigned pos,
  const unsigned value)
{
  if (pos < _indices.size())
    _indices[pos] = value;
}


const vector<unsigned>& Target::indices()
{
  return _indices;
}


unsigned Target::index(const unsigned pos) const
{
  if (pos < _indices.size())
    return _indices[pos];
  else
    return 0;
}


unsigned Target::bogieGap() const
{
  if (_indices.size() != 4)
    return 0;
  else
    return (_indices[3] - _indices[2] + _indices[1] - _indices[0]) / 2;
}


void Target::limits(
  unsigned& limitLower,
  unsigned& limitUpper) const
{
  // Returns limits beyond which unused peak pointers should not be
  // marked down.  (A zero value means no limit in that direction.)
  // When we received enough peaks for several cars, we don't want to
  // discard peaks that may belong to other cars.
  
  if (borders == BORDERS_NONE ||
      borders == BORDERS_DOUBLE_SIDED_SINGLE ||
      borders == BORDERS_DOUBLE_SIDED_SINGLE_SHORT)
  {
    // All the unused peaks are fair game.
    limitLower = 0;
    limitUpper = 0;
  }
  else if (borders == BORDERS_SINGLE_SIDED_LEFT ||
      (borders == BORDERS_DOUBLE_SIDED_DOUBLE && abutLeftFlag))
  {
    limitLower = 0;
    limitUpper = end;
  }
  else if (borders == BORDERS_SINGLE_SIDED_RIGHT ||
      (borders == BORDERS_DOUBLE_SIDED_DOUBLE && abutRightFlag))
  {
    limitLower = start;
    limitUpper = 0;
  }
  else
  {
    // Should not happen.
    limitLower = 0;
    limitUpper = 0;
  }
}


string Target::strIndices(
  const string& title,
  const unsigned offset) const
{
  if (_indices.empty())
    return "";

  stringstream ss;
  ss << title << " indices:";
  for (auto i: _indices)
    ss << " " << i + offset ;

  return ss.str() + "\n";
};


string Target::strGaps(const string& title) const
{
  if (_indices.empty())
    return "";

  stringstream ss;
  ss << title << " gaps:";
  ss << " " << _indices[0] - start;
  for (unsigned i = 1; i < _indices.size(); i++)
    ss << " " << _indices[i] - _indices[i-1];
  ss << " " << end - _indices.back();

  return ss.str() + "\n";
}

