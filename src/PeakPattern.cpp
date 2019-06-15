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


void PeakPattern::getActiveModels(
  const CarModels& models,
  const bool fullFlag)
{
  activeEntries.clear();

  for (unsigned index = 0; index < models.size(); index++)
  {
    if (models.empty(index))
      continue;

    ModelData const * data = models.getData(index);
    if (! fullFlag)
    {
      if (data->containedFlag)
        continue;
    }
    else if (! data->fullFlag)
      continue;

    activeEntries.emplace_back(ActiveEntry());
    ActiveEntry& ae = activeEntries.back();
    ae.data = data;
    ae.index = index;

// cout << "getActiveModels: Got index " << index << endl;
// cout << data->str() << endl;

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

    ae.lenLo[QUALITY_WHOLE_MODEL] =
      static_cast<unsigned>((1.f - LEN_FACTOR_GREAT) * len);
    ae.lenHi[QUALITY_WHOLE_MODEL] =
      static_cast<unsigned>((1.f + LEN_FACTOR_GREAT) * len);

    ae.lenLo[QUALITY_SYMMETRY] = 
      static_cast<unsigned>((1.f - LEN_FACTOR_GOOD) * len);
    ae.lenHi[QUALITY_SYMMETRY] =
      static_cast<unsigned>((1.f + LEN_FACTOR_GOOD) * len);
  }
// cout << "DONE getActive" << endl;
}


bool PeakPattern::fillFromModel(
  const CarModels& models,
  const unsigned indexModel,
  const bool symmetryFlag,
  const unsigned indexRangeLeft,
  const unsigned indexRangeRight,
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
        indexRangeLeft, indexRangeRight, patternType, carPoints))
    {
      targets.emplace_back(target);
      seenFlag = true;
    }
  }

  if (! symmetryFlag && car.hasRightGap())
  {
    if (target.fill(indexModel, weight, true, 
        indexRangeLeft, indexRangeRight, patternType, carPoints))
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
  cout << "Trying guessBothSingle\n";

  for (auto& ae: activeEntries)
  {
    if (rangeData.lenRange >= ae.lenLo[rangeData.qualBest] &&
        rangeData.lenRange <= ae.lenHi[rangeData.qualBest])
    {
      PeakPattern::fillFromModel(models, ae.index, 
        ae.data->symmetryFlag, rangeData.indexLeft, rangeData.indexRight, 
        BORDERS_DOUBLE_SIDED_SINGLE);
    }
  }

  return (! targets.empty());
}


bool PeakPattern::guessBothSingleShort()
{
  if (rangeData.qualLeft == QUALITY_GENERAL || 
      rangeData.qualLeft == QUALITY_NONE)
    return false;

  cout << "Trying guessBothSingleShort\n";

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
  cout << "Trying guessBothDouble\n";

  targets.clear();

  for (auto& ae1: activeEntries)
  {
    for (auto& ae2: activeEntries)
    {
      if (rangeData.lenRange >= 
          ae1.lenLo[rangeData.qualBest] + ae2.lenLo[rangeData.qualBest] &&
          rangeData.lenRange <= 
          ae1.lenHi[rangeData.qualBest] + ae2.lenHi[rangeData.qualBest])
      {
        if (leftFlag)
          PeakPattern::fillFromModel(models, ae1.index, 
            ae1.data->symmetryFlag, 
            rangeData.indexLeft, rangeData.indexRight, 
            BORDERS_DOUBLE_SIDED_DOUBLE);
        else
          PeakPattern::fillFromModel(models, ae2.index, 
            ae2.data->symmetryFlag, 
            rangeData.indexLeft, rangeData.indexRight, 
            BORDERS_DOUBLE_SIDED_DOUBLE);
      }
    }
  }

  return (! targets.empty());
}


