#include <list>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "PeakRepair.h"
#include "CarModels.h"
#include "PeakPool.h"
#include "PeakRange.h"
#include "CarDetect.h"

#include "Except.h"

#define REPAIR_TOL 0.1f

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


PeakRepair::PeakRepair()
{
  PeakRepair::reset();
}


PeakRepair::~PeakRepair()
{
}


void PeakRepair::reset()
{
  offset = 0;
}


void PeakRepair::init(
  const unsigned msize,
  const unsigned start)
{
  partialData.clear();

  for (unsigned i = 0; i < msize; i++)
  {
    for (bool b: {false, true})
    {
      partialData.emplace_back(PeakPartial());
      PeakPartial& p = partialData.back();
      p.init(i, b, start);
    }
  }
}


bool PeakRepair::add(
  const unsigned end,
  const unsigned leftDirection,
  const unsigned pos1,
  const unsigned adder,
  unsigned& result) const
{
  // If we are looking left (leftDirection == true), then end < start.
  if (leftDirection)
  {
    if (pos1 < end + adder)
    {
      result = end;
      return false;
    }
    else
    {
      result = pos1 - adder;
      return true;
    }
  }
  else
  {
    if (pos1 + adder > end)
    {
      result = end;
      return false;
    }
    else
    {
      result = pos1 + adder;
      return true;
    } 
  }
}


bool PeakRepair::bracket(
  const RepairRange& range,
  const unsigned gap,
  unsigned& lower,
  unsigned& upper) const
{
  // If we are looking left (leftDirection == true), then end < start.

  const unsigned gapLower = static_cast<unsigned>((1.f - REPAIR_TOL) * gap);
  const unsigned gapUpper = static_cast<unsigned>((1.f + REPAIR_TOL) * gap);

  if (range.leftDirection)
  {
    if (! PeakRepair::add(range.end, range.leftDirection, range.start, 
        gapLower, upper))
      return false;

    PeakRepair::add(range.end, range.leftDirection, range.start, 
        gapUpper, lower);
  }
  else
  {
    if (! PeakRepair::add(range.end, range.leftDirection, range.start, 
        gapLower, lower))
      return false;

    PeakRepair::add(range.end, range.leftDirection, range.start, 
        gapUpper, upper);
  }
  return true;
}


Peak * PeakRepair::locatePeak(
  const unsigned lower,
  const unsigned upper,
  PeakPtrVector& peakPtrsUsed,
  unsigned& indexUsed) const
{
  unsigned num = 0;
  unsigned i = 0;
  Peak * ptr = nullptr;
  for (auto& p: peakPtrsUsed)
  {
    const unsigned index = p->getIndex();
    if (index >= lower && index <= upper)
    {
      num++;
      indexUsed = i;
      ptr = p;
    }
    i++;
  }

  if (num == 0)
    return nullptr;
  else if (num > 1)
  {
    // TODO May have to pick the best one.
    cout << "WARNING locatePeak: Multiple choices\n";
    return nullptr;
  }
  else
    return ptr;
}


bool PeakRepair::updatePossibleModels(
  RepairRange& range,
  const bool specialFlag,
  PeakPtrVector& peakPtrsUsed,
  const unsigned peakNo,
  const CarModels& models)
{
  // specialFlag is set for the first wheel in a car, as there are
  // two inter-car gaps in that case.
  // skippedFlag is set if the peak from which we're starting was
  // not in fact found, so we have to count up two gaps and not just one.
  // (We should try to guess the gap of the abutting car, rather than
  // just doubling our own gap.)
  // If we're at peak #1, we'll have to add g0+g1 and not just g1.
  // If we get to two skips, we give up.
  // 
  // |    #3    #2        #1    #0    |
  //   g4    g2      g2      g1    g0

  bool aliveFlag = false;
  for (auto& p: partialData)
  {
    if (! p.alive())
      continue;

    const unsigned gap = models.getGap(
      p.number(),
      (p.reversed() != range.leftDirection),
      specialFlag, 
      p.skipped(), 
      (p.reversed() ? 3-peakNo : peakNo));

    if (gap == 0)
    {
      p.registerFinished();
      continue;
    }

    // Start from the last seen peak.
    range.start = p.latest();

    unsigned lower, upper;
    if (! PeakRepair::bracket(range, gap, lower, upper))
      continue;

    p.registerRange(peakNo, lower, upper);

    unsigned indexUsed;
    Peak * pptr = PeakRepair::locatePeak(lower, upper, 
      peakPtrsUsed, indexUsed);

    p.registerPtr(peakNo, pptr);
    p.registerIndexUsed(peakNo, indexUsed);
  }
  return aliveFlag;
}


