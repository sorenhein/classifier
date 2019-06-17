#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "PeakPattern.h"
#include "PeakRange.h"
#include "Except.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))

// If we try to interpolate an entire car, the end gap should not be
// too large relative to the bogie gap.
#define NO_BORDER_FACTOR 3.0f

// The inter-car gap should be larger than effectively zero.
#define SMALL_BORDER_FACTOR 0.1f

// Limits for match between model and actual gap.
#define LEN_FACTOR_GREAT 0.05f
#define LEN_FACTOR_GOOD 0.10f

// Expect a short car to be within these factors of a typical car.
#define SHORT_FACTOR 0.5f
#define LONG_FACTOR 0.9f


PeakPattern::PeakPattern()
{
  PeakPattern::reset();
}


PeakPattern::~PeakPattern()
{
}


void PeakPattern::reset()
{
  carBeforePtr = nullptr;
  carAfterPtr = nullptr;

  activeEntries.clear();
  targets.clear();
}


bool PeakPattern::setGlobals(
  const CarModels& models,
  const PeakRange& range,
  const unsigned offsetIn)
{
  if (models.size() == 0)
    return false;

  carBeforePtr = range.carBeforePtr();
  carAfterPtr = range.carAfterPtr();

  offset = offsetIn;

  models.getTypical(bogieTypical, longTypical, sideTypical);
  if (sideTypical == 0)
    sideTypical = bogieTypical;

  if (range.characterize(models, rangeData))
  {
    cout << "\n" << rangeData.str("Range globals", offset) << "\n";
    return true;
  }
  else
    return false;
}


void PeakPattern::getActiveModels(const CarModels& models)
{
  activeEntries.clear();

  for (unsigned index = 0; index < models.size(); index++)
  {
    if (models.empty(index))
      continue;

    ModelData const * data = models.getData(index);
    if (data->containedFlag || ! data->bothBogiesFlag)
      continue;

    activeEntries.emplace_back(ActiveEntry());
    ActiveEntry& ae = activeEntries.back();
    ae.data = data;
    ae.index = index;
    ae.fullFlag = data->fullFlag;

    // Three different qualities; only two used for now.
    ae.lenLo.resize(3);
    ae.lenHi.resize(3);

    unsigned len;
    if (data->gapLeft == 0)
    {
      if (data->gapRight == 0)
        cout << "getActiveModels ERROR: No gaps\n";

      len = data->lenPP + data->gapLeft + 2 * data->gapRight;
    }
    else if (data->gapRight == 0)
      len = data->lenPP + 2 * data->gapLeft + data->gapRight;
    else
      len = data->lenPP + data->gapLeft + data->gapRight;

    ae.lenLo[QUALITY_ACTUAL_GAP] =
      static_cast<unsigned>((1.f - LEN_FACTOR_GREAT) * len);
    ae.lenHi[QUALITY_ACTUAL_GAP] =
      static_cast<unsigned>((1.f + LEN_FACTOR_GREAT) * len);

    ae.lenLo[QUALITY_BY_SYMMETRY] = 
      static_cast<unsigned>((1.f - LEN_FACTOR_GOOD) * len);
    ae.lenHi[QUALITY_BY_SYMMETRY] =
      static_cast<unsigned>((1.f + LEN_FACTOR_GOOD) * len);
  }
}


bool PeakPattern::addModelTargets(
  const CarModels& models,
  const unsigned indexModel,
  const bool symmetryFlag,
  const BordersType patternType)
{
  CarDetect car;
  models.getCar(car, indexModel);
  const unsigned weight = models.count(indexModel);

  list<unsigned> carPoints;
  models.getCarPoints(indexModel, carPoints);

  Target target;
  bool seenFlag = false;

  if (car.hasLeftGap())
  {
    if (target.fill(indexModel, weight, false, 
          rangeData.indexLeft, rangeData.indexRight, 
          patternType, carPoints))
    {
      targets.emplace_back(target);
      seenFlag = true;
    }
  }

  if (! symmetryFlag && car.hasRightGap())
  {
    if (target.fill(indexModel, weight, true, 
          rangeData.indexLeft, rangeData.indexRight, 
          patternType, carPoints))
    {
      targets.emplace_back(target);
      seenFlag = true;
    }
  }

  return seenFlag;
}


