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

  qualLeft = QUALITY_NONE;
  qualRight = QUALITY_NONE;
  qualBest = QUALITY_NONE;
  qualWorst = QUALITY_NONE;

  gapLeft = 0;
  gapRight = 0;

  indexLeft = 0;
  indexRight = 0;
  lenRange = 0;

  activeEntries.clear();
  candidates.clear();
}


bool PeakPattern::getRangeQuality(
  const CarModels& models,
  CarDetect const * carPtr,
  const bool leftFlag,
  RangeQuality& quality,
  unsigned& gap) const
{
  if (! carPtr)
    return false;

  bool modelFlipFlag = false;
  unsigned modelIndex = carPtr->getMatchData()->index;
  ModelData const * data = models.getData(modelIndex);

  // The abutting car could be contained in a whole car.
  if (data->containedFlag)
  {
    modelFlipFlag = data->containedReverseFlag;
    modelIndex = data->containedIndex;
    data = models.getData(modelIndex);
  }

  if (! data->gapLeftFlag && ! data->gapRightFlag)
    return false;

  // Two flips would make a regular order.
  const bool revFlag = carPtr->isReversed() ^ modelFlipFlag;

  if (data->gapRightFlag && leftFlag == revFlag)
  {
    gap = data->gapRight;
    quality = QUALITY_WHOLE_MODEL;
  }
  else if (data->gapLeftFlag && leftFlag != revFlag)
  {
    gap = data->gapLeft;
    quality = QUALITY_WHOLE_MODEL;
  }
  else if (data->gapRightFlag)
  {
    gap = data->gapRight;
    quality = QUALITY_SYMMETRY;
  }
  else
  {
    gap = data->gapLeft;
    quality = QUALITY_SYMMETRY;
  }

  return true;
}


