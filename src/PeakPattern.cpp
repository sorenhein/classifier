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
  fullEntries.clear();
}


bool PeakPattern::getRangeQuality(
  const CarModels& models,
  const PeakRange& range,
  CarDetect const * carPtr,
  const bool leftFlag,
  RangeQuality& quality,
  unsigned& gap) const
{
  gap = 0;
  quality = QUALITY_NONE;

  bool hasGap = (leftFlag ? range.hasLeftGap() : range.hasRightGap());
  if (hasGap)
  {
    cout << "SUGGEST-ERR: Range already has gap\n";
    return false;
  }

  if (! carPtr)
  {
    cout << "SUGGEST-ERR: Expected non-null car pointer\n";
    return false;
  }

  // The previous car could be contained in a whole car.
  const unsigned modelIndex = carPtr->getMatchData()->index;
  ModelData const * data = models.getData(modelIndex);
  const bool revFlag = carPtr->isReversed();

  if (data->fullFlag)
  {
    cout << "SUGGEST-ERR: Car should not already be full\n";
    return false;
  }

  if (leftFlag)
  {
    if ((revFlag && data->gapLeftFlag) || 
        (! revFlag && data->gapRightFlag))
    {
      cout << "SUGGEST-ERR: Left car should not have abutting gap\n";
      cout << data->str();
      return false;
    }
  }
  else
  {
    if ((revFlag && data->gapRightFlag) ||
        (! revFlag && data->gapLeftFlag))
    {
      cout << "SUGGEST-ERR: Right car should not have abutting gap\n";
      cout << data->str();
      return false;
    }
  }

  if (data->containedFlag)
  {
    if (revFlag)
      gap = (leftFlag ? data->gapRight : data->gapLeft);
    else
      gap = (leftFlag ? data->gapLeft : data->gapRight);

    quality = QUALITY_WHOLE_MODEL;
  }
  else if ((leftFlag && revFlag && data->gapRightFlag) ||
       (! leftFlag && ! revFlag && data->gapRightFlag))
  {
    gap = data->gapRight;
    quality = QUALITY_SYMMETRY;
  }
  else if ((leftFlag && ! revFlag && data->gapLeftFlag) ||
           (! leftFlag && revFlag && data->gapLeftFlag))
  {
    gap = data->gapLeft;
    quality = QUALITY_SYMMETRY;
  }
  else
  {
    // TODO Go by general gap
    // quality = QUALITY_GENERAL;
    cout << "SUGGEST-ERR: No general gap (yet?)\n";
    return false;
  }

  return true;
}


bool PeakPattern::guessNoBorders(list<PatternEntry>& candidates) const
{
  // This is a half-hearted try to fill in exactly one car of the 
  // same type as its neighbors if those neighbors do not have any
  // edge gaps at all.

  if (! carBeforePtr || ! carAfterPtr)
  {
    cout << "SUGGEST-ERR: guessNoBorders one+ pointer is null\n";
    return false;
  }


  if (carBeforePtr->index() != carAfterPtr->index())
  {
    cout << "SUGGEST-ERR: guessNoBorders cars are different\n";
    return false;
  }

  // We will not test for symmetry.
  const CarPeaksPtr& peaksBefore = carBeforePtr->getPeaksPtr();
  const CarPeaksPtr& peaksAfter = carAfterPtr->getPeaksPtr();

  const unsigned bogieGap = carBeforePtr->getLeftBogieGap();

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
  {
    cout << "SUGGEST-ERR: guessNoBorders left gap way too small\n";
    cout << "SUGGEST-YY0: avgLL " << avgLeftLeft << 
      " vs. " << peaksBefore.secondBogieRightPtr->getIndex() << endl;
    return false;
  }

  if (avgRightRight > peaksAfter.firstBogieLeftPtr->getIndex())
  {
    cout << "SUGGEST-ERR: guessNoBorders right gap way too small\n";
    cout << "SUGGEST-YY1: avgRR " << avgRightRight << 
      " vs. " << peaksAfter.firstBogieLeftPtr->getIndex() << endl;
    return false;
  }

  const unsigned delta = 
    avgLeftLeft - peaksBefore.secondBogieRightPtr->getIndex();

  if (delta > NO_BORDER_FACTOR * bogieGap)
  {
    cout << "SUGGEST-ERR: guessNoBorders cars are different vs bogie\n";
    cout << "SUGGEST-YY2: gap " << delta << " vs. " << bogieGap << endl;
    return false;
  }

  if (delta < SMALL_BORDER_FACTOR * bogieGap)
  {
    cout << "SUGGEST-ERR: guessNoBorders left gap tiny\n";
    cout << "SUGGEST-YY3: gap " << delta << " vs. " << bogieGap << endl;
    return false;
  }

  const unsigned mid = carBeforePtr->getMidGap();
  if (delta > mid)
  {
    cout << "SUGGEST-ERR: guessNoBorders cars are different vs mid gap\n";
    cout << "SUGGEST-YY4: gap " << delta << " vs. " << mid << endl;
    return false;
  }

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

cout << "NOBORDER\n";
cout << pe.str("SUGGEST-YY5");

  return true;
}


