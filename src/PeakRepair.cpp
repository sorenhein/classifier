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

    if (pptr)
      aliveFlag = true;

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
  PeakPtrVector& peakPtrsUsed,
  PeakPtrVector& peakPtrsUnused)
{
  offset = offsetIn;

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
  {
    cout << "WARNREPAIR: No model match\n";
    return false;
  }

  PeakRepair::printMatches();

  // Pick a longest model to start.
  PeakPartial superModel;
  PeakPartial * superBest = nullptr;
  unsigned bestCount = 0;
  for (auto& p: partialData)
  {
    if (p.count() > bestCount)
    {
      bestCount = p.count();
      superBest = &p;
    }
  }
  superModel = * superBest;

  bool superFlag = true;
  for (auto& p: partialData)
  {
    if (! superModel.supersede(p))
    {
  cout << superModel.strHeader();
  cout << superModel.str(offset);
  cout << p.str(offset);
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
      superModel.getPeak(i, peakHint);

      Peak * pptr = peaks.repair(peakHint, &Peak::goodQuality, offset);
      if (pptr)
      {
        cout << "WARNREPAIR: " << superModel.strTarget(i, offset) <<
          " fixable\n";
        
        // Add to superModel.
        superModel.registerPtr(i, pptr);

        // Remove from Unused if needed
        for (auto p = peakPtrsUnused.begin(); p != peakPtrsUnused.end(); )
        {
          if (*p == pptr)
            p = peakPtrsUnused.erase(p);
          else
            p++;
        }
      }
    }
  }

  // Fill out Used with the peaks actually used.
  superModel.getPeaks(peakPtrsUsed, peakPtrsUnused);
  return true;
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