bool PeakPattern::setGlobals(
  const CarModels& models,
  const PeakRange& range,
  const unsigned offsetIn)
{
  carBeforePtr = range.carBeforePtr();
  carAfterPtr = range.carAfterPtr();

  if (! carBeforePtr && ! carAfterPtr)
    return false;

  offset = offsetIn;

  if (! PeakPattern::getRangeQuality(models, carBeforePtr,
      true, qualLeft, gapLeft))
    qualLeft = QUALITY_NONE;

  if (! PeakPattern::getRangeQuality(models, carAfterPtr,
      false, qualRight, gapRight))
    qualRight = QUALITY_NONE;

  qualBest = (qualLeft <= qualRight ? qualLeft : qualRight);
  qualWorst = (qualLeft <= qualRight ? qualRight : qualLeft);

  if (carBeforePtr)
    indexLeft = carBeforePtr->
      getPeaksPtr().secondBogieRightPtr->getIndex() + gapLeft;

  if (carAfterPtr)
  indexRight = carAfterPtr->
    getPeaksPtr().firstBogieLeftPtr->getIndex() - gapRight;

  if (indexRight > 0 && indexRight <= indexLeft)
    return false;

  lenRange = indexRight - indexLeft;

cout << PeakPattern::strGlobals();

  return true;
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

    const unsigned len = data->lenPP + data->gapLeft + data->gapRight;

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
    if (lenRange >= ae.lenLo[qualBest] &&
        lenRange <= ae.lenHi[qualBest])
    {
cout << ae.strShort("guessBothSingle", qualBest);

      PeakPattern::fillFromModel(models, ae.index, 
        ae.data->symmetryFlag, indexLeft, indexRight, 
        PATTERN_DOUBLE_SIDED_SINGLE);
    }
  }

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
      if (lenRange >= ae1.lenLo[qualBest] + ae2.lenLo[qualBest] &&
          lenRange <= ae1.lenHi[qualBest] + ae2.lenHi[qualBest])
      {
cout << ae1.strShort("guessBothDouble 1, left " + to_string(leftFlag),
  qualBest);
cout << ae2.strShort("guessBothDouble 2, left " + to_string(leftFlag),
  qualBest);

        if (leftFlag)
          PeakPattern::fillFromModel(models, ae1.index, 
            ae1.data->symmetryFlag, indexLeft, indexRight, 
            PATTERN_DOUBLE_SIDED_DOUBLE);
        else
          PeakPattern::fillFromModel(models, ae2.index, 
            ae2.data->symmetryFlag, indexLeft, indexRight, 
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

  if (qualLeft == QUALITY_GENERAL || qualLeft == QUALITY_NONE)
    return false;

  candidates.clear();

  for (auto& ae: activeEntries)
  {
cout << ae.strShort("guessLeft", qualLeft);

    PeakPattern::fillFromModel(models, ae.index, ae.data->symmetryFlag,
      indexLeft, 0, PATTERN_SINGLE_SIDED_LEFT);
  }

  return (! candidates.empty());
}


bool PeakPattern::guessRight(const CarModels& models)
{
  // Starting from the right, so open towards the beginning.

  if (carAfterPtr == nullptr)
    return false;

  if (qualRight == QUALITY_GENERAL || qualRight == QUALITY_NONE)
    return false;

  if (gapRight >= carAfterPtr->getPeaksPtr().firstBogieLeftPtr->getIndex())
    return false;

  candidates.clear();

  for (auto& ae: activeEntries)
  {
cout << ae.strShort("guessRight", qualLeft);

    PeakPattern::fillFromModel(models, ae.index, ae.data->symmetryFlag,
      0, indexRight, PATTERN_SINGLE_SIDED_RIGHT);
  }

  return (candidates.size() > 0);
}


void PeakPattern::updateUnused(
  const PatternEntry& pe,
  PeakPtrs& peakPtrsUnused) const
{
  if (pe.borders == PATTERN_NO_BORDERS ||
      pe.borders == PATTERN_DOUBLE_SIDED_SINGLE)
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
  const vector<Peak const *>& peaksClose,
  PeakPtrs& peakPtrsUsed) const
{
  // There may be some peaks that have to be moved.
  auto pu = peakPtrsUsed.begin();
  for (auto& peak: peaksClose)
  {
    const unsigned indexClose = peak->getIndex();
    while (pu != peakPtrsUsed.end() && (* pu)->getIndex() < indexClose)
      pu = peakPtrsUsed.erase(pu);

    if (pu == peakPtrsUsed.end())
      break;

    // Preserve those peaks that are also in peaksClose.
    if ((* pu)->getIndex() == indexClose)
      pu = peakPtrsUsed.next(pu);
  }

  // Erase trailing peaks.
  while (pu != peakPtrsUsed.end())
    pu = peakPtrsUsed.erase(pu);
}


void PeakPattern::update(
  const NoneEntry& none,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  PeakPattern::updateUnused(none.pe, peakPtrsUnused);
  PeakPattern::updateUsed(none.peaksBest, peakPtrsUsed);
}


void PeakPattern::setNone(
  PatternEntry& pe,
  vector<Peak const *>& peaksBest,
  NoneEntry& none) const
{
  none.pe = pe;
  none.peaksBest = peaksBest;
}


void PeakPattern::addToSingles(
  const vector<unsigned>& indices,
  const vector<Peak const *>& peaksClose,
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
  const vector<Peak const *>& peaksClose,
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
      pe.borders == PATTERN_DOUBLE_SIDED_SINGLE)
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
    vector<Peak const *> peaksClose;
    unsigned numClose;
    unsigned dist;
    peakPtrsUsed.getClosest(pe->indices, peaksClose, numClose, dist);

    PeakPattern::strClosest(pe->indices, peaksClose);

    if (numClose == 4)
    {
      // Here we take the best one, whereas we take a more democratic
      // approach for 2's and 3's.
      if (dist < distBest)
      {
        distBest = dist;
        PeakPattern::setNone(* pe, peaksClose, none);
      }
    }
    else if (numClose == 3)
      PeakPattern::addToSingles(pe->indices, peaksClose, singles);
    else if (numClose == 2)
      PeakPattern::addToDoubles(* pe, peaksClose, doubles);
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


void PeakPattern::fixOnePeak(
  const string& text,
  const unsigned target,
  const unsigned lower,
  const unsigned upper,
  PeakPool& peaks,
  Peak *& pptr) const
{
  Peak peakHint;
  peakHint.logPosition(target, lower, upper);
  // pptr = peaks.repair(peakHint, &Peak::acceptableQuality, offset);
  pptr = peaks.repair(peakHint, &Peak::borderlineQuality, offset);

  if (pptr == nullptr)
  {
    cout << text << ": Failed repair target " << target + offset << "\n";
  }
  else
  {
    cout << text << ": Repaired target " << target + offset << " to " <<
      pptr->getIndex() + offset << endl;
  }
}


bool PeakPattern::fixSingles(
  PeakPool& peaks,
  list<SingleEntry>& singles,
  PeakPtrs& peakPtrsUsed) const
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

    PeakPattern::fixOnePeak("fixSingles",
      single.target, single.lower, single.upper, peaks, pptr);

    if (pptr != nullptr)
    {
      peakPtrsUsed.add(pptr);
      return true;
    }
  }

  return false;
}


