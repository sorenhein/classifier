#include <sstream>
#include <sstream>
#include <limits>

#include "CarCompletion.h"

#include "../Peak.h"
#include "../misc.h"

#define CAR_BOGIE_TOLERANCE 0.3f

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


CarCompletion::CarCompletion()
{
  CarCompletion::reset();
}


CarCompletion::~CarCompletion()
{
}


void CarCompletion::reset()
{
  data.clear();
  limitLower = 0;
  limitUpper = 0;
  abutLeftFlag = false;
  abutRightFlag = false;
  _start = 0;
  _end = numeric_limits<unsigned>::max();
  _indices.clear();

  weight = 0;
  distanceSquared = numeric_limits<unsigned>::max();

}


bool CarCompletion::fillPoints(
  const bool reverseFlag,
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
    // if (indexBase + pointFirstPeak <= pointLast)
      // return false;

    _indices.resize(nc-2);

    if (reverseFlag)
    {
      // The car has a right gap, so we don't need to flip it.
      unsigned pi = 0;
      for (auto i = next(carPoints.begin()); i != prev(carPoints.end());
          i++, pi++)
      {
        if (pointLast > indexBase + * i)
          _indices[pi] = 0;
        else
          _indices[pi] = indexBase + * i - pointLast;
      }
    }
    else
    {
      unsigned pi = nc-3;
      for (auto i = next(carPoints.begin()); i != prev(carPoints.end());
          i++, pi--)
      {
        if (* i > indexBase)
          _indices[pi] = 0;
        else
          _indices[pi] = indexBase - * i;
      }
    }
  }

  return true;
}


void CarCompletion::setLimits(const TargetData& tdata)
{
  // Sets limits beyond which unused peak pointers should not be
  // marked down.  (A zero value means no limit in that direction.)
  // When we received enough peaks for several cars, we don't want to
  // discard peaks that may belong to other cars.

  if (tdata.borders == BORDERS_NONE ||
      tdata.borders == BORDERS_DOUBLE_SIDED_SINGLE ||
      tdata.borders == BORDERS_DOUBLE_SIDED_SINGLE_SHORT)
  {
    // All the unused peaks are fair game.
    limitLower = 0;
    limitUpper = 0;
  }
  else if (tdata.borders == BORDERS_SINGLE_SIDED_LEFT ||
      (tdata.borders == BORDERS_DOUBLE_SIDED_DOUBLE && abutLeftFlag))
  {
    limitLower = 0;
    limitUpper = _end;
  }
  else if (tdata.borders == BORDERS_SINGLE_SIDED_RIGHT ||
      (tdata.borders == BORDERS_DOUBLE_SIDED_DOUBLE && abutRightFlag))
  {
    limitLower = _start;
    limitUpper = 0;
  }
  else
  {
    // Should not happen.
    limitLower = 0;
    limitUpper = 0;
  }
}


bool CarCompletion::fill(
  const TargetData& tdata,
  const unsigned indexRangeLeft,
  const unsigned indexRangeRight,
  const list<unsigned>& carPoints)
{
  abutLeftFlag = (indexRangeLeft != 0);
  abutRightFlag = (indexRangeRight != 0);

  const unsigned indexBase =
     (abutLeftFlag ? indexRangeLeft : indexRangeRight);

  if (! CarCompletion::fillPoints(tdata.reverseFlag, carPoints, indexBase))
    return false;

  if (_indices.front() < indexRangeLeft ||
      (indexRangeRight != 0 && _indices.back() > indexRangeRight))
    return false;

  data.emplace_back(tdata);

  if (abutLeftFlag && abutRightFlag)
  {
    _start = indexRangeLeft;
    _end = indexRangeRight;
  }
  else if (abutLeftFlag)
  {
    _start = indexRangeLeft;
    _end = _start + carPoints.back();
  }
  else if (abutRightFlag)
  {
    if (carPoints.back() > indexRangeRight)
      _start = 0;
    else
      _start = indexRangeRight - carPoints.back();
    _end = indexRangeRight;
  }

  CarCompletion::setLimits(tdata);
  return true;
}