bool PeakPattern::guessNoBorders()
{
  // This is a half-hearted try to fill in exactly one car of the 
  // same type as its neighbors if those neighbors do not have any
  // edge gaps at all.

  if (! carBeforePtr || ! carAfterPtr)
    return false;

  if (carBeforePtr->index() != carAfterPtr->index())
    return false;

  if (rangeData.qualLeft != QUALITY_NONE ||
      rangeData.qualRight != QUALITY_NONE)
    return false;

  cout << "Trying guessNoBorders\n";

  // We will not test for symmetry.
  const CarPeaksPtr& peaksBefore = carBeforePtr->getPeaksPtr();
  const CarPeaksPtr& peaksAfter = carAfterPtr->getPeaksPtr();

  const unsigned avgLeftLeft = 
    (peaksBefore.firstBogieLeftPtr->getIndex() +
     peaksAfter.firstBogieLeftPtr->getIndex()) / 2;
  const unsigned avgLeftRight =
    (peaksBefore.firstBogieRightPtr->getIndex() +
     peaksAfter.firstBogieRightPtr->getIndex()) / 2;
  const unsigned avgRightLeft =
    (peaksBefore.secondBogieLeftPtr->getIndex() +
     peaksAfter.secondBogieLeftPtr->getIndex()) / 2;
  const unsigned avgRightRight =
    (peaksBefore.secondBogieRightPtr->getIndex() +
     peaksAfter.secondBogieRightPtr->getIndex()) / 2;

  // Disqualify if the resulting car is implausible.
  if (avgLeftLeft < peaksBefore.secondBogieRightPtr->getIndex())
    return false;

  if (avgRightRight > peaksAfter.firstBogieLeftPtr->getIndex())
    return false;

  const unsigned delta = 
    avgLeftLeft - peaksBefore.secondBogieRightPtr->getIndex();

  const unsigned bogieGap = carBeforePtr->getLeftBogieGap();

  // It is implausible for the intra-car gap to be too large.
  if (delta > NO_BORDER_FACTOR * bogieGap)
    return false;

  // It is implausible for the intra-car gap to be tiny.
  if (delta < SMALL_BORDER_FACTOR * bogieGap)
    return false;

  // It is implausible for the intra-car gap to exceed a mid gap.
  const unsigned mid = carBeforePtr->getMidGap();
  if (delta > mid)
    return false;

  targets.clear();
  targets.emplace_back(Target());
  Target& target = targets.back();

  const unsigned start = avgLeftLeft - delta/2;
  const unsigned end =
    (peaksAfter.firstBogieLeftPtr->getIndex() - avgRightRight) / 2;

  list<unsigned> carPoints;
  carPoints.push_back(0);
  carPoints.push_back(avgLeftLeft - start);
  carPoints.push_back(avgLeftRight - start);
  carPoints.push_back(avgRightLeft - start);
  carPoints.push_back(avgRightRight - start);
  carPoints.push_back(end - start);

  target.fill(
    carBeforePtr->index(),
    1,
    false,
    start,
    end,
    BORDERS_NONE,
    carPoints);
    
  return true;
}


bool PeakPattern::guessBothSingle(const CarModels& models)
{
  if (rangeData.qualBest == QUALITY_NONE)
    return false;

  cout << "Trying guessBothSingle\n";
  targets.clear();

  for (auto& ae: activeEntries)
  {
    if (! ae.fullFlag)
      continue;

    if (rangeData.lenRange >= ae.lenLo[rangeData.qualBest] &&
        rangeData.lenRange <= ae.lenHi[rangeData.qualBest])
    {
      PeakPattern::addModelTargets(models, ae.index, 
        ae.data->symmetryFlag, BORDERS_DOUBLE_SIDED_SINGLE);
    }
  }

  return (! targets.empty());
}


