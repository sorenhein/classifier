#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "PeakPattern.h"
#include "PeakRange.h"

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
  candidates.clear();
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
    cout << rangeData.str("Range globals", offset);
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

cout << "getActiveModels: Got index " << index << endl;
cout << data->str() << endl;

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
}


bool PeakPattern::fillPoints(
  const list<unsigned>& carPoints,
  const unsigned indexBase,
  const bool reverseFlag,
  PatternEntry& pe) const
{
  if (pe.abutLeftFlag)
  {
    pe.indices.resize(4);

    if (! reverseFlag)
    {
      unsigned pi = 0;
      for (auto i = next(carPoints.begin()); i != prev(carPoints.end()); 
          i++, pi++)
        pe.indices[pi] = indexBase + * i;
    }
    else
    {
      const unsigned pointLast = carPoints.back();
      unsigned pi = 3;
      for (auto i = next(carPoints.begin()); i != prev(carPoints.end()); 
          i++, pi--)
        pe.indices[pi] = indexBase + pointLast - * i;
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

    pe.indices.resize(4);

    if (! reverseFlag)
    {
      unsigned pi = 0;
      for (auto i = next(carPoints.begin()); i != prev(carPoints.end()); 
          i++, pi++)
        pe.indices[pi] = indexBase + * i - pointLast;
    }
    else
    {
      unsigned pi = 3;
      for (auto i = next(carPoints.begin()); i != prev(carPoints.end()); 
          i++, pi--)
        pe.indices[pi] = indexBase - * i;
    }
  }

  return true;
}


bool PeakPattern::fillFromModel(
  const CarModels& models,
  const unsigned indexModel,
  const bool symmetryFlag,
  const unsigned indexRangeLeft,
  const unsigned indexRangeRight,
  const PatternType patternType)
{
  CarDetect car;
  models.getCar(car, indexModel);

  list<unsigned> carPoints;
  models.getCarPoints(indexModel, carPoints);

  PatternEntry pe;

  pe.modelNo = indexModel;
  pe.reverseFlag = false;
  pe.abutLeftFlag = (indexRangeLeft != 0);
  pe.abutRightFlag = (indexRangeRight != 0);

  if (pe.abutLeftFlag && pe.abutRightFlag)
  {
    pe.start = indexRangeLeft;
    pe.end = indexRangeRight;
  }
  else if (pe.abutLeftFlag)
  {
    pe.start = indexRangeLeft;
    pe.end = pe.start + carPoints.back();
  }
  else if (pe.abutRightFlag)
  {
    pe.start = indexRangeRight - carPoints.back();
    pe.end = indexRangeRight;
  }

  pe.borders = patternType;

  const unsigned indexBase = 
     (pe.abutLeftFlag ? indexRangeLeft : indexRangeRight);

  if (car.hasLeftGap())
  {
    if (PeakPattern::fillPoints(carPoints, indexBase, false, pe))
      candidates.emplace_back(pe);
  }

  if (symmetryFlag)
    return true;

  if (car.hasRightGap())
  {
    pe.reverseFlag = true;

    if (PeakPattern::fillPoints(carPoints, indexBase, true, pe))
      candidates.emplace_back(pe);
  }

  return true;
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

  candidates.clear();
  candidates.emplace_back(PatternEntry());
  PatternEntry& pe = candidates.back();

  pe.modelNo = carBeforePtr->index();
  pe.reverseFlag = false;
  pe.abutLeftFlag = false;
  pe.abutRightFlag = false;
  pe.start = avgLeftLeft - delta/2;
  pe.end = (peaksAfter.firstBogieLeftPtr->getIndex() - avgRightRight) / 2;

  pe.indices.push_back(avgLeftLeft);
  pe.indices.push_back(avgLeftRight);
  pe.indices.push_back(avgRightLeft);
  pe.indices.push_back(avgRightRight);

  pe.borders = PATTERN_NO_BORDERS;

  return true;
}


bool PeakPattern::guessBothSingle(const CarModels& models)
{
  for (auto& ae: activeEntries)
  {
    if (rangeData.lenRange >= ae.lenLo[rangeData.qualBest] &&
        rangeData.lenRange <= ae.lenHi[rangeData.qualBest])
    {
cout << ae.strShort("guessBothSingle", rangeData.qualBest);

      PeakPattern::fillFromModel(models, ae.index, 
        ae.data->symmetryFlag, rangeData.indexLeft, rangeData.indexRight, 
        PATTERN_DOUBLE_SIDED_SINGLE);
    }
  }

  return (! candidates.empty());
}


bool PeakPattern::guessBothSingleShort()
{
  if (rangeData.qualLeft == QUALITY_GENERAL || 
      rangeData.qualLeft == QUALITY_NONE)
    return false;

  // unsigned bogieTypical, longTypical, sideTypical;
  // models.getTypical(bogieTypical, longTypical, sideTypical);
  // if (sideTypical == 0)
    // sideTypical = bogieTypical;

  unsigned lenTypical = 2 * (sideTypical + bogieTypical) + longTypical;
  unsigned lenShortLo = static_cast<unsigned>(SHORT_FACTOR * lenTypical);
  unsigned lenShortHi = static_cast<unsigned>(LONG_FACTOR * lenTypical);

  if (rangeData.lenRange < lenShortLo ||
      rangeData.lenRange > lenShortHi)
    return false;

  PatternEntry pe;
  pe.modelNo = 0; // Doesn't matter
  pe.reverseFlag = false;
  pe.abutLeftFlag = true;
  pe.abutRightFlag = true;
  pe.start = rangeData.indexLeft;
  pe.end = rangeData.indexRight;
  pe.borders = PATTERN_DOUBLE_SIDED_SINGLE_SHORT;

  // Guess that particularly the middle part is shorter in a short car.
  list<unsigned> carPoints;
  carPoints.push_back(0);
  carPoints.push_back(sideTypical);
  carPoints.push_back(sideTypical + bogieTypical);
  carPoints.push_back(rangeData.lenRange - sideTypical - bogieTypical);
  carPoints.push_back(rangeData.lenRange - sideTypical);
  carPoints.push_back(rangeData.lenRange);

  if (PeakPattern::fillPoints(carPoints, pe.start, false, pe))
      candidates.emplace_back(pe);

  return (! candidates.empty());
}


bool PeakPattern::guessBothDouble(
  const CarModels& models,
  const bool leftFlag)
{
  candidates.clear();

  for (auto& ae1: activeEntries)
  {
    for (auto& ae2: activeEntries)
    {
      if (rangeData.lenRange >= 
          ae1.lenLo[rangeData.qualBest] + ae2.lenLo[rangeData.qualBest] &&
          rangeData.lenRange <= 
          ae1.lenHi[rangeData.qualBest] + ae2.lenHi[rangeData.qualBest])
      {
cout << ae1.strShort("guessBothDouble 1, left " + to_string(leftFlag),
  rangeData.qualBest);
cout << ae2.strShort("guessBothDouble 2, left " + to_string(leftFlag),
  rangeData.qualBest);

        if (leftFlag)
          PeakPattern::fillFromModel(models, ae1.index, 
            ae1.data->symmetryFlag, 
            rangeData.indexLeft, rangeData.indexRight, 
            PATTERN_DOUBLE_SIDED_DOUBLE);
        else
          PeakPattern::fillFromModel(models, ae2.index, 
            ae2.data->symmetryFlag, 
            rangeData.indexLeft, rangeData.indexRight, 
            PATTERN_DOUBLE_SIDED_DOUBLE);
      }
    }
  }

  return (! candidates.empty());
}


bool PeakPattern::guessLeft(const CarModels& models)
{
  // Starting from the left, so open towards the end.

  if (carBeforePtr == nullptr)
    return false;

  if (rangeData.qualLeft == QUALITY_GENERAL || 
      rangeData.qualLeft == QUALITY_NONE)
    return false;

  candidates.clear();

  for (auto& ae: activeEntries)
  {
cout << ae.strShort("guessLeft", rangeData.qualLeft);

    PeakPattern::fillFromModel(models, ae.index, ae.data->symmetryFlag,
      rangeData.indexLeft, 0, PATTERN_SINGLE_SIDED_LEFT);
  }

  return (! candidates.empty());
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

  candidates.clear();

  for (auto& ae: activeEntries)
  {
cout << ae.strShort("guessRight", rangeData.qualRight);

    PeakPattern::fillFromModel(models, ae.index, ae.data->symmetryFlag,
      0, rangeData.indexRight, PATTERN_SINGLE_SIDED_RIGHT);
  }

  return (candidates.size() > 0);
}


void PeakPattern::updateUnused(
  const PatternEntry& pe,
  PeakPtrs& peakPtrsUnused) const
{
  if (pe.borders == PATTERN_NO_BORDERS ||
      pe.borders == PATTERN_DOUBLE_SIDED_SINGLE ||
      pe.borders == PATTERN_DOUBLE_SIDED_SINGLE_SHORT)
  {
    // All the unused peaks are fair game.
  }
  else if (pe.borders == PATTERN_SINGLE_SIDED_LEFT ||
      (pe.borders == PATTERN_DOUBLE_SIDED_DOUBLE && pe.abutLeftFlag))
  {
    // Only keep unused peaks in range for later markdown.
    peakPtrsUnused.erase_above(pe.end);
  }
  else if (pe.borders == PATTERN_SINGLE_SIDED_RIGHT ||
      (pe.borders == PATTERN_DOUBLE_SIDED_DOUBLE && pe.abutRightFlag))
  {
    // Only keep unused peaks in range for later markdown.
    peakPtrsUnused.erase_below(pe.start);
  }
  else
    cout << "ERROR: Unrecognized border type.\n";
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
  PatternEntry& pe,
  NoneEntry& none) const
{
  none.pe = pe;
  none.peaksClose = peaksClose;
  none.emptyFlag = false;
}


void PeakPattern::addToSingles(
  const vector<unsigned>& indices,
  list<SingleEntry>& singles) const 
{
  // We rely heavily on having exactly one nullptr.
  // Singles are generally quite likely, so we permit more range.
  const unsigned bogieThird =
    (indices[3] - indices[2] + indices[1] - indices[0]) / 6;

  singles.emplace_back(SingleEntry());
  SingleEntry& se = singles.back();

  for (unsigned i = 0; i < peaksClose.size(); i++)
  {
    if (peaksClose[i] == nullptr)
    {
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


void PeakPattern::addToDoubles(
  const PatternEntry& pe,
  list<DoubleEntry>& doubles) const 
{
  // We rely heavily on having exactly two nullptrs.
  const unsigned bogieQuarter =
    (pe.indices[3] - pe.indices[2] + pe.indices[1] - pe.indices[0]) / 8;

  doubles.emplace_back(DoubleEntry());
  DoubleEntry& de = doubles.back();

  bool seenFirstFlag = false;
  bool seenSecondFlag = false;
  unsigned i0 = numeric_limits<unsigned>::max();
  unsigned i1 = numeric_limits<unsigned>::max();
  for (unsigned i = 0; i < peaksClose.size(); i++)
  {
    if (peaksClose[i] == nullptr)
    {
      if (! seenFirstFlag)
      {
        de.first.target = pe.indices[i];
        i0 = i;
        seenFirstFlag = true;
      }
      else
      {
        de.second.target = pe.indices[i];
        i1 = i;
        seenSecondFlag = true;
        break;
      }
    }
  }

  if (! seenSecondFlag)
    return;

  if (pe.borders == PATTERN_NO_BORDERS ||
      pe.borders == PATTERN_DOUBLE_SIDED_SINGLE ||
      pe.borders == PATTERN_DOUBLE_SIDED_SINGLE_SHORT)
  {
    // All the unused peaks are fair game.
  }
  else if (pe.borders == PATTERN_SINGLE_SIDED_LEFT ||
      (pe.borders == PATTERN_DOUBLE_SIDED_DOUBLE && pe.abutLeftFlag))
  {
    // Don't look open-ended from the left when the right bogie has
    // no match at all.
    if (i0 == 2 && i1 == 3)
      return;
  }
  else if (pe.borders == PATTERN_SINGLE_SIDED_RIGHT ||
      (pe.borders == PATTERN_DOUBLE_SIDED_DOUBLE && pe.abutRightFlag))
  {
    // Don't look open-ended from the right when the left bogie has
    // no match at all.
    if (i0 == 0 && i1 == 1)
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


void PeakPattern::examineCandidates(
  const PeakPtrs& peakPtrsUsed,
  NoneEntry& none,
  list<SingleEntry>& singles,
  list<DoubleEntry>& doubles)
{
  // Get the lie of the land.
  unsigned distBest = numeric_limits<unsigned>::max();
  singles.clear();
  doubles.clear();

  for (auto pe = candidates.begin(); pe != candidates.end(); )
  {
    unsigned numClose;
    unsigned dist;
    peakPtrsUsed.getClosest(pe->indices, peaksClose, numClose, dist);

    cout << PeakPattern::strClosest(pe->indices);

    if (numClose == 4)
    {
      // Here we take the best one, whereas we take a more democratic
      // approach for 2's and 3's.
      if (dist < distBest)
      {
        distBest = dist;
        PeakPattern::setNone(* pe, none);
      }
    }
    else if (numClose == 3)
      PeakPattern::addToSingles(pe->indices, singles);
    else if (numClose == 2)
      PeakPattern::addToDoubles(* pe, doubles);
    else if (numClose <= 1)
    {
      // Get rid of the 1's and 0's.
      pe = candidates.erase(pe);
      continue;
    }

    pe++;
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


void PeakPattern::readjust(list<SingleEntry>& singles)
{
  // This applies for a short car where we got three peaks.
  // Probably the fourth peak is at a predictable bogie distance
  // which we can better estimate now.

  if (candidates.size() != 1 || singles.size() != 1)
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

  candidates.front().indices[p] = target;
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
  PeakPtrs& peakPtrsUnused,
  Peak *& pptr) const
{
  unsigned indexUsed;
  pptr = peakPtrsUnused.locate(single.lower, single.upper, single.target,
    &Peak::borderlineQuality, indexUsed);

  PeakPattern::processMessage(text, "revive", single.target, pptr);
}


void PeakPattern::fixOnePeak(
  const string& text,
  const SingleEntry& single,
  PeakPool& peaks,
  Peak *& pptr) const
{
  Peak peakHint;
  peakHint.logPosition(single.target, single.lower, single.upper);
  pptr = peaks.repair(peakHint, &Peak::borderlineQuality, offset);

  PeakPattern::processMessage(text, "repair", single.target, pptr);
}


void PeakPattern::processOnePeak(
  const string& origin,
  const SingleEntry& single,
  PeakPtrs& peakPtrsUnused,
  PeakPool& peaks,
  Peak *& pptr) const
{
  PeakPattern::reviveOnePeak(origin, single, peakPtrsUnused, pptr);

  if (pptr == nullptr)
    PeakPattern::fixOnePeak("fixSingles", single, peaks, pptr);
}


bool PeakPattern::fixSingles(
  PeakPool& peaks,
  list<SingleEntry>& singles,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
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
      peakPtrsUnused, peaks, pptr);

    if (pptr != nullptr)
    {
cout << "fixSingles before add/remove" << endl;
      peakPtrsUsed.add(pptr);
      peakPtrsUnused.remove(pptr);
cout << "fixSingles after add/remove" << endl;
      return true;
    }
  }

  return false;
}


bool PeakPattern::fixDoubles(
  PeakPool& peaks,
  list<DoubleEntry>& doubles,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
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
      peakPtrsUnused, peaks, pptr);

    if (pptr != nullptr)
    {
cout << "fixDoubles 1 before add/remove" << endl;
      peakPtrsUsed.add(pptr);
      peakPtrsUnused.remove(pptr);
cout << "fixDoubles 1 after add/remove" << endl;
    }

    PeakPattern::processOnePeak("fixDoubles 2", db.second, 
      peakPtrsUnused, peaks, pptr);

    if (pptr != nullptr)
    {
cout << "fixDoubles 2 before add/remove" << endl;
      peakPtrsUsed.add(pptr);
      peakPtrsUnused.remove(pptr);
cout << "fixDoubles 2 after add/remove" << endl;
      return true;
    }
  }

  return false;
}


bool PeakPattern::fix(
  PeakPool& peaks,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused,
  bool flexibleFlag)
{
  NoneEntry none;
  list<SingleEntry> singles;
  list<DoubleEntry> doubles;

  PeakPattern::examineCandidates(peakPtrsUsed, none, singles, doubles);

  if (candidates.empty())
    return false;

  // Start with the 2's if that's all there is.
  if (none.empty() && singles.empty() && ! doubles.empty())
  {
    if (PeakPattern::fixDoubles(peaks, doubles, 
        peakPtrsUsed, peakPtrsUnused))
    {
      // TODO This is quite inefficient, as we actually know the
      // changes we made.  But we just reexamine for now.
cout << "Re-examining after fixDoubles" << endl;
      PeakPattern::examineCandidates(peakPtrsUsed, none, singles, doubles);
cout << "Re-examined after fixDoubles" << endl;
    }
  }

  // Try the 3's (original or 2's turned 3's) if there are no 4's.
  if (none.empty() && ! singles.empty())
  {
    // This happens in a short inner car.
    if (flexibleFlag)
      PeakPattern::readjust(singles);

    if (PeakPattern::fixSingles(peaks, singles, 
        peakPtrsUsed, peakPtrsUnused))
      PeakPattern::examineCandidates(peakPtrsUsed, none, singles, doubles);
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
    const PeakQuality pqL = se.peakLeft->getQualityWhole();
    const PeakQuality pqR = se.peakRight->getQualityWhole();
    se.qualityLower = (pqL >= pqR ? pqL : pqR);
  }
}


bool PeakPattern::guessAndFixShort(
  const bool leftFlag,
  const unsigned indexFirst,
  const unsigned indexLast,
  PeakPool& peaks,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
cout << "PPINDEX " << indexLast - indexFirst + 1 << endl;

  UNUSED(leftFlag);
  UNUSED(indexFirst);
  UNUSED(indexLast);
  UNUSED(peaks);
  UNUSED(peakPtrsUsed);
  UNUSED(peakPtrsUnused);

  return false;
}


bool PeakPattern::guessAndFixShortLeft(
  PeakPool& peaks,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  if (rangeData.qualLeft == QUALITY_NONE)
    return false;

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

  const unsigned ss = spacings.size();
  return PeakPattern::guessAndFixShort(
    false, (ss <= 4 ? 0 : ss-4), ss, peaks, peakPtrsUsed, peakPtrsUnused);
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

  // First try filling the entire range with 1-2 cards.
  if (rangeData.qualBest != QUALITY_GENERAL || 
      rangeData.qualBest == QUALITY_NONE)
  {
    PeakPattern::getActiveModels(models, true);

    // Note that guessBothDouble(true) could generate candidates
    // which aren't fitted.  So we can't return the fix value no
    // matter what, and we only return true if it fits.

    // The short one is not model-based, but it fits well here.
    if (PeakPattern::guessBothSingle(models))
      return PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused);
    else if (PeakPattern::guessBothSingleShort())
      return PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused, true);
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
      return true;
  }

  if (rangeData.qualRight != QUALITY_NONE)
  {
    if (PeakPattern::guessRight(models) &&
        PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused))
      return true;
  }

  if (peakPtrsUsed.size() <= 2)
    return false;

  if (rangeData.qualBest != QUALITY_GENERAL || 
      rangeData.qualBest == QUALITY_NONE)
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
    ss << setw(4) << left << i <<
      setw(8) << right << indices[i] + offset <<
      setw(8) << 
        (peaksClose[i] ? to_string(peaksClose[i]->getIndex() + offset) : 
          "-") << 
        endl;
  }
  return ss.str();
}

