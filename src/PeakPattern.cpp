#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "PeakPattern.h"
#include "CarModels.h"
#include "PeakRange.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))

// If we try to interpolate an entire car, the end gap should not be
// too large relative to the bogie gap.
#define NO_BORDER_FACTOR 3.0f


PeakPattern::PeakPattern()
{
  PeakPattern::reset();
}


PeakPattern::~PeakPattern()
{
}


void PeakPattern::reset()
{
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
    cout << "PeakPattern WARNING: Range already has gap\n";
    return false;
  }

  if (! carPtr)
    return false;

  // The previous car could be contained in a whole car.
  const unsigned modelIndex = carPtr->getMatchData()->index;
  ModelData const * data = models.getData(modelIndex);
  const bool revFlag = carPtr->isReversed();

  if (data->fullFlag)
  {
    cout << "PeakPattern ERROR: Car should not already be full\n";
    return false;
  }

  if (leftFlag)
  {
    if ((revFlag && data->gapLeftFlag) || 
        (! revFlag && data->gapRightFlag))
    {
      cout << "PeakPattern left ERROR: Car should not have abutting gap\n";
cout << data->str();
      return false;
    }
  }
  else
  {
    if ((revFlag && data->gapRightFlag) ||
        (! revFlag && data->gapLeftFlag))
    {
      cout << "PeakPattern right ERROR: Car should not have abutting gap\n";
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
    return false;

  if (carBeforePtr->index() != carAfterPtr->index())
    return false;

  // We will not test for symmetry.
  const CarPeaksPtr& peaksBefore = carBeforePtr->getPeaksPtr();
  const CarPeaksPtr& peaksAfter = carAfterPtr->getPeaksPtr();

  const unsigned bogieGap = carBeforePtr->getLeftBogieGap();
  const unsigned avgLeftLeft = 
    (peaksBefore.firstBogieLeftPtr->getIndex() +
     peaksAfter.firstBogieLeftPtr->getIndex()) / 2;

  // Disqualify if the resulting car is implausible.
  if (avgLeftLeft - peaksBefore.secondBogieRightPtr->getIndex() >
      NO_BORDER_FACTOR * bogieGap)
    return false;

  candidates.emplace_back(PatternEntry());
  PatternEntry& pe = candidates.back();

  pe.modelNo = carBeforePtr->index();
  pe.reverseFlag = false;
  pe.abutLeftFlag = false;
  pe.abutRightFlag = false;

  pe.indices.push_back(avgLeftLeft);
  pe.indices.push_back((peaksBefore.firstBogieRightPtr->getIndex() +
    peaksAfter.firstBogieRightPtr->getIndex()) / 2);
  pe.indices.push_back((peaksBefore.secondBogieLeftPtr->getIndex() +
    peaksAfter.secondBogieLeftPtr->getIndex()) / 2);
  pe.indices.push_back((peaksBefore.secondBogieRightPtr->getIndex() +
    peaksAfter.secondBogieRightPtr->getIndex()) / 2);

  pe.borders = PATTERN_NO_BORDERS;

cout << "NOBORDER\n";

  return true;
}


bool PeakPattern::guessLeft(
  const CarModels& models,
  const PeakRange& range,
  const bool leftFlag,
  const RangeQuality quality,
  list<PatternEntry>& candidates) const
{
  UNUSED(models);
  UNUSED(range);
  UNUSED(leftFlag);
  UNUSED(quality);
  UNUSED(candidates);
  return false;
}


bool PeakPattern::guessRight(
  const CarModels& models,
  const PeakRange& range,
  const bool leftFlag,
  const RangeQuality quality,
  list<PatternEntry>& candidates) const
{
  UNUSED(models);
  UNUSED(range);
  UNUSED(leftFlag);
  UNUSED(quality);
  UNUSED(candidates);
  return false;
}


bool PeakPattern::guessBoth(
  const CarModels& models,
  const PeakRange& range,
  const RangeQuality qualLeft,
  const RangeQuality qualRight,
  list<PatternEntry>& candidates) const
{
  UNUSED(models);
  UNUSED(range);
  UNUSED(qualLeft);
  UNUSED(qualRight);
  UNUSED(candidates);
  return false;
}


bool PeakPattern::suggest(
  const CarModels& models,
  const PeakRange& range,
  list<PatternEntry>& candidates)
{
  carBeforePtr = range.carBeforePtr();
  carAfterPtr = range.carAfterPtr();

  if (! carBeforePtr && ! carAfterPtr)
    return false;

  RangeQuality qualLeft, qualRight;
  unsigned gapLeft, gapRight;

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
  else if (qualLeft == QUALITY_NONE)
    return PeakPattern::guessRight(models, range, false, qualRight,
      candidates);
  else if (qualRight == QUALITY_NONE)
    return PeakPattern::guessLeft(models, range, true, qualLeft,
      candidates);
  else
    return PeakPattern::guessBoth(models, range, qualLeft, qualRight,
      candidates);
}