void CarCompletion::registerPeaks(vector<Peak *>& peaksClose)
{
  const unsigned bogieGap = (_indices.size() == 4 ?
    (_indices[3] - _indices[2] + _indices[1] - _indices[0]) / 2 : 0);

  const unsigned bogieTolerance = static_cast<unsigned>
    (CAR_BOGIE_TOLERANCE * bogieGap);

  for (unsigned i = 0; i < peaksClose.size(); i++)
  {
    peakCompletions.emplace_back(PeakCompletion());
    PeakCompletion& pc = peakCompletions.back();

    if (peaksClose[i])
      pc.addMatch(_indices[i], peaksClose[i]);
    else
      pc.addMiss(_indices[i], bogieTolerance);
  }
}


// TODO Not needed
void CarCompletion::addMiss(
  const unsigned target,
  const unsigned tolerance)
{
  peakCompletions.emplace_back(PeakCompletion());
  PeakCompletion& pc = peakCompletions.back();

  pc.addMiss(target, tolerance);
}


void CarCompletion::addMatch(
  const unsigned target,
  Peak * pptr)
{
  peakCompletions.emplace_back(PeakCompletion());
  PeakCompletion& pc = peakCompletions.back();

  pc.addMatch(target, pptr);
}


const vector<unsigned>& CarCompletion::indices()
{
  return _indices;
}


Miterator CarCompletion::begin()
{
  return peakCompletions.begin();
}


Miterator CarCompletion::end()
{
  return peakCompletions.end();
}


void CarCompletion::markWith(
  Peak& peak,
  const CompletionType type,
  const bool forceFlag)
{
  // _forceFlag says whether the whole completion is prepared to
  // force peaks back into existence (in at least one instance).
  // forceFlag says whether the particular peak arose with force or
  // not.  But as it might also have arisen without force, we
  // actually can't make a lot of this.  It probably doesn't matter
  // much in practice anyway.  Otherwise, if we were sure that the
  // peak only would have arisen with force, we would not revive it
  // if data.forceFlag was false.

  UNUSED(forceFlag);
  for (auto& pc: peakCompletions)
  {
    if (pc.markWith(peak, type))
    {
      if (type == COMP_REPAIRABLE)
        CarCompletion::pruneRepairables(pc);
    }
  }
}


unsigned CarCompletion::filled() const
{
  unsigned f = 0;
  for (auto& pc: peakCompletions)
  {
    const auto s = pc.source();
    if (s == COMP_USED || s == COMP_UNUSED || s == COMP_REPAIRED)
      f++;
  }
  return f;
}


bool CarCompletion::complete() const
{
  for (auto& pc: peakCompletions)
  {
    if (pc.source() == COMP_UNMATCHED)
      return false;
  }
  return true;
}


bool CarCompletion::partial() const
{
  for (auto& pc: peakCompletions)
  {
    if (pc.source() == COMP_UNMATCHED)
      return true;
  }
  return false;
}


bool CarCompletion::operator < (const CarCompletion& comp2) const
{
  // Sort first by count, then by model number, then by reverseFlag.
  return (CarCompletion::filled() >= comp2.filled());
}


void CarCompletion::sort()
{
  data.sort();
}


void CarCompletion::getMatch(
  vector<Peak const *>& closestPtrs,
  unsigned& limitLowerOut,
  unsigned& limitUpperOut)
{
  closestPtrs.clear();
  for (auto& pc: peakCompletions)
    closestPtrs.push_back(pc.ptr());

  limitLowerOut = limitLower;
  limitUpperOut = limitUpper;
}


bool CarCompletion::forceFlag() const
{
  return _forceFlag;
}


BordersType CarCompletion::bestBorders() const
{
  BordersType b = BORDERS_SIZE;

  unsigned dBest = numeric_limits<unsigned>::max();
  for (auto& tdata: data)
  {
    if (tdata.distanceSquared < dBest)
    {
      dBest = tdata.distanceSquared;
      b = tdata.borders;
    }
  }
  return b;
}


bool CarCompletion::samePeaks(CarCompletion& miss2)
{
  const unsigned nc = peakCompletions.size();
  if (miss2.peakCompletions.size() != nc)
    return false;

  for (auto pc1 = peakCompletions.begin(), 
      pc2 = miss2.peakCompletions.begin();
      pc1 != peakCompletions.end() && 
      pc2 != miss2.peakCompletions.end();
      pc1++, pc2++)
  {
    if (pc1->ptr() != pc2->ptr())
      return false;
  }
  return true;
}


