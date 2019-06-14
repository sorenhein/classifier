#include <sstream>
#include <limits>

#include "Completion.h"


Completion::Completion()
{
  Completion::reset();
}


Completion::~Completion()
{
}


void Completion::reset()
{
  abutLeftFlag = false;
  abutRightFlag = false;
  start = 0;
  end = numeric_limits<unsigned>::max();
  borders = BORDERS_SIZE;
  _indices.clear();
}


bool Completion::fillPoints(
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

  const unsigned nc = carPoints.size();

  if (abutLeftFlag)
  {
    _indices.resize(nc);

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

    _indices.resize(nc);

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


bool Completion::fill(
  const unsigned modelNoIn,
  const bool reverseFlagIn,
  const unsigned indexRangeLeft,
  const unsigned indexRangeRight,
  const BordersType bordersIn,
  const list<unsigned>& carPoints)
{
  modelNo = modelNoIn;
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

  return Completion::fillPoints(carPoints, indexBase);
}


const vector<unsigned>& Completion::indices()
{
  return _indices;
}


void Completion::limits(
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


string Completion::strIndices(
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

string Completion::strGaps(const string& title) const
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