void PeakPattern::getFullModels(const CarModels& models)
{
  for (unsigned index = 0; index < models.size(); index++)
  {
    if (models.empty(index))
      continue;

    ModelData const * data = models.getData(index);
    if (! data->fullFlag)
      continue;

cout << "TTT got model " << index << endl;
cout << data->str();

    fullEntries.emplace_back(FullEntry());
    FullEntry& fe = fullEntries.back();
    fe.data = data;
    fe.index = index;

    // Three different qualities; only two used for now.
    fe.lenLo.resize(3);
    fe.lenHi.resize(3);

    const unsigned len = data->lenPP + data->gapLeft + data->gapRight;

    fe.lenLo[QUALITY_WHOLE_MODEL] =
      static_cast<unsigned>((1.f - LEN_FACTOR_GREAT) * len);
    fe.lenHi[QUALITY_WHOLE_MODEL] =
      static_cast<unsigned>((1.f + LEN_FACTOR_GREAT) * len);

    fe.lenLo[QUALITY_SYMMETRY] = 
      static_cast<unsigned>((1.f - LEN_FACTOR_GOOD) * len);
    fe.lenHi[QUALITY_SYMMETRY] =
      static_cast<unsigned>((1.f + LEN_FACTOR_GOOD) * len);
  }
}


void PeakPattern::fillFromModel(
  const CarModels& models,
  const unsigned indexModel,
  const bool symmetryFlag,
  const unsigned indexRangeLeft,
  const unsigned indexRangeRight,
  list<PatternEntry>& candidates) const
{
  candidates.emplace_back(PatternEntry());
  PatternEntry& pe = candidates.back();

  pe.modelNo = indexModel;
  pe.reverseFlag = false;
  pe.abutLeftFlag = true;
  pe.abutRightFlag = true;
  pe.start = indexRangeLeft;
  pe.end = indexRangeRight;

  list<unsigned> carPoints;
  models.getCarPoints(indexModel, carPoints);

  for (auto i = next(carPoints.begin()); i != carPoints.end(); i++)
    pe.indices.push_back(indexRangeLeft + * i);

  pe.borders = PATTERN_DOUBLE_SIDED;

cout << pe.str("SUGGEST-ZZ1");

  // Two options if asymmetric.
  if (symmetryFlag)
    return;

  candidates.emplace_back(PatternEntry());
  PatternEntry& pe2 = candidates.back();

  pe2.modelNo = indexModel;
  pe2.reverseFlag = true;
  pe2.abutLeftFlag = true;
  pe2.abutRightFlag = true;
  pe2.start = indexRangeLeft;
  pe2.end = indexRangeRight;

  unsigned pi = 3;
  for (auto i = next(carPoints.begin()); i != carPoints.end(); i++, pi--)
    pe2.indices[pi] = indexRangeLeft + * i;

  pe2.borders = PATTERN_DOUBLE_SIDED;

cout << pe.str("SUGGEST-ZZ1rev");
}


