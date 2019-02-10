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
  modelData.clear();
  for (unsigned i = 0; i < msize; i++)
  {
    for (bool b: {false, true})
    {
      modelData.emplace_back(ModelData());
      ModelData& m = modelData.back();

      m.mno = i;
      m.reverseFlag = b;

      m.matchFlag = true;
      m.skippedFlag = false;
      m.newFlag = true;
      m.lastIndex = start;

      for (unsigned j = 0; j < 4; j++)
        m.peaks.push_back(nullptr);
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


void PeakRepair::bracket(
  const RepairRange& range,
  const unsigned gap,
  const unsigned target,
  unsigned& lower,
  unsigned& upper) const
{
  const unsigned tolerance = static_cast<unsigned>(REPAIR_TOL * gap);
  if (range.leftDirection)
  {
    PeakRepair::add(range.end, true, target, tolerance, lower);
    PeakRepair::add(range.start, false, target, tolerance, upper);
  }
  else
  {
    PeakRepair::add(range.start, true, target, tolerance, lower);
    PeakRepair::add(range.end, false, target, tolerance, upper);
  }
}


Peak * PeakRepair::locatePeak(
  const RepairRange& range,
  const unsigned gap,
  PeakPtrVector& peakPtrsUsed) const
{
  // If we are looking left (leftDirection == true), then end < start.
  unsigned target;
  PeakRepair::add(range.end, range.leftDirection, range.start, gap, target);

  unsigned lower, upper;
  PeakRepair::bracket(range, gap, target, upper, lower);
  
  unsigned num = 0;
  Peak * ptr = nullptr;
  for (auto& p: peakPtrsUsed)
  {
    const unsigned index = p->getIndex();
    if (index >= lower && index <= upper)
    {
      num++;
      ptr = p;
    }
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
  for (auto& m: modelData)
  {
    if (! m.matchFlag)
      continue;
    
    const unsigned gap = models.getGap(m.mno, m.reverseFlag, 
      specialFlag, m.skippedFlag, peakNo);
    if (gap == 0)
      continue;

    // Start from the last seen peak.
    range.start = m.lastIndex;

    m.peaks[peakNo] = PeakRepair::locatePeak(range, gap, peakPtrsUsed);
    if (m.peaks[peakNo] == nullptr)
      m.newFlag = false;
    else
    {
      m.newFlag = true;
      m.lastIndex = m.peaks[peakNo]->getIndex();
      aliveFlag = true;
    }
  }
  return aliveFlag;
}


unsigned PeakRepair::numMatches() const
{
  unsigned numAlive = 0;
  for (auto& m: modelData)
  {
    if (m.matchFlag)
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
    if (PeakRepair::updatePossibleModels(repairRange, peakNoPlus1 == 4,
      peakPtrsUsed, peakNoPlus1-1, models))
    {
      for (auto& m: modelData)
      {
        // Give up on a model if we get two skips in a row.
        if (m.newFlag)
          m.skippedFlag = false;
        else if (m.skippedFlag)
          m.matchFlag = false;
        else
          m.skippedFlag = true;
      }
    }
    else
    {
      // Give up entirely if no model works.
      return false;
    }
  }

  PeakRepair::printMatches();

  // We've got at least one model.
  // If a partial one fits within the full one, use the partial one.
  //
  // Look for any missing peaks, but don't update those peaks yet.
  // This should go between the good neighbors, ignoring anything
  // that in this context is spurious.
  // The one with the most peaks wins.
  // If there is more than one and they are different w.r.t. peaks,
  // give a warning.
  //
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


string PeakRepair::strIndex(const Peak * peak) const
{
  if (peak == nullptr)
    return "";
  else
    return to_string(peak->getIndex() + offset);
}


void PeakRepair::printMatches() const
{
  cout << "FIRSTCAR matches: " << PeakRepair::numMatches() << "\n";

  cout << 
    setw(4) << left << "mno" <<
    setw(4) << left << "rev" <<
    setw(8) << left << "p1" <<
    setw(8) << left << "p2" <<
    setw(8) << left << "p3" <<
    setw(8) << left << "p4" << endl;
    
  for (auto& m: modelData)
  {
   if (! m.matchFlag)
     continue;

    cout << 
      setw(4) << left << m.mno <<
      setw(4) << left << (m.reverseFlag ? "R" : "") <<
      setw(8) << left << PeakRepair::strIndex(m.peaks[0]) <<
      setw(8) << left << PeakRepair::strIndex(m.peaks[1]) <<
      setw(8) << left << PeakRepair::strIndex(m.peaks[2]) <<
      setw(8) << left << PeakRepair::strIndex(m.peaks[3]) << "\n";
  }
  cout << endl;
}