bool PeakPattern::guessLeft(const CarModels& models)
{
  // Starting from the left, so open towards the end.

  if (carBeforePtr == nullptr)
    return false;

  if (rangeData.qualLeft == QUALITY_GENERAL || 
      rangeData.qualLeft == QUALITY_NONE)
    return false;

  cout << "Trying guessLeft\n";

  targets.clear();

  for (auto& ae: activeEntries)
  {
    PeakPattern::fillFromModel(models, ae.index, ae.data->symmetryFlag,
      rangeData.indexLeft, rangeData.indexRight, BORDERS_SINGLE_SIDED_LEFT);
  }

  return (! targets.empty());
}


bool PeakPattern::guessRight(const CarModels& models)
{
  // Starting from the right, so open towards the beginning.

  if (carAfterPtr == nullptr)
    return false;

  if (rangeData.qualRight == QUALITY_GENERAL || 
      rangeData.qualRight == QUALITY_NONE)
    return false;

  if (rangeData.gapRight >= 
      carAfterPtr->getPeaksPtr().firstBogieLeftPtr->getIndex())
    return false;

  targets.clear();

  cout << "Trying guessRight\n";

  for (auto& ae: activeEntries)
  {
    PeakPattern::fillFromModel(models, ae.index, ae.data->symmetryFlag,
      rangeData.indexLeft, rangeData.indexRight, BORDERS_SINGLE_SIDED_RIGHT);
  }

  return (! targets.empty());
}