bool PeakPattern::guessLeft(
  const CarModels& models,
  const PeakRange& range,
  list<PatternEntry>& candidates) const
{
  UNUSED(models);
  UNUSED(range);
  UNUSED(candidates);
  cout << "SUGGEST-ERR: guessLeft\n";
  return false;
}


bool PeakPattern::guessRight(
  const CarModels& models,
  const PeakRange& range,
  list<PatternEntry>& candidates) const
{
  UNUSED(models);
  UNUSED(range);
  UNUSED(candidates);
  cout << "SUGGEST-ERR: guessRight\n";
  return false;
}


bool PeakPattern::guessBoth(
  const CarModels& models,
  list<PatternEntry>& candidates) const
{
  const unsigned indexLeft = 
    carBeforePtr->getPeaksPtr().secondBogieRightPtr->getIndex() + gapLeft;
  const unsigned indexRight = 
    carAfterPtr->getPeaksPtr().firstBogieLeftPtr->getIndex() - gapRight;

cout << "TTT actual abutting peaks: " <<
    carBeforePtr->getPeaksPtr().secondBogieRightPtr->getIndex() << ", " <<
    carAfterPtr->getPeaksPtr().firstBogieLeftPtr->getIndex() << "\n";

  if (indexRight <= indexLeft)

  {
    cout << "SUGGEST-ERR: guessBoth no range\n";
    return false;
  }

  RangeQuality qualOverall = (qualLeft <= qualRight ? qualLeft : qualRight);
  if (qualOverall == QUALITY_GENERAL || qualOverall == QUALITY_NONE)
  {
    cout << "SUGGEST-ERR: guessBoth unexpected quality\n";
    return false;
  }

  const unsigned lenRange = indexRight - indexLeft;
cout << "TTT looking for actual length " << lenRange << endl;

  unsigned count = 0;
  FullEntry const * fep = nullptr;
  for (auto& fe: fullEntries)
  {
cout << "TTT comparing to model " << fe.index << ": lenPP " <<
  fe.data->lenPP << ", " << fe.lenLo[qualOverall] << ", " <<
  fe.lenHi[qualOverall] << endl;

    if (lenRange >= fe.lenLo[qualOverall] &&
        lenRange <= fe.lenHi[qualOverall])
    {
      count++;
      fep = &fe;
    }
  }

  if (count == 0)
  {
    // TODO Double car
    cout << "SUGGEST-ERR: guessBoth no match to single car\n";
    return false;
  }

  if (count >= 2)
  {
    cout << "SUGGEST-ERR: guessBoth several matches to single car\n";
    return false;
  }

  PeakPattern::fillFromModel(models, fep->index, 
    fep->data->symmetryFlag, indexLeft, indexRight, candidates);

  return true;
}


bool PeakPattern::suggest(
  const CarModels& models,
  const PeakRange& range,
  list<PatternEntry>& candidates)
{
  carBeforePtr = range.carBeforePtr();
  carAfterPtr = range.carAfterPtr();

  if (! carBeforePtr && ! carAfterPtr)
  {
    cout << "SUGGEST-ERR: Entire range\n";
    return false;
  }


  if (! PeakPattern::getRangeQuality(models, range, carBeforePtr,
      true, qualLeft, gapLeft))
    qualLeft = QUALITY_NONE;

  if (! PeakPattern::getRangeQuality(models, range, carAfterPtr,
      false, qualRight, gapRight))
    qualRight = QUALITY_NONE;

cout << "SUGGESTX " << qualLeft << "-" << qualRight << "\n";
cout << "SUGGEST " << qualLeft << "-" << qualRight << ": " <<
  gapLeft << ", " << gapRight << endl;

  candidates.clear();

  // TODO A lot of these seem to be misalignments of cars with peaks.
  if (qualLeft == QUALITY_NONE && qualRight == QUALITY_NONE)
    return PeakPattern::guessNoBorders(candidates);

  PeakPattern::getFullModels(models);

  if (qualLeft == QUALITY_NONE)
    return PeakPattern::guessRight(models, range, candidates);
  else if (qualRight == QUALITY_NONE)
    return PeakPattern::guessLeft(models, range, candidates);
  else
    return PeakPattern::guessBoth(models, candidates);
}