bool PeakPattern::fixDoubles(
  PeakPool& peaks,
  list<DoubleEntry>& doubles,
  PeakPtrs& peakPtrsUsed) const
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

    PeakPattern::fixOnePeak("fixDoubles 1",
      db.first.target, db.first.lower, db.first.upper, peaks, pptr);

    if (pptr != nullptr)
      peakPtrsUsed.add(pptr);

    PeakPattern::fixOnePeak("fixDoubles 2",
      db.second.target, db.second.lower, db.second.upper, peaks, pptr);

    if (pptr != nullptr)
    {
      peakPtrsUsed.add(pptr);
      return true;
    }
  }

  return false;
}


bool PeakPattern::fix(
  PeakPool& peaks,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused)
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
    if (PeakPattern::fixDoubles(peaks, doubles, peakPtrsUsed))
    {
      // TODO This is quite inefficient, as we actually know the
      // changes we made.  But we just reexamine for now.
      PeakPattern::examineCandidates(peakPtrsUsed, none, singles, doubles);
    }
  }

  // Try the 3's (original or 2's turned 3's) if there are no 4's.
  if (none.empty() && ! singles.empty())
  {
    if (PeakPattern::fixSingles(peaks, singles, peakPtrsUsed))
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

  if (qualLeft == QUALITY_NONE && qualRight == QUALITY_NONE)
  {
  // TODO A lot of these seem to be misalignments of cars with peaks.
  // So it's not clear that we should recognize them.
    if (PeakPattern::guessNoBorders() &&
        PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused))
      return true;
    else
      return false;
  }

  // First try filling the entire range with 1-2 cards.
  if (qualBest != QUALITY_GENERAL || qualBest == QUALITY_NONE)
  {
    PeakPattern::getActiveModels(models, true);

    // Note that guessBothDouble(true) could generate candidates
    // which aren't fitted.  So we can't return the fix value no
    // matter what, and we only return true if it fits.
    if (guessBothSingle(models))
      return PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused);
    else if (guessBothDouble(models, true) &&
        PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused))
      return true;
    else if (guessBothDouble(models, false) &&
        PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused))
      return true;
  }

  // Then try to fill up from the left or right.
  PeakPattern::getActiveModels(models, false);

  if (qualLeft != QUALITY_NONE)
  {
    if (PeakPattern::guessLeft(models) &&
        PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused))
      return true;
  }

  if (qualRight != QUALITY_NONE)
  {
    if (PeakPattern::guessRight(models) &&
        PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused))
      return true;
  }

  return false;
}


string PeakPattern::strQuality(const RangeQuality& qual) const
{
  if (qual == QUALITY_WHOLE_MODEL)
    return "WHOLE";
  else if (qual == QUALITY_SYMMETRY)
    return "SYMMETRY";
  else if (qual == QUALITY_GENERAL)
    return "GENERAL";
  else
    return "(none)";
}


string PeakPattern::strGlobals() const
{
  stringstream ss;
  ss << "PeakPattern globals:\n" << 
    setw(10) << left << "qualLeft " << 
      PeakPattern::strQuality(qualLeft) << "\n" <<
    setw(10) << left << "qualRight " << 
      PeakPattern::strQuality(qualRight) << "\n" <<
    setw(10) << left << "gapLeft " << gapLeft << "\n" <<
    setw(10) << left << "gapRight " << gapLeft << "\n" <<
    setw(10) << left << "indices " << 
      indexLeft + offset << " - " << indexRight + offset <<  "\n" <<
    setw(10) << left << "length " << lenRange << endl;
  return ss.str();
}


string PeakPattern::strClosest(
  const vector<unsigned>& indices,
  const vector<Peak const *>& peaksClose) const
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