bool CarCompletion::samePartialPeaks(CarCompletion& miss2)
{
  const unsigned nc = peakCompletions.size();
  if (miss2.peakCompletions.size() != nc)
    return false;

  auto pc1 = peakCompletions.begin();
  auto pc2 = miss2.peakCompletions.begin();

  while (pc1 != peakCompletions.end() && pc2 != miss2.peakCompletions.end())
  {
    Peak const * p1 = pc1->ptr();
    Peak const * p2 = pc2->ptr();

    if (p1 == nullptr)
      pc1++;
    else if (p2 == nullptr)
      pc2++;
    else if (p1 == p2)
    {
      pc1++;
      pc2++;
    }
    else
      return false;
  }

  return (pc1 == peakCompletions.end() && 
    pc2 == miss2.peakCompletions.end());
}


bool CarCompletion::contains(CarCompletion& comp2)
{
  const unsigned nc = peakCompletions.size();
  if (comp2.peakCompletions.size() != nc)
    return false;

  for (auto pc1 = peakCompletions.begin(), 
      pc2 = comp2.peakCompletions.begin();
      pc1 != peakCompletions.end() && 
      pc2 != comp2.peakCompletions.end();
      pc1++, pc2++)
  {
    if (pc2->ptr() && pc1->ptr() != pc2->ptr())
      return false;
  }
  return true;
}


bool CarCompletion::dominates(
  const float dRatio,
  const float qsRatio,
  const float qpRatio,
  const unsigned dSq) const
{
  if (dRatio > 1. && qsRatio > 1. && qpRatio > 1.)
    return true;
  else if (dRatio > 3. && qsRatio > 0.9 && qpRatio > 1.2)
    return true;
  else if (dSq < 50 && dRatio > 0.667 && qsRatio > 1.25 && qpRatio > 1.25)
    return true;
  else
    return false;
}


void CarCompletion::combineSameCount(CarCompletion& carCompl2)
{
  // Both completions miss exactly one peak.  One set does not dominate
  // the other.  We will pick only those peaks that are the same in both.

  for (auto pc1 = peakCompletions.begin(), 
      pc2 = carCompl2.peakCompletions.begin();
      pc1 != peakCompletions.end() && 
      pc2 != carCompl2.peakCompletions.end();
      pc1++, pc2++)
  {
    if (! pc2->ptr())
      pc1->reset();
  }
}


void CarCompletion::updateOverallFrom(CarCompletion& carCompl2)
{
  if (carCompl2.distanceSquared < distanceSquared)
  {
    peakCompletions = carCompl2.peakCompletions;

    distanceSquared = carCompl2.distanceSquared;

    if (carCompl2._forceFlag)
      _forceFlag = true;
  }
}


void CarCompletion::mergeFrom(CarCompletion& miss2)
{
  // Merge the two lists of origins.
  // TODO As sorted, can stop when fewer peaks available.
  for (auto& d2: miss2.data)
  {
    TargetData * d1ptr = nullptr;
    for (auto& d1: data)
    {
      if (d1.modelNo == d2.modelNo)
      {
        // Could have different reverseFlags.  We only allow the best
        // one into the list.
        d1ptr = &d1;
        break;
      }
    }

    if (d1ptr)
    {
      // Don't update weight
      if (d2.distanceSquared < d1ptr->distanceSquared)
      {
        * d1ptr = d2;
        CarCompletion::updateOverallFrom(miss2);
      }
    }
    else
    {
      weight += d2.weight;
      data.push_back(d2);
      CarCompletion::updateOverallFrom(miss2);
    }
  }
}


