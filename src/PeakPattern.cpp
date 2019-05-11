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

cout << "fillFromModel: " << pe.abutLeftFlag << ", " <<
  pe.abutRightFlag << "," <<
  indexRangeLeft << ", " <<
  indexRangeRight << endl;

  // TODO Find a better way to detect this.
  if (* next(carPoints.begin()) != 0)
  {
    // This is a somewhat indirect measure of whether the car has
    // a front gap.
  if (! PeakPattern::fillPoints(carPoints, 
      (pe.abutLeftFlag ? indexRangeLeft : indexRangeRight), false, pe))
    return false;

  candidates.emplace_back(pe);
cout << pe.strAbs("SUGGEST-ZZ1", offset);

  // Two options if asymmetric.
  if (symmetryFlag)
    return true;
  }
  else
    cout << "skip due to lead zero\n";

  if (* prev(prev(carPoints.end())) != 0)
  {
  pe.reverseFlag = true;

  if (! PeakPattern::fillPoints(carPoints, 
      (pe.abutLeftFlag ? indexRangeLeft : indexRangeRight), true, pe))
    return false;

  candidates.emplace_back(pe);

cout << pe.strAbs("SUGGEST-ZZ1rev", offset);
  }
  else
    cout << "skip due to trail zero\n";


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
  if (carBeforePtr == nullptr)
    return false;

  // Starting from the left, so open towards the end.

cout << "TTT guessLeft actual abutting peak: " <<
  carBeforePtr->getPeaksPtr().secondBogieRightPtr->getIndex() + offset << 
  ", gapLeft " << gapLeft << "\n";

  if (qualLeft == QUALITY_GENERAL || qualLeft == QUALITY_NONE)
  {
    cout << "SUGGEST-ERR: guessLeft unexpected quality\n";
    return false;
  }

  for (auto& ae: activeEntries)
  {
cout << " TTT got left model: " << ae.index << ", " <<
  indexLeft + offset << endl;
    PeakPattern::fillFromModel(models, ae.index, ae.data->symmetryFlag,
      indexLeft, 0, PATTERN_SINGLE_SIDED_LEFT);
  }

  return (candidates.size() > 0);
}


bool PeakPattern::guessRight(const CarModels& models)
{
  if (carAfterPtr == nullptr)
    return false;

  // Starting from the right, so open towards the beginning.
  if (gapRight >= carAfterPtr->getPeaksPtr().firstBogieLeftPtr->getIndex())
  {
    cout << "SUGGEST-ERR: guessRight no room\n";
    return false;
  }

cout << "TTT guessRight actual abutting peak: " <<
  carAfterPtr->getPeaksPtr().firstBogieLeftPtr->getIndex() + offset << "\n";

  if (qualRight == QUALITY_GENERAL || qualRight == QUALITY_NONE)
  {
    cout << "SUGGEST-ERR: guessRight unexpected quality\n";
    return false;
  }

  for (auto& ae: activeEntries)
  {
cout << " TTT got right model: " << ae.index << endl;
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
cout << "All unused peaks are OK\n";
  }
  else if (pe.borders == PATTERN_SINGLE_SIDED_LEFT ||
      (pe.borders == PATTERN_DOUBLE_SIDED_DOUBLE && pe.abutLeftFlag))
  {
    // Only keep unused peaks in range for later markdown.
cout << "Erasing unused above " << pe.end << endl;
    peakPtrsUnused.erase_above(pe.end);
  }
  else if (pe.borders == PATTERN_SINGLE_SIDED_RIGHT ||
      (pe.borders == PATTERN_DOUBLE_SIDED_DOUBLE && pe.abutRightFlag))
  {
    // Only keep unused peaks in range for later markdown.
cout << "Erasing unused below " << pe.start << endl;
    peakPtrsUnused.erase_below(pe.start);
  }
  else
  {
    cout << "ERROR: Unrecognized border type.\n";
  }
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
cout << "Looking for " << indexClose + offset << "\n";
    while (pu != peakPtrsUsed.end() && (* pu)->getIndex() < indexClose)
    {
cout << "  erasing " << (* pu)->getIndex() + offset << endl;
      pu = peakPtrsUsed.erase(pu);
    }

    if (pu == peakPtrsUsed.end())
      break;

    // Preserve those peaks that are also in peaksClose.
    if ((* pu)->getIndex() == indexClose)
    {
cout << "  found " << (* pu)->getIndex() + offset << endl;
      pu = peakPtrsUsed.next(pu);
    }
  }

  while (pu != peakPtrsUsed.end())
  {
cout << "  erasing trailing " << (* pu)->getIndex() + offset << endl;
    pu = peakPtrsUsed.erase(pu);
  }
}


void PeakPattern::update(
  const NoneEntry& none,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  PeakPattern::updateUnused(none.pe, peakPtrsUnused);
  PeakPattern::updateUsed(none.peaksBest, peakPtrsUsed);
}