bool PeakPattern::looksEmptyFirst(const PeakPtrs& peakPtrsUsed) const
{
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


void PeakPattern::addToSingles(const vector<unsigned>& indices)
{
  // We rely heavily on having exactly one nullptr.
  // Singles are generally quite likely, so we permit more range.
  const unsigned bogieThird =
    (indices[3] - indices[2] + indices[1] - indices[0]) / 6;

  MissCar& miss = completions.back();

  for (unsigned i = 0; i < peaksClose.size(); i++)
  {
    if (peaksClose[i] == nullptr)
      miss.add(indices[i], bogieThird);
  }
}


bool PeakPattern::checkDoubles(const Target& target) const
{
  // TODO Can make simpler?
  unsigned limitLower, limitUpper;
  target.limits(limitLower, limitUpper);
  if (limitLower && peaksClose[2] == nullptr && peaksClose[3] == nullptr)
  {
    // Don't look open-ended from the left when the right bogie has
    // no match at all.
    return false;
  }

  if (limitUpper && peaksClose[0] == nullptr && peaksClose[1] == nullptr)
  {
    // Don't look open-ended from the right when the left bogie has
    // no match at all.
    return false;
  }
  return true;
}


void PeakPattern::addToDoubles(const Target& target)
{
  // We rely heavily on having exactly two nullptrs.
  const unsigned bogieQuarter =
    (target.index(3) - target.index(2) + 
     target.index(1) - target.index(0)) / 8;

  MissCar& miss = completions.back();
  for (unsigned i = 0; i < peaksClose.size(); i++)
  {
    if (peaksClose[i] == nullptr)
      miss.add(target.index(i), bogieQuarter);
  }
}


void PeakPattern::addToTriples(const Target& pe)
{
  const unsigned bogieQuarter = bogieTypical / 4;

  MissCar& miss = completions.back();
  for (unsigned i = 0; i < peaksClose.size(); i++)
  {
    if (peaksClose[i] == nullptr)
    {
      miss.add(pe.index(i), bogieQuarter);
    }
  }
}


void PeakPattern::examineTargets(
  const PeakPtrs& peakPtrsUsed,
  NoneEntry& none)
  // list<SingleEntry>& singles,
  // list<DoubleEntry>& doubles,
  // list<TripleEntry>& triples)
{
UNUSED(none);

  // Get the lie of the land.
  unsigned distBest = numeric_limits<unsigned>::max();
  // singles.clear();
  // doubles.clear();
  // triples.clear();
  completions.reset();

  for (auto target = targets.begin(); target != targets.end(); )
  {
    unsigned numClose;
    unsigned dist;

    peakPtrsUsed.getClosest(target->indices(), peaksClose, numClose, dist);

    cout << PeakPattern::strClosest(target->indices());

    if (numClose == 4)
    {
      // Here we take the best one, whereas we take a more democratic
      // approach for 2's and 3's.
      if (dist < distBest)
      {
cout << "PERFECT MATCH\n";
        distBest = dist;
        MissCar& miss = completions.emplace_back(dist);

        unsigned limitLower, limitUpper;
        target->limits(limitLower, limitUpper);
        miss.setMatch(peaksClose, limitLower, limitUpper);
      }
    }
    else if (numClose == 3)
    {
      MissCar& miss = completions.emplace_back(dist);

      unsigned limitLower, limitUpper;
      target->limits(limitLower, limitUpper);
      miss.setMatch(peaksClose, limitLower, limitUpper);

      PeakPattern::addToSingles(target->indices());
    }
    else if (numClose == 2)
    {
      if (PeakPattern::checkDoubles(* target))
      {
        MissCar& miss = completions.emplace_back(dist);

        unsigned limitLower, limitUpper;
        target->limits(limitLower, limitUpper);
        miss.setMatch(peaksClose, limitLower, limitUpper);

        PeakPattern::addToDoubles(* target);
      }
    }
    else if (numClose == 1)
    {
      MissCar& miss = completions.emplace_back(dist);

      unsigned limitLower, limitUpper;
      target->limits(limitLower, limitUpper);
      miss.setMatch(peaksClose, limitLower, limitUpper);

      PeakPattern::addToTriples(* target);
    }
    else if (numClose == 0)
    {
      // Get rid of the 1's and 0's.
      target = targets.erase(target);
      continue;
    }

    target++;
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


void PeakPattern::processMessage(
  const string& origin,
  const string& verb,
  const unsigned target,
  Peak *& pptr) const
{
  if (pptr == nullptr)
  {
    cout << origin << ": Failed target " << verb << " " <<
      target + offset << "\n";
  }
  else
  {
    cout << origin << ": Target " << verb << " " <<
      target + offset << " to " << pptr->getIndex() + offset << endl;
  }
}


/*
void PeakPattern::reviveOnePeak(
  const string& text,
  const SingleEntry& single,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused,
  Peak *& pptr) const
{
  unsigned indexUsed;

  // Peak may have been revived once already, in which case we shouldn't
  // repeat it.
  pptr = peakPtrsUsed.locate(single.lower, single.upper, single.target,
    &Peak::borderlineQuality, indexUsed);
  
  if (pptr)
  {
    PeakPattern::processMessage(text, "revive from used", single.target, pptr);
    return;
  }

  pptr = peakPtrsUnused.locate(single.lower, single.upper, single.target,
    &Peak::borderlineQuality, indexUsed);

  PeakPattern::processMessage(text, "revive from unused", single.target, pptr);
}


void PeakPattern::fixOnePeak(
  const string& text,
  const SingleEntry& single,
  PeakPool& peaks,
  Peak *& pptr,
  const bool forceFlag) const
{
  Peak peakHint;
  peakHint.logPosition(single.target, single.lower, single.upper);

  unsigned testIndex;
  pptr = peaks.repair(peakHint, &Peak::borderlineQuality, offset,
    false, forceFlag, testIndex);

  PeakPattern::processMessage(text, "repair", single.target, pptr);
}


void PeakPattern::processOnePeak(
  const string& origin,
  const SingleEntry& single,
PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused,
  PeakPool& peaks,
  Peak *& pptr,
  const bool forceFlag) const
{
  PeakPattern::reviveOnePeak(origin, single, peakPtrsUsed, peakPtrsUnused, pptr);

  if (pptr == nullptr)
  {
    PeakPattern::fixOnePeak(origin, single, peaks, pptr, forceFlag);
  }
}
*/


bool PeakPattern::fix(
  PeakPool& peaks,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused,
  const bool forceFlag,
  const bool flexibleFlag)
{
  if (targets.empty())
    return false;

  NoneEntry none;
  // list<SingleEntry> singles;
  // list<DoubleEntry> doubles;
  // list<TripleEntry> triples;

  PeakPattern::examineTargets(peakPtrsUsed, none);
    // none, singles, doubles, triples);

  // Fill out with relevant, unused peaks.
  for (auto pptr: peakPtrsUnused)
    completions.markWith(* pptr, MISS_UNUSED);

  // Then fill out with repairable peaks.
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

  cout << "Completions before filling out\n\n";
  cout << completions.str(offset);

  vector<Peak const *>* closestPtr;
  unsigned limitLower, limitUpper;

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
        {
          THROW(ERR_ALGO_PEAK_CONSISTENCY, "Not repairable after all?");
        }
      }
    }
  }

  MissCar * winnerPtr;

  completions.condense();
  const unsigned numComplete = completions.numComplete(winnerPtr);

  cout << "Completions after filling out\n\n";
  cout << completions.str(offset);

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


void PeakPattern::getSpacings(PeakPtrs& peakPtrsUsed)
{
  for (auto pit = peakPtrsUsed.begin(); pit != prev(peakPtrsUsed.end()); 
      pit++)
  {
    spacings.emplace_back(SpacingEntry());
    SpacingEntry& se = spacings.back();

    se.peakLeft = * pit;
    se.peakRight = * next(pit);
    se.dist = se.peakRight->getIndex() - se.peakLeft->getIndex();

    se.numBogies = (bogieTypical == 0 ? 1.f : 
      se.dist / static_cast<float>(bogieTypical));
    // TODO #define
    se.bogieLikeFlag = (se.numBogies >= 0.7f && se.numBogies <= 1.75f);

    const PeakQuality pqWL = se.peakLeft->getQualityWhole();
    const PeakQuality pqWR = se.peakRight->getQualityWhole();
    se.qualityWholeLower = (pqWL >= pqWR ? pqWL : pqWR);

    const PeakQuality pqSL = se.peakLeft->getQualityShape();
    const PeakQuality pqSR = se.peakRight->getQualityShape();
    se.qualityShapeLower = (pqSL >= pqSR ? pqSL : pqSR);

  }
}


bool PeakPattern::plausibleCar(
  const bool leftFlag,
  const unsigned index1,
  const unsigned index2) const
{
  const float bogieRatio = 
    spacings[index1].numBogies / spacings[index2].numBogies;
  if (bogieRatio < 0.67f || bogieRatio > 1.5f)
  {
cout << "FAIL on bogie ratio: " << bogieRatio << "\n";
    return false;
  }

  const float bogieAvg = static_cast<float>(
    (spacings[index1].dist + spacings[index2].dist)) / 2.f;
  const unsigned mid = spacings[index2].peakLeft->getIndex() -
    spacings[index1].peakRight->getIndex();
  const float midRatio = static_cast<float>(mid) / bogieAvg;
  if (midRatio < 1.2f || midRatio > 10.f)
  {
cout << "FAIL on mid ratio\n";
    return false;
  }

  unsigned gap;
  if (leftFlag)
    gap = spacings[index1].peakLeft->getIndex() - rangeData.indexLeft;
  else
    gap = rangeData.indexRight - spacings[index2].peakRight->getIndex();
  const float gapRatio = static_cast<float>(gap) / bogieAvg;
  if (gapRatio < 0.3f || gapRatio > 2.f)
  {
cout << "FAIL on gap ratio: gap " << gap << endl;
    return false;
  }

  // TODO Also check quality in some cases
  return true;
}


void PeakPattern::fixShort(
  const string& text,
  const unsigned indexFirst,
  const unsigned indexLast,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  NoneEntry none;
  none.peaksClose.push_back(spacings[indexFirst].peakLeft);
  none.peaksClose.push_back(spacings[indexFirst].peakRight);
  none.peaksClose.push_back(spacings[indexLast].peakLeft);
  none.peaksClose.push_back(spacings[indexLast].peakRight);

  BordersType border;

  if (rangeData.gapLeft)
  {
    border = (rangeData.gapRight ?
      BORDERS_DOUBLE_SIDED_SINGLE_SHORT :
      BORDERS_SINGLE_SIDED_LEFT);
  }
  else
    border = BORDERS_SINGLE_SIDED_RIGHT;

  list<unsigned> carPoints;

  none.pe.fill(
    0, // Doesn't matter
    1, 
    false, // Doesn't matter
    spacings[indexFirst].peakLeft->getIndex(),
    spacings[indexLast].peakRight->getIndex(),
    border,
    carPoints); // Doesn't matter

cout << text << ": " <<
  none.peaksClose[0]->getIndex() + offset << ", " <<
  none.peaksClose[1]->getIndex() + offset << ", " <<
  none.peaksClose[2]->getIndex() + offset << ", " <<
  none.peaksClose[3]->getIndex() + offset << endl;

  PeakPattern::update(none, peakPtrsUsed, peakPtrsUnused);
}


bool PeakPattern::guessAndFixShortFromSpacings(
  const string& text,
  const bool leftFlag,
  const unsigned indexFirst,
  const unsigned indexLast,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  const SpacingEntry& spFirst = spacings[indexFirst];
  const SpacingEntry& spLast = spacings[indexLast];

  if (spFirst.bogieLikeFlag &&
      spLast.bogieLikeFlag &&
      spFirst.qualityWholeLower <= PEAK_QUALITY_GOOD &&
      spLast.qualityWholeLower <= PEAK_QUALITY_GOOD &&
      PeakPattern::plausibleCar(leftFlag, indexFirst, indexLast))
  {
    PeakPattern::fixShort(text, indexFirst, indexLast,
      peakPtrsUsed, peakPtrsUnused);
    return true;
  }
  else
    return false;
}


bool PeakPattern::guessAndFixShort(
  const bool leftFlag,
  const unsigned indexFirst,
  const unsigned indexLast,
  PeakPool& peaks,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  const unsigned numSpaces = indexLast + 1 - indexFirst;
cout << "NUMSPACES " << numSpaces << endl;

  const SpacingEntry& spFirst = spacings[indexFirst];
  const SpacingEntry& spLast = spacings[indexLast];

  if (numSpaces == 2)
  {
    // TODO Could extend
    /*
    if (rangeData.qualWorst != QUALITY_GENERAL && 
        rangeData.qualWorst != QUALITY_NONE)
    {
      cout << "PPDOUBLE 2\n";
    }
    else
    {
      cout << "PPSINGLE 2\n";
    }
    */
  }
  else if (numSpaces == 3)
  {
    // Could simply be one car.
    return PeakPattern::guessAndFixShortFromSpacings("PPINDEX 3",
      leftFlag, indexFirst, indexLast, peakPtrsUsed, peakPtrsUnused);
  }
  else if (numSpaces == 4)
  {
    // There could be one spurious peak in the middle.
    if (spFirst.bogieLikeFlag &&
        spLast.bogieLikeFlag)
    {
      if (spFirst.qualityWholeLower <= PEAK_QUALITY_GOOD &&
          spLast.qualityWholeLower <= PEAK_QUALITY_GOOD &&
          PeakPattern::plausibleCar(leftFlag, indexFirst, indexLast))
      {
        PeakPattern::fixShort("PPINDEX 4a", indexFirst, indexLast,
          peakPtrsUsed, peakPtrsUnused);
        return true;
      }
      else if (spFirst.qualityShapeLower <= PEAK_QUALITY_GOOD &&
          spLast.qualityShapeLower <= PEAK_QUALITY_GOOD &&
          PeakPattern::plausibleCar(leftFlag, indexFirst, indexLast))
      {
        PeakPattern::fixShort("PPINDEX 4b", indexFirst, indexLast,
          peakPtrsUsed, peakPtrsUnused);
        return true;
      }
    }
  }
  else if (numSpaces == 5)
  {
cout << "TRYING RIGHT5\n";
cout << "LEFT " << leftFlag << "\n";
    if (leftFlag)
    {
cout << indexFirst << " " << indexLast-2 << "\n";
      // There could be a simple car at the left end.
      return PeakPattern::guessAndFixShortFromSpacings("PPINDEX 4c",
        leftFlag, indexFirst, indexLast-2, peakPtrsUsed, peakPtrsUnused);
    }
    else
    {
cout << indexFirst+2 << " " << indexLast << "\n";
      // There could be a simple car at the right end.
      return PeakPattern::guessAndFixShortFromSpacings("PPINDEX 4d",
        leftFlag, indexFirst+2, indexLast, peakPtrsUsed, peakPtrsUnused);
    }
  }

  UNUSED(peaks);

  return false;
}


bool PeakPattern::guessAndFixShortLeft(
  PeakPool& peaks,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  if (rangeData.qualLeft == QUALITY_NONE)
    return false;

  cout << "Trying guessAndFixShortLeft\n";

  const unsigned ss = spacings.size();
  return PeakPattern::guessAndFixShort(
    true, 0, (ss > 4 ? 4 : ss-1), peaks, peakPtrsUsed, peakPtrsUnused);
}


bool PeakPattern::guessAndFixShortRight(
  PeakPool& peaks,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  if (rangeData.qualRight == QUALITY_NONE || 
      (rangeData.qualLeft != QUALITY_NONE && peakPtrsUsed.size() <= 6))
    return false;

  cout << "Trying guessAndFixShortRight\n";

  const unsigned ss = spacings.size();
  return PeakPattern::guessAndFixShort(
    false, (ss <= 5 ? 0 : ss-5), ss-1, peaks, peakPtrsUsed, peakPtrsUnused);
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

  if (rangeData.qualLeft == QUALITY_NONE && 
      rangeData.qualRight == QUALITY_NONE)
  {
  // TODO A lot of these seem to be misalignments of cars with peaks.
  // So it's not clear that we should recognize them.
    if (PeakPattern::guessNoBorders() &&
        PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused))
      return true;
    else
      return false;
  }

cout << peakPtrsUsed.strQuality("Used", offset);
cout << peakPtrsUnused.strQuality("Unused", offset);

  // First try filling the entire range with 1-2 cars.
  if (rangeData.qualBest != QUALITY_GENERAL && 
      rangeData.qualBest != QUALITY_NONE)
  {
    PeakPattern::getActiveModels(models, true);

    // Note that guessBothDouble(true) could generate candidates
    // which aren't fitted.  So we can't return the fix value no
    // matter what, and we only return true if it fits.

    // The short one is not model-based, but it fits well here.
    if (PeakPattern::guessBothSingle(models))
      return PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused, true);
    else if (PeakPattern::guessBothSingleShort() &&
        PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused, true))
      return true;
    else if (PeakPattern::guessBothDouble(models, true) &&
        PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused))
      return true;
    else if (PeakPattern::guessBothDouble(models, false) &&
        PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused))
      return true;
  }

  // Then try to fill up with known models from the left or right.
  PeakPattern::getActiveModels(models, false);

  if (rangeData.qualLeft != QUALITY_NONE)
  {
    if (PeakPattern::guessLeft(models) &&
        PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused))
    {
      return true;
    }
  }

  if (rangeData.qualRight != QUALITY_NONE)
  {
    if (PeakPattern::guessRight(models) &&
        PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused))
      return true;

    if (rangeData.qualLeft == QUALITY_NONE &&
        PeakPattern::looksEmptyFirst(peakPtrsUsed))
    {
      peakPtrsUnused.moveFrom(peakPtrsUsed);
      return true;
    }
  }

  if (peakPtrsUsed.size() <= 2)
    return false;

  if (rangeData.qualBest != QUALITY_GENERAL && 
      rangeData.qualBest != QUALITY_NONE)
  {
    // Make an attempt to find short cars without a model.
    PeakPattern::getSpacings(peakPtrsUsed);

    for (auto& sit: spacings)
      cout << sit.str(offset);

    if (PeakPattern::guessAndFixShortLeft(peaks, 
        peakPtrsUsed, peakPtrsUnused))
      return true;

    if (PeakPattern::guessAndFixShortRight(peaks, 
        peakPtrsUsed, peakPtrsUnused))
      return true;

  }

  return false;
}


string PeakPattern::strClosest(const vector<unsigned>& indices) const
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

