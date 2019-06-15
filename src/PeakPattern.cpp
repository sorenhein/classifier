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


void PeakPattern::setNone(
  Target& target,
  NoneEntry& none) const
{
  none.pe = target;

  none.peaksClose = peaksClose;
  none.emptyFlag = false;
}


void PeakPattern::addToSingles(
  const vector<unsigned>& indices,
  list<SingleEntry>& singles)
{
  // We rely heavily on having exactly one nullptr.
  // Singles are generally quite likely, so we permit more range.
  const unsigned bogieThird =
    (indices[3] - indices[2] + indices[1] - indices[0]) / 6;

  singles.emplace_back(SingleEntry());
  SingleEntry& se = singles.back();
  MissCar& miss = completions.back();

  for (unsigned i = 0; i < peaksClose.size(); i++)
  {
    if (peaksClose[i] == nullptr)
    {
miss.add(indices[i], bogieThird);
      se.target = indices[i];
      break;
    }
  }

  if (se.target < bogieThird)
    se.lower = 0;
  else
    se.lower = se.target - bogieThird;

  se.upper = se.target + bogieThird;
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


void PeakPattern::addToDoubles(
  const Target& target,
  list<DoubleEntry>& doubles)
{
  // We rely heavily on having exactly two nullptrs.
  const unsigned bogieQuarter =
    (target.index(3) - target.index(2) + target.index(1) - target.index(0)) / 8;

  doubles.emplace_back(DoubleEntry());
  DoubleEntry& de = doubles.back();

  bool seenFirstFlag = false;
  bool seenSecondFlag = false;
  unsigned i0 = numeric_limits<unsigned>::max();
  unsigned i1 = numeric_limits<unsigned>::max();
  MissCar& miss = completions.back();

  for (unsigned i = 0; i < peaksClose.size(); i++)
  {
    if (peaksClose[i] == nullptr)
    {
miss.add(target.index(i), bogieQuarter);
      if (! seenFirstFlag)
      {
        // de.first.target = target.indices[i];
        de.first.target = target.index(i);
        i0 = i;
        seenFirstFlag = true;
      }
      else
      {
        // de.second.target = target.indices[i];
        de.second.target = target.index(i);
        i1 = i;
        seenSecondFlag = true;
        break;
      }
    }
  }

  if (! seenSecondFlag)
  {
    cout << "DOUBERR\n";
    return;
  }

  if (de.first.target < bogieQuarter)
    de.first.lower = 0;
  else
    de.first.lower = de.first.target - bogieQuarter;

  de.first.upper = de.first.target + bogieQuarter;

  de.second.lower = de.second.target - bogieQuarter;
  de.second.upper = de.second.target + bogieQuarter;
}


void PeakPattern::addToTriples(
  const Target& pe,
  list<TripleEntry>& triples)
{
  triples.emplace_back(TripleEntry());
  TripleEntry& de = triples.back();

  const unsigned bogieQuarter = bogieTypical / 4;

  MissCar& miss = completions.back();

  unsigned pos = 0;
  for (unsigned i = 0; i < peaksClose.size(); i++)
  {
    if (peaksClose[i] == nullptr)
    {
miss.add(pe.index(i), bogieQuarter);

      if (pos == 0)
      {
        de.first.target = pe.index(i);
        pos++;
      }
      else if (pos == 1)
      {
        de.second.target = pe.index(i);
        pos++;
      }
      else if (pos == 2)
      {
        de.third.target = pe.index(i);
        pos++;
      }
    }
  }

  if (pos != 3)
    THROW(ERR_ALGO_PEAK_CONSISTENCY, "addToTriples error");

  de.first.bracket(bogieQuarter);
  de.second.bracket(bogieQuarter);
  de.third.bracket(bogieQuarter);
}


void PeakPattern::examineTargets(
  const PeakPtrs& peakPtrsUsed,
  NoneEntry& none,
  list<SingleEntry>& singles,
  list<DoubleEntry>& doubles,
  list<TripleEntry>& triples)
{
  // Get the lie of the land.
  unsigned distBest = numeric_limits<unsigned>::max();
  singles.clear();
  doubles.clear();
  triples.clear();
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
        completions.emplace_back(dist);
        PeakPattern::setNone(* target, none);
      }
    }
    else if (numClose == 3)
    {
      completions.emplace_back(dist);
      PeakPattern::addToSingles(target->indices(), singles);
    }
    else if (numClose == 2)
    {
      if (PeakPattern::checkDoubles(* target))
      {
        completions.emplace_back(dist);
        PeakPattern::addToDoubles(* target, doubles);
      }
    }
    else if (numClose == 1)
    {
      completions.emplace_back(dist);
      PeakPattern::addToTriples(* target, triples);
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


void PeakPattern::condenseSingles(list<SingleEntry>& singles) const
{
  for (auto se = singles.begin(); se != singles.end(); se++)
  {
    se->count = 1;
   
    for (auto se2 = next(se); se2 != singles.end(); )
    {
      if (se2->target >= se->lower && se2->target <= se->upper)
      {
        se->count++;
        se2 = singles.erase(se2);
      }
      else
        se2++;
    }
  }
  singles.sort();
}


void PeakPattern::condenseDoubles(list<DoubleEntry>& doubles) const
{
  for (auto de = doubles.begin(); de != doubles.end(); de++)
  {
    de->count = 1;
   
    for (auto de2 = next(de); de2 != doubles.end(); )
    {
      if (de2->first.target >= de->first.lower && 
          de2->first.target <= de->first.upper &&
          de2->second.target >= de->second.lower &&
          de2->second.target <= de->second.upper)
      {
        de->count++;
        de2 = doubles.erase(de2);
      }
      else
        de2++;
    }
  }
  doubles.sort();
}


void PeakPattern::condenseTriples(list<TripleEntry>& triples) const
{
  for (auto te = triples.begin(); te != triples.end(); te++)
  {
    te->count = 1;
   
    for (auto te2 = next(te); te2 != triples.end(); )
    {
      if (te2->first.target >= te->first.lower && 
          te2->first.target <= te->first.upper &&
          te2->second.target >= te->second.lower &&
          te2->second.target <= te->second.upper &&
          te2->third.target >= te->third.lower &&
          te2->third.target <= te->third.upper)
      {
        te->count++;
        te2 = triples.erase(te2);
      }
      else
        te2++;
    }
  }
  triples.sort();
}


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

  pptr = peaks.repair(peakHint, &Peak::borderlineQuality, offset,
    forceFlag);

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


bool PeakPattern::fixSingles(
  PeakPool& peaks,
  list<SingleEntry>& singles,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused,
  const bool forceFlag) const
{
  PeakPattern::condenseSingles(singles);

  cout << "Condensed singles\n";
  for (auto s: singles)
    cout << s.str(offset);

  Peak * pptr;
  for (auto& single: singles)
  {
    // Once we have 3 peaks, chances are good that we're right.
    // So we lower our peak quality standard.

    PeakPattern::processOnePeak("fixSingles", single, 
      peakPtrsUsed, peakPtrsUnused, peaks, pptr, forceFlag);

    if (pptr != nullptr)
    {
      peakPtrsUsed.add(pptr);
      peakPtrsUnused.remove(pptr);
      return true;
    }
  }

  return false;
}


bool PeakPattern::fixDoubles(
  PeakPool& peaks,
  list<DoubleEntry>& doubles,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused,
  const bool forceFlag) const
{
  PeakPattern::condenseDoubles(doubles);

  cout << "Condensed doubles\n";
  for (auto d: doubles)
    cout << d.str(offset);

  Peak * pptr;
  for (auto& db: doubles)
  {
    // We could end up fixing only one of the two, in which case
    // we should arguably unroll the change.

    PeakPattern::processOnePeak("fixDoubles 1", db.first, 
      peakPtrsUsed, peakPtrsUnused, peaks, pptr, forceFlag);

    if (pptr != nullptr)
    {
      peakPtrsUsed.add(pptr);
      peakPtrsUnused.remove(pptr);
    }

    PeakPattern::processOnePeak("fixDoubles 2", db.second, 
      peakPtrsUsed, peakPtrsUnused, peaks, pptr, forceFlag);

    if (pptr != nullptr)
    {
      peakPtrsUsed.add(pptr);
      peakPtrsUnused.remove(pptr);
      return true;
    }
  }

  return false;
}


bool PeakPattern::fixTriples(
  PeakPool& peaks,
  list<TripleEntry>& triples,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused,
  const bool forceFlag) const
{
  PeakPattern::condenseTriples(triples);

  cout << "Condensed triples\n";
  for (auto t: triples)
    cout << t.str(offset);

  Peak * pptr;
  for (auto& tr: triples)
  {
    // We could end up fixing less than all three, in which case
    // we should arguably unroll the change.

    PeakPattern::processOnePeak("fixTriples 1", tr.first, 
      peakPtrsUsed, peakPtrsUnused, peaks, pptr, forceFlag);

    if (pptr != nullptr)
    {
      peakPtrsUsed.add(pptr);
      peakPtrsUnused.remove(pptr);
    }

    PeakPattern::processOnePeak("fixTriples 2", tr.second, 
      peakPtrsUsed, peakPtrsUnused, peaks, pptr, forceFlag);

    if (pptr != nullptr)
    {
      peakPtrsUsed.add(pptr);
      peakPtrsUnused.remove(pptr);
      return true;
    }

    PeakPattern::processOnePeak("fixTriples 3", tr.third, 
      peakPtrsUsed, peakPtrsUnused, peaks, pptr, forceFlag);

    if (pptr != nullptr)
    {
      peakPtrsUsed.add(pptr);
      peakPtrsUnused.remove(pptr);
      return true;
    }
  }

  return false;
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

  NoneEntry none;
  list<SingleEntry> singles;
  list<DoubleEntry> doubles;
  list<TripleEntry> triples;

  PeakPattern::examineTargets(peakPtrsUsed, 
    none, singles, doubles, triples);

  for (auto pptr: peakPtrsUnused)
    completions.markWith(pptr, MISS_UNUSED);

  completions.condense();

  cout << completions.str(offset);

  if (none.empty() && singles.empty() && doubles.empty())
  {
    if (! forceFlag)
      return false;
    else if (PeakPattern::fixTriples(peaks, triples,
        peakPtrsUsed, peakPtrsUnused, forceFlag))
    {
      // TODO This is quite inefficient, as we actually know the
      // changes we made.  But we just reexamine for now.
      PeakPattern::examineTargets(peakPtrsUsed, 
        none, singles, doubles, triples);
    }
  }

  // Start with the 2's if that's all there is.
  if (none.empty() && singles.empty() && ! doubles.empty())
  {
    if (PeakPattern::fixDoubles(peaks, doubles, 
        peakPtrsUsed, peakPtrsUnused, forceFlag))
    {
      // TODO This is quite inefficient, as we actually know the
      // changes we made.  But we just reexamine for now.
      PeakPattern::examineTargets(peakPtrsUsed, 
        none, singles, doubles, triples);
    }
  }

  // Try the 3's (original or 2's turned 3's) if there are no 4's.
  if (none.empty() && ! singles.empty())
  {
    // This happens in a short inner car.
    if (flexibleFlag)
    {
      PeakPattern::readjust(singles);
      if (targets.front().outOfRange(rangeData.indexLeft, 
        rangeData.indexRight))
        return false;
    }

    if (PeakPattern::fixSingles(peaks, singles, 
        peakPtrsUsed, peakPtrsUnused, forceFlag))
    {
      PeakPattern::examineTargets(peakPtrsUsed, 
        none, singles, doubles, triples);
    }
  }

  // Try the 4's of various origins.
  if (! none.empty())
  {
    PeakPattern::update(none, peakPtrsUsed, peakPtrsUnused);
    return true;
  }

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