CondenseType CarCompletion::condense(CarCompletion& miss2)
{
  if (CarCompletion::samePeaks(miss2))
  {
    // Merge the two lists of origins.
    CarCompletion::mergeFrom(miss2);
    return CONDENSE_SAME;
  }
  else if (CarCompletion::samePartialPeaks(miss2))
  {
    // For example the same three peaks show up as 134 and 234.
    return (distanceSquared <= miss2.distanceSquared ?
      CONDENSE_BETTER : CONDENSE_WORSE);
  }
  else if (CarCompletion::contains(miss2))
  {
    return CONDENSE_BETTER;
  }
  else if (miss2.contains(* this))
  {
    return CONDENSE_WORSE;
  }

  const unsigned fill = CarCompletion::filled();
  const unsigned fill2 = miss2.filled();
  const unsigned np = peakCompletions.size();

  if (fill == fill2 && (fill == np || fill+1 == np))
  {
    const float dratio = ratioCappedUnsigned(miss2.distanceSquared, 
      distanceSquared, 100.f);

    const float qsratio = ratioCappedFloat(miss2.qualShapeSum,
      qualShapeSum, 100.f);

    const float qpratio = ratioCappedFloat(miss2.qualPeakSum,
      qualPeakSum, 100.f);

    if (CarCompletion::dominates(dratio, qsratio, qpratio, distanceSquared))
    {
      return CONDENSE_BETTER;
    }
    else if (CarCompletion::dominates(1.f / dratio, 1.f / qsratio, 
      1.f / qpratio, miss2.distanceSquared))
    {
      return CONDENSE_WORSE;
    }
    else if (fill+1 == peakCompletions.size())
    {
      // For example peaks 134 and 234 where one set doesn't dominate.
      // We will go with 34 in that case.
      CarCompletion::combineSameCount(miss2);
      return CONDENSE_BETTER;
    }
    else
      return CONDENSE_DIFFERENT;
  }
  else if (fill < np && fill2 < np && fill != fill2)
  {
    if (data.front().range == RANGE_BOUNDED_RIGHT)
    {
      // Could be a first car.
      return CONDENSE_DIFFERENT;
    }
    else
      return CONDENSE_DIFFERENT;
  }
  else
    return CONDENSE_DIFFERENT;
}


void CarCompletion::makeRepairables()
{
  repairables.clear();
  for (auto& pc: peakCompletions)
  {
    if (pc.source() == COMP_UNMATCHED)
      repairables.push_back(&pc);
  }
  itRep = repairables.begin();
}


bool CarCompletion::nextRepairable(
  Peak& peak,
  bool& forceFlag)
{
  if (itRep == repairables.end())
    return false;

  (* itRep)->fill(peak);
  forceFlag = _forceFlag;
  itRep++;
  return true;
}


void CarCompletion::pruneRepairables(PeakCompletion& pc)
{
  if (itRep == repairables.end() || ** itRep > pc)
    return; // No point
  else if (* itRep == &pc)
    itRep = repairables.erase(itRep);
  else
  {
    for (auto it = next(itRep); it != repairables.end(); it++)
    {
      if (* itRep == &pc)
      {
        repairables.erase(it);
        return;
      }
    }
  }
}


void CarCompletion::makeShift()
{
  int sum = 0;
  int npos = 0;
  int nneg = 0;
  int nall = 0;

  // Make a list of the distances.
  for (auto& pc: peakCompletions)
  {
    if (! pc.ptr())
      continue;

    const int d = pc.distance();
    sum += d;
    if (d > 0)
      npos++;
    if (d < 0)
      nneg++;
    nall++;
  }

  // If they are systematically shifted, compensate for this.
  int shift = 0;
  if (nall > 0 && (npos == 0 || nneg == 0))
    shift = sum / nall;

  for (auto& pc: peakCompletions)
    pc.adjust(-shift);
}


void CarCompletion::calcDistanceSquared()
{
  // At this point there is exactly one element in data.
  distanceSquared = 0;
  for (auto& pc: peakCompletions)
    distanceSquared += pc.distanceShiftSquared();

  if (data.empty())
    return;

  data.front().distanceSquared = distanceSquared;
}


void CarCompletion::calcMetrics()
{
  CarCompletion::calcDistanceSquared();

  qualShapeSum = 0.f;
  qualPeakSum = 0.f;
  for (auto& pc: peakCompletions)
  {
    qualShapeSum += pc.qualityShape();
    qualPeakSum += pc.qualityPeak();
  }
}


string CarCompletion::str(const unsigned offset) const
{
  stringstream ss;
  ss << "Car with weight " << weight << 
    ", squared distance " << distanceSquared;

  if (peakCompletions.empty())
    return ss.str() + " is empty\n\n";

  string smprev = "", sm;
  for (auto& d: data)
  {
    sm = d.strModel();
    if (sm != smprev)
    {
      ss << "\n" << setw(3) << sm << ":";
      smprev = sm;
    }
    ss << d.strSource();
  }

  ss << "\n\n";
  ss << peakCompletions.front().strHeader();
  for (auto& pc: peakCompletions)
    ss << pc.str(offset);
  ss << "\n";

  return ss.str();
}