bool PeakPattern::guessBothSingleShort()
{
  if (rangeData.qualLeft == QUALITY_NONE)
    return false;

  cout << "Trying guessBothSingleShort\n";
  targets.clear();

  unsigned lenTypical = 2 * (sideTypical + bogieTypical) + longTypical;
  unsigned lenShortLo = static_cast<unsigned>(SHORT_FACTOR * lenTypical);
  unsigned lenShortHi = static_cast<unsigned>(LONG_FACTOR * lenTypical);

  if (rangeData.lenRange < lenShortLo ||
      rangeData.lenRange > lenShortHi)
    return false;

  Target target;

  // Guess that particularly the middle part is shorter in a short car.
  list<unsigned> carPoints;
  carPoints.push_back(0);
  carPoints.push_back(sideTypical);
  carPoints.push_back(sideTypical + bogieTypical);
  carPoints.push_back(rangeData.lenRange - sideTypical - bogieTypical);
  carPoints.push_back(rangeData.lenRange - sideTypical);
  carPoints.push_back(rangeData.lenRange);

  if (target.fill(
    0, // Doesn't matter
    1,
    false,
    rangeData.indexLeft,
    rangeData.indexRight,
    BORDERS_DOUBLE_SIDED_SINGLE_SHORT,
    carPoints))
  {
    targets.emplace_back(target);
  }

  return (! targets.empty());
}


bool PeakPattern::guessBothDouble(
  const CarModels& models,
  const bool leftFlag)
{
  if (rangeData.qualBest == QUALITY_NONE)
    return false;

  cout << "Trying guessBothDouble\n";

  targets.clear();

  for (auto& ae1: activeEntries)
  {
    if (! ae1.fullFlag)
      continue;

    for (auto& ae2: activeEntries)
    {
      if (! ae2.fullFlag)
        continue;

      if (rangeData.lenRange >= 
          ae1.lenLo[rangeData.qualBest] + ae2.lenLo[rangeData.qualBest] &&
          rangeData.lenRange <= 
          ae1.lenHi[rangeData.qualBest] + ae2.lenHi[rangeData.qualBest])
      {
        if (leftFlag)
          PeakPattern::addModelTargets(models, ae1.index, 
            ae1.data->symmetryFlag, 
            BORDERS_DOUBLE_SIDED_DOUBLE);
        else
          PeakPattern::addModelTargets(models, ae2.index, 
            ae2.data->symmetryFlag, 
            BORDERS_DOUBLE_SIDED_DOUBLE);
      }
    }
  }

  return (! targets.empty());
}

bool PeakPattern::guessBothDoubleLeft(const CarModels& models)
{
  return PeakPattern::guessBothDouble(models, true);
}


bool PeakPattern::guessBothDoubleRight(const CarModels& models)
{
  return PeakPattern::guessBothDouble(models, false);
}


bool PeakPattern::guessLeft(const CarModels& models)
{
  // Starting from the left, so open towards the end.

  if (carBeforePtr == nullptr)
    return false;

  if (rangeData.qualLeft == QUALITY_NONE)
    return false;

  cout << "Trying guessLeft\n";

  targets.clear();

  for (auto& ae: activeEntries)
  {
    PeakPattern::addModelTargets(models, ae.index, ae.data->symmetryFlag,
      BORDERS_SINGLE_SIDED_LEFT);
  }

  return (! targets.empty());
}