void PeakPattern::printClosest(
  const vector<unsigned>& indices,
  const vector<Peak const *>& peaksClose) const
{
  cout << "printClosest\n";
  for (unsigned i = 0; i < indices.size(); i++)
  {
    cout << setw(4) << left << i <<
      setw(8) << right << indices[i] + offset <<
      setw(8) << 
        (peaksClose[i] ? to_string(peaksClose[i]->getIndex() + offset) : 
          "-") << 
        endl;
  }
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

    PeakPattern::printClosest(pe->indices, peaksClose);

    if (numClose <= 1)
    {
      // Get rid of the 1's and 0's.
      pe = candidates.erase(pe);
      continue;
    }
    else if (numClose == 2)
      PeakPattern::addToDoubles(* pe, peaksClose, doubles);
    else if (numClose == 3)
      PeakPattern::addToSingles(pe->indices, peaksClose, singles);
    else if (numClose == 4)
    {
      // Here we take the best one, whereas we take a more democratic
      // approach for 2's and 3's.
      if (dist < distBest)
      {
        distBest = dist;
        PeakPattern::setNone(* pe, peaksClose, none);
      }
    }

    pe++;
  }
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


void PeakPattern::setNone(
  PatternEntry& pe,
  vector<Peak const *>& peaksBest,
  NoneEntry& none) const
{
  none.pe = pe;
  none.peaksBest = peaksBest;
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


bool PeakPattern::fixSingles(
  PeakPool& peaks,
  list<SingleEntry>& singles,
  PeakPtrs& peakPtrsUsed) const
{
cout << "Singles before\n";
for (auto s: singles)
  cout << s.str(offset);

  PeakPattern::condenseSingles(singles);

cout << "Singles after\n";
for (auto s: singles)
  cout << s.str(offset);

  Peak peakHint;
  for (auto& single: singles)
  {
    // Once we have 3 peaks, chances are good that we're right.
    // So we lower our peak quality standard.
    peakHint.logPosition(single.target, single.lower, single.upper);
    Peak * pptr = peaks.repair(peakHint, &Peak::acceptableQuality, offset);

    if (pptr == nullptr)
    {
      cout << "Failed repair target " << single.target + offset << "\n";
      continue;
    }

    cout << "Repaired target " << single.target + offset << " to " <<
      pptr->getIndex() + offset << endl;

    peakPtrsUsed.add(pptr);
    return true;
  }

  return false;
}


bool PeakPattern::fixDoubles(
  PeakPool& peaks,
  list<DoubleEntry>& doubles,
  PeakPtrs& peakPtrsUsed) const
{
cout << "Doubles before\n";
for (auto d: doubles)
  cout << d.str(offset);

  PeakPattern::condenseDoubles(doubles);

cout << "Doubles after\n";
for (auto d: doubles)
  cout << d.str(offset);

  Peak peakHint;
  for (auto& db: doubles)
  {
    peakHint.logPosition(db.first.target, db.first.lower, db.first.upper);
    Peak * pptr = peaks.repair(peakHint, &Peak::goodQuality, offset);
    if (pptr == nullptr)
      continue;

    cout << "Repaired target " << db.first.target + offset << " to " <<
      pptr->getIndex() + offset << endl;

    peakPtrsUsed.add(pptr);

    peakHint.logPosition(db.second.target, db.second.lower, db.second.upper);
    pptr = peaks.repair(peakHint, &Peak::goodQuality, offset);
    if (pptr == nullptr)
      continue;

    cout << "Repaired target " << db.second.target + offset << " to " <<
      pptr->getIndex() + offset << endl;

    peakPtrsUsed.add(pptr);

    return true;
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

cout << "VERIFY1 " << ! none.empty() << ", " <<
  singles.size() << ", " << doubles.size() << endl;

  // Start with the 2's if that's all there is.
  if (none.empty() && singles.empty() && ! doubles.empty())
  {
    if (PeakPattern::fixDoubles(peaks, doubles, peakPtrsUsed))
    {
      PeakPattern::examineCandidates(peakPtrsUsed, none, singles, doubles);
    }
  }

  // Try the 3's (original or 2's turned 3's) if there are no 4's.
  if (none.empty() && ! singles.empty())
  {
    if (PeakPattern::fixSingles(peaks, singles, peakPtrsUsed))
    {
      PeakPattern::examineCandidates(peakPtrsUsed, none, singles, doubles);
    }
  }

  // Try the 4's of various origins.
  if (! none.empty())
  {
    cout << "Have all 4 indices now\n";
    PeakPattern::update(none, peakPtrsUsed, peakPtrsUnused);
    return true;
  }

  if (candidates.front().borders == PATTERN_SINGLE_SIDED_RIGHT &&
      ! singles.empty() &&
      singles.front().target < 500)
  {
    // We don't need to flag the difficult first cars here.
    return false;
  }

cout << "VERIFY2 " << singles.size() << ", " << doubles.size() << endl;

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


  bool candFlag = false;
  if (! candFlag)
  {
cout << "Both was not successful, now looking at one-sided\n";
  PeakPattern::getActiveModels(models, false);

  // Add both!
  if (qualLeft != QUALITY_NONE)
  {
cout << "Trying left\n";
    if (PeakPattern::guessLeft(models))
      candFlag = true;
  }

  if (qualRight != QUALITY_NONE)
  {
cout << "Trying right\n";
    if (PeakPattern::guessRight(models))
      candFlag = true;
  }
  }

  if (! candFlag)
    return false;

  // Probably the best way to do this is to check (non-destructively)
  // for each candidate whether its missing peaks can be repaired, and
  // only then repair those peaks.  In PeakPool::repair it would indeed
  // be possible to do this.  But I'm going the easier way here for now.

  return PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused);
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

