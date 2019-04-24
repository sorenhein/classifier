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

  if ((leftFlag && data->gapRightFlag) ||
      (! leftFlag && data->gapLeftFlag))
  {
    cout << "PeakPattern ERROR: Car should not have abutting gap\n";
    return false;
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


bool PeakPattern::suggest(
  const CarModels& models,
  const PeakRange& range,
  list<PatternEntry>& candidates) const
{
  CarDetect const * carBeforePtr = range.carBeforePtr();
  CarDetect const * carAfterPtr = range.carAfterPtr();

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

  // If neither of them is given, give up
  // If both are given, particularly comfortable
  // Separate into one or more cars that might fit

  UNUSED(candidates);
  return false;
}