bool PeakPattern::guessRight(const CarModels& models)
{
  // Starting from the right, so open towards the beginning.

  if (carAfterPtr == nullptr)
    return false;

  if (rangeData.qualRight == QUALITY_NONE)
    return false;

  if (rangeData.gapRight >= 
      carAfterPtr->getPeaksPtr().firstBogieLeftPtr->getIndex())
    return false;

  targets.clear();

  cout << "Trying guessRight\n";

  for (auto& ae: activeEntries)
  {
    PeakPattern::addModelTargets(models, ae.index, ae.data->symmetryFlag,
      BORDERS_SINGLE_SIDED_RIGHT);
  }

  return (! targets.empty());
}


bool PeakPattern::looksEmptyFirst(const PeakPtrs& peakPtrsUsed) const
{
  if (rangeData.qualRight == QUALITY_NONE ||
      rangeData.qualLeft != QUALITY_NONE)
    return false;

  cout << "Trying looksEmptyFirst\n";

  if (peakPtrsUsed.size() >= 2)
    return false;

  Peak const * pptr = peakPtrsUsed.front();
  if (pptr->goodQuality())
    return false;

  // This could become more elaborate, e.g. the peak is roughly
  // in an expected position.

  return true;
}


void PeakPattern::updateUnused(
  const Target& target,
  PeakPtrs& peakPtrsUnused) const
{
  unsigned limitLower, limitUpper;
  target.limits(limitLower, limitUpper);
  if (limitLower)
    peakPtrsUnused.erase_below(limitLower);
  if (limitUpper)
    peakPtrsUnused.erase_above(limitUpper);
}


void PeakPattern::updateUnused(
  const unsigned limitLower,
  const unsigned limitUpper,
  PeakPtrs& peakPtrsUnused) const
{
  if (limitLower)
    peakPtrsUnused.erase_below(limitLower);
  if (limitUpper)
    peakPtrsUnused.erase_above(limitUpper);
}


void PeakPattern::updateUsed(
  const vector<Peak const *>& peaksClosest,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  // There may be some peaks that have to be moved.
  auto pu = peakPtrsUsed.begin();
  for (auto& peak: peaksClosest)
  {
    if (peak == nullptr)
      continue;

    const unsigned indexClose = peak->getIndex();
    while (pu != peakPtrsUsed.end() && (* pu)->getIndex() < indexClose)
    {
      peakPtrsUnused.add(* pu);
      pu = peakPtrsUsed.erase(pu);
    }

    if (pu == peakPtrsUsed.end())
      break;

    // Preserve those peaks that are also in peaksClose.
    if ((* pu)->getIndex() == indexClose)
      pu = peakPtrsUsed.next(pu);
  }

  // Erase trailing peaks.
  while (pu != peakPtrsUsed.end())
  {
    peakPtrsUnused.add(* pu);
    pu = peakPtrsUsed.erase(pu);
  }
}


void PeakPattern::update(
  const NoneEntry& none,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  PeakPattern::updateUsed(none.peaksClose, peakPtrsUsed, peakPtrsUnused);
  PeakPattern::updateUnused(none.pe, peakPtrsUnused);
}


void PeakPattern::update(
  const vector<Peak const *>& closest,
  const unsigned limitLower,
  const unsigned limitUpper,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  PeakPattern::updateUsed(closest, peakPtrsUsed, peakPtrsUnused);

  PeakPattern::updateUnused(limitLower, limitUpper, peakPtrsUnused);
}


void PeakPattern::targetsToCompletions(const PeakPtrs& peakPtrsUsed)
{
  completions.reset();

  vector<Peak const *> peaksClose;
  unsigned numClose;
  unsigned dist;

  for (auto& target: targets)
  {
    peakPtrsUsed.getClosest(target.indices(), peaksClose, numClose, dist);

    cout << PeakPattern::strClosest(peaksClose, target.indices());

    if (numClose == 0)
      continue;

    unsigned limitLower, limitUpper;
    target.limits(limitLower, limitUpper);

    MissCar& miss = completions.emplace_back(dist);
    miss.setLimits(limitLower, limitUpper);

    const unsigned bogieTolerance = 3 * target.bogieGap() / 10;

    for (unsigned i = 0; i < peaksClose.size(); i++)
    {
      if (peaksClose[i] == nullptr)
        miss.addMiss(target.index(i), bogieTolerance);
      else
        miss.addMatch(target.index(i), peaksClose[i]);
    }
  }
}


