#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "CarModels.h"
#include "PeakShort.h"
#include "PeakRange.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))

// Expect a short car to be within these factors of a typical car.
#define SHORT_FACTOR 0.5f
#define LONG_FACTOR 0.9f


PeakShort::PeakShort()
{
  PeakShort::reset();
}


PeakShort::~PeakShort()
{
}


void PeakShort::reset()
{
  carBeforePtr = nullptr;
  carAfterPtr = nullptr;

  offset = 0;
}


bool PeakShort::setGlobals(
  const CarModels& models,
  const PeakRange& range,
  const unsigned offsetIn)
{
  carBeforePtr = range.carBeforePtr();
  carAfterPtr = range.carAfterPtr();

  offset = offsetIn;

  if (range.characterize(models, rangeData))
  {
    cout << rangeData.str("Range globals", offset);
    return true;
  }
  else
    return false;
}


void PeakShort::getShorty(const CarModels& models)
{
  models.getTypical(bogieTypical, longTypical, sideTypical);

  if (sideTypical == 0)
    sideTypical = bogieTypical;

  lenTypical = 2 * (sideTypical + bogieTypical) + longTypical;
  lenShortLo = static_cast<unsigned>(SHORT_FACTOR * lenTypical);
  lenShortHi = static_cast<unsigned>(LONG_FACTOR * lenTypical);
}


bool PeakShort::locate(
  const CarModels& models,
  PeakPool& peaks,
  const PeakRange& range,
  const unsigned offsetIn,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused)
{
  if (! PeakShort::setGlobals(models, range, offsetIn))
    return false;

  if (rangeData.qualLeft == QUALITY_NONE ||
      rangeData.qualRight == QUALITY_NONE)
    return false;

  // Find a measure of an expected short car.
  PeakShort::getShorty(models);

  if (rangeData.lenRange >= lenShortLo &&
      rangeData.lenRange <= lenShortHi)
  {
    cout << "SHORT seen " << rangeData.lenRange << " in " <<
      lenShortLo << " - " << lenShortHi << "\n";
  }

  UNUSED(peaks);
  UNUSED(peakPtrsUsed);
  UNUSED(peakPtrsUnused);

  // Update consists in:
  // Eliminating Used outside of needed
  // peakPtrsUnused.erase_below(start)
  // peakPtrsUnused.erase_above(end)

  return false;
}

