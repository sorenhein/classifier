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


bool PeakPattern::suggest(
  const CarModels& models,
  const PeakRange& range,
  list<PatternEntry>& candidates) const
{
  UNUSED(models);
  UNUSED(range);
  UNUSED(candidates);
  return false;
}