// TODO =========================== Move to Completions
//
/*
void PeakPattern::readjust(list<SingleEntry>& singles)
{
  // This applies for a short car where we got three peaks.
  // Probably the fourth peak is at a predictable bogie distance
  // which we can better estimate now.

  if (targets.size() != 1 || singles.size() != 1)
    return;

  unsigned p, target;
  if (peaksClose[0] == nullptr)
  {
    p = 0;
    target = peaksClose[1]->getIndex() -
      (peaksClose[3]->getIndex() - peaksClose[2]->getIndex());
  }
  else if (peaksClose[1] == nullptr)
  {
    p = 1;
    target = peaksClose[0]->getIndex() +
      (peaksClose[3]->getIndex() - peaksClose[2]->getIndex());
  }
  else if (peaksClose[2] == nullptr)
  {
    p = 2;
    target = peaksClose[3]->getIndex() -
      (peaksClose[1]->getIndex() - peaksClose[0]->getIndex());
  }
  else if (peaksClose[3] == nullptr)
  {
    p = 3;
    target = peaksClose[2]->getIndex() +
      (peaksClose[1]->getIndex() - peaksClose[0]->getIndex());
  }
  else
    return;

  SingleEntry& single = singles.front();
  const unsigned delta = target - single.target;

cout << "Adjusted " << p << " goal from " << 
  single.target + offset << " to " <<
  target + offset << " (" << single.lower + offset + delta << " - " << 
  single.upper + offset + delta << ")\n";

  single.target = target;
  single.lower += delta;
  single.upper += delta;

  targets.front().revise(p, target);
}
*/


void PeakPattern::annotateCompletions(
  PeakPool& peaks,
  PeakPtrs& peakPtrsUnused,
  const bool forceFlag)
{
  // Mark up with relevant, unused peaks.
  for (auto pptr: peakPtrsUnused)
    completions.markWith(* pptr, MISS_UNUSED);

  // Mark up with repairable peaks.
  completions.makeRepairables();

  Peak peakRep;
  while (completions.nextRepairable(peakRep))
  {
    unsigned testIndex;
    cout << "TRYING " << peakRep.strQuality(offset);
    peaks.repair(peakRep, &Peak::borderlineQuality, offset,
      true, forceFlag, testIndex);

    if (testIndex > 0)
    {
      cout << "REPAIRABLE" << endl;
      peakRep.logPosition(testIndex, testIndex, testIndex);
      completions.markWith(peakRep, MISS_REPAIRABLE);
    }
  }

  cout << "Completions after mark-up\n\n";
  cout << completions.str(offset);
}


void PeakPattern::fillCompletions(
  PeakPool& peaks,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused,
  const bool forceFlag)
{
  Peak peakRep;

  for (auto& misses: completions)
  {
    for (auto& miss: misses)
    {
      if (miss.source() == MISS_UNUSED)
      {
        cout << "Reviving unused " << miss.str(offset) << endl;;
        Peak * ptr = miss.ptr();
        peakPtrsUsed.add(ptr);
        peakPtrsUnused.remove(ptr);
      }
      else if (miss.source() == MISS_REPAIRABLE)
      {
        cout << "Repairing unused " << miss.str(offset) << endl;
        unsigned testIndex;
        miss.fill(peakRep);
        Peak * ptr = peaks.repair(peakRep, &Peak::borderlineQuality, 
          offset, false, forceFlag, testIndex);

        if (ptr)
        {
          peakPtrsUsed.add(ptr);
          cout << peakPtrsUsed.strQuality("Used now", offset);

          completions.markWith(* ptr, MISS_REPAIRED);
        }
        else
          THROW(ERR_ALGO_PEAK_CONSISTENCY, "Not repairable after all?");
      }
    }
  }
}