unsigned PeakRepair::numMatches() const
{
  unsigned numAlive = 0;
  for (auto& p: partialData)
  {
    if (p.used())
      numAlive++;
  }
  return numAlive;
}


bool PeakRepair::firstCar(
  const CarModels& models,
  const unsigned offsetIn,
  PeakPool& peaks,
  PeakRange& range,
  CarDetect& car)
{
  offset = offsetIn;

  PeakPtrVector peakPtrsUsed, peakPtrsUnused;
  range.splitByQuality(&Peak::greatQuality, peakPtrsUsed, peakPtrsUnused);

  PeakPtrVector peakResult(4, nullptr);

  PeakRepair::init(models.size(), range.endValue());

  RepairRange repairRange;
  repairRange.start = range.endValue();
  repairRange.end = range.startValue();
  repairRange.leftDirection = true;

  // This finds existing models within the first-car mess.

  // TODO Guess cars that don't fit a model.
  // For this we need an idea of usual gaps.  Then we can try to
  // guess what's a small gap and a large gap.

  for (unsigned peakNoPlus1 = 4; peakNoPlus1 > 0; peakNoPlus1--)
  {
    if (! PeakRepair::updatePossibleModels(repairRange, peakNoPlus1 == 4,
        peakPtrsUsed, peakNoPlus1-1, models))
      break;
  }

  const unsigned numModelMatches = PeakRepair::numMatches();
  if (numModelMatches == 0)
    return false;

  PeakRepair::printMatches();

  PeakPartial superModel;
  bool superFlag = true;
  for (auto& p: partialData)
  {
    if (! superModel.supersede(p))
    {
      superFlag = false;
      break;
    }
  }

  if (! superFlag)
  {
    cout << "WARNREPAIR: No dominant model " << superModel.count() << "\n";
    return false;
  }

  cout << "WARNREPAIR: Dominant model " << superModel.count() << "\n";
  cout << superModel.strHeader();
  cout << superModel.str(offset);

  if (superModel.count() == 4)
    cout << "WARNREPAIR: Full car (dominant)\n";
  else
  {
    for (unsigned i = 0; i < 4; i++)
    {
      if (superModel.hasPeak(i) || ! superModel.hasRange(i))
        continue;

      // See if we could complete the peak.
      Peak peakHint;
      superModel.setPeak(i, peakHint);

      if (peaks.repair(peakHint, &Peak::goodQuality, offset))
        cout << "WARNREPAIR: " << superModel.strTarget(i, offset) <<
          " fixable\n";
      else
        cout << "WARNREPAIR: " << superModel.strTarget(i, offset) <<
          " missing\n";
    }
  }

  // For the winning set of peaks:
  // Make downgrades.
  // Add the peaks.
  // Is this a model?
  // If not, add one?
  // Add car
  UNUSED(car);
  UNUSED(peaks);
  return false;
}


void PeakRepair::printMatches() const
{
  const unsigned n = PeakRepair::numMatches();
  if (n == 0)
    return;

  cout << "FIRSTCAR matches: " << n << "\n";
  cout << partialData.front().strHeader();
  
  for (auto& p: partialData)
  {
    if (p.used())
      cout << p.str(offset);
  }

  cout << endl;
}