bool PeakPattern::fix(
  PeakPool& peaks,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused,
  const bool forceFlag,
  const bool flexibleFlag)
{
  if (targets.empty())
    return false;

  PeakPattern::targetsToCompletions(peakPtrsUsed);

  PeakPattern::annotateCompletions(peaks, peakPtrsUnused, forceFlag);

  PeakPattern::fillCompletions(peaks, peakPtrsUsed, peakPtrsUnused, 
    forceFlag);

  completions.condense();

  cout << "Completions after filling out\n\n";
  cout << completions.str(offset);

  MissCar * winnerPtr;
  const unsigned numComplete = completions.numComplete(winnerPtr);

  vector<Peak const *>* closestPtr;
  unsigned limitLower, limitUpper;

  if (numComplete == 1)
  {
    cout << "COMPLETION MATCH" << endl;

    limitLower = 0;
    limitUpper = 0;
    winnerPtr->getMatch(closestPtr, limitLower, limitUpper);

    PeakPattern::update(* closestPtr, limitLower, limitUpper, 
      peakPtrsUsed, peakPtrsUnused);

    return true;
  }
  else if (numComplete > 1)
  {
    cout << "COMPLETION ABUNDANCE " << numComplete << endl;
  }
  else
  {
    cout << "COMPLETION MISS" << endl;
  }

//  TODO Put this in MissCar.
UNUSED(flexibleFlag);
return false;

}


bool PeakPattern::locate(
  const CarModels& models,
  PeakPool& peaks,
  const PeakRange& range,
  const unsigned offsetIn,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused)
{
  if (! PeakPattern::setGlobals(models, range, offsetIn))
    return false;

  cout << peakPtrsUsed.strQuality("Used", offset);
  cout << peakPtrsUnused.strQuality("Unused", offset);

  PeakPattern::getActiveModels(models);

  // TODO A lot of these seem to be misalignments of cars with peaks.
  // So it's not clear that we should recognize them.
  if (PeakPattern::guessNoBorders())
  {
    if (PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused))
      return true;
  }

  if (PeakPattern::guessBothSingle(models))
  {
    // Single car whose length fits well.  We should never fail.
    if (PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused, true))
      return true;
    // else
      // return false;
  }

  if (PeakPattern::guessBothDoubleLeft(models))
  {
    // The left car of a two-car combination that might fit.
    if (PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused))
      return true;
  }

  if (PeakPattern::guessBothDoubleRight(models))
  {
    // The right car of a two-car combination that might fit.
    if (PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused))
      return true;
  }

  if (PeakPattern::guessBothSingleShort())
  {
    // Not model-based, more heuristic.
    if (PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused, true))
      return true;
  }

  if (PeakPattern::guessLeft(models))
  {
    // Fill up with known models from the left.
    if (PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused))
      return true;
  }

  if (PeakPattern::guessRight(models))
  {
    // Fill up with known models from the right.
    if (PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused))
      return true;
  }

  if (PeakPattern::looksEmptyFirst(peakPtrsUsed))
  {
    peakPtrsUnused.moveFrom(peakPtrsUsed);
    return true;
  }

  return false;
}

string PeakPattern::strClosest(
  vector<Peak const *>& peaksClose,
  const vector<unsigned>& indices) const
{
  stringstream ss;
  ss << "Closest indices\n";
  for (unsigned i = 0; i < indices.size(); i++)
  {
    string str;
    if (peaksClose[i])
      str = to_string((peaksClose[i]->getIndex()) + offset);
    else
      str = "-";

    ss << setw(4) << left << i <<
      setw(8) << right << indices[i] + offset <<
      setw(8) << str << endl;
  }
  return ss.str();
}

