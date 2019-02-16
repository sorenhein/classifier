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

      m.modelUsedFlag = false;
      m.matchFlag = true;
      m.skippedFlag = false;
      m.newFlag = true;
      m.lastIndex = start;

      for (unsigned j = 0; j < 4; j++)
        m.peaks.push_back(nullptr);

      m.lower.resize(4);
      m.upper.resize(4);
      m.target.resize(4);
      m.indexUsed.resize(4);
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

if (ptr)
  cout << "got " << ptr->getIndex() + offset << endl;

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
    

    const unsigned gap = models.getGap(
      m.mno, 
      (m.reverseFlag != range.leftDirection),
      specialFlag, 
      m.skippedFlag, 
      (m.reverseFlag ? 3-peakNo : peakNo));

cout << "\nmno " << m.mno << ", rev " << m.reverseFlag <<
  ", spec " << specialFlag << ", skip " << m.skippedFlag << ", pno " <<
  peakNo << ": gap " << gap << endl;
    if (gap == 0)
    {
      m.matchFlag = false;
      continue;
    }

    // Start from the last seen peak.
    range.start = m.lastIndex;

cout << "range " << range.start << ", " << range.end << ", " <<
  range.leftDirection << endl;

    if (! PeakRepair::bracket(range, gap, m.lower[peakNo], m.upper[peakNo]))
      continue;

    m.target[peakNo] = (m.lower[peakNo] + m.upper[peakNo]) / 2;
cout << "lower " << m.lower[peakNo] << ", upper " << m.upper[peakNo] << endl;

    m.peaks[peakNo] = PeakRepair::locatePeak(m.lower[peakNo], 
      m.upper[peakNo], peakPtrsUsed, m.indexUsed[peakNo]);
    if (m.peaks[peakNo] == nullptr)
      m.newFlag = false;
    else
    {
cout << "updating m\n";
      m.modelUsedFlag = true;
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
    if (m.modelUsedFlag)
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

// cout << "FIRSTSTART\n";

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
      // return false;
      break;
    }
  }

  const unsigned numModelMatches = PeakRepair::numMatches();
  if (numModelMatches == 0)
    return false;

  PeakRepair::printMatches();

  // Check how many peaks each model uses, and the leftmost such peak.
  list<unsigned> numPeaks, leftmost;
  unsigned maxNumPeaks = 0, minLeftmost = 4;

  for (auto& m: modelData)
  {
    if (m.modelUsedFlag)
    {
      for (unsigned pno = 0; pno < 4; pno++)
      {
        if (m.peaks[pno])
        {
          leftmost.push_back(pno);
          if (pno < minLeftmost)
            minLeftmost = pno;
          break;
        }
      }

      unsigned n = 0;
      for (unsigned pno = 0; pno < 4; pno++)
      {
        if (m.peaks[pno])
          n++;
      }
      numPeaks.push_back(n);
      if (n > maxNumPeaks)
        maxNumPeaks = n;
    }
    else
    {
      numPeaks.push_back(0);
      leftmost.push_back(0);
    }
  }

  // Check whether all models use the same peaks.
  bool samePeaks = true;
  auto mi = modelData.begin();
  while (mi != modelData.end() && ! mi->modelUsedFlag)
    mi++;

  for (auto mi2 = modelData.begin(); mi2 != modelData.end(); mi2++)
  {
    if (mi2 == mi || ! mi2->modelUsedFlag)
      continue;

    for (unsigned i = 0; i < 4; i++)
    {
      if (mi->peaks[i] != mi2->peaks[i])
      {
        samePeaks = false;
        break;
      }
    }
    if (! samePeaks)
      break;
  }

  cout << "WARNREPAIR: Total\n";
  if (numModelMatches > 1)
  {
    if (! samePeaks)
      cout << "WARNREPAIR: Models have different peaks\n";

    bool shorterFlag = false;
    for (auto n: numPeaks)
    {
      if (n > 0 && n < maxNumPeaks)
      {
        shorterFlag = true;
        break;
      }
    }

    if (shorterFlag)
      cout << "WARNREPAIR: Shorter model\n";

    bool lefterFlag = false;
    for (auto n: leftmost)
    {
      if (n > 0 && n > minLeftmost)
      {
        lefterFlag = true;
        break;
      }
    }

    if (lefterFlag)
      cout << "WARNREPAIR: Righter model\n";
  }
  else
  {
    if (maxNumPeaks == 4)
      cout << "WARNREPAIR: Full car\n";
    else
    {
      for (unsigned i = 0; i < 4; i++)
      {
        if (mi->peaks[i] || mi->upper[i] == 0)
          continue;

        // See if we could complete the peak.
        Peak peakHint;
        peakHint.logPosition(mi->target[i], mi->lower[i], mi->upper[i]);

        if (peaks.repair(peakHint, &Peak::goodQuality, offset))
          cout << "WARNREPAIR: " << 
            mi->peaks[i] + offset << " fixable\n";
        else
          cout << "WARNREPAIR: " << 
            mi->peaks[i] + offset << " missing\n";
      }
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


string PeakRepair::strIndex(
  const ModelData& m,
  const unsigned pno) const
{
  if (m.peaks[pno] == nullptr)
  {
    if (m.lower[pno] == 0)
      return "-";
    else
      return "(" + to_string(m.lower[pno] + offset) + 
        ", " + to_string(m.upper[pno] + offset) + ")";
  }
  else
    return to_string(m.peaks[pno]->getIndex() + offset) +
      " (" + to_string(m.indexUsed[pno]) + ")";
}


void PeakRepair::printMatches() const
{
  const unsigned n = PeakRepair::numMatches();
  if (n == 0)
    return;

  cout << "FIRSTCAR matches: " << n << "\n";

  cout << 
    setw(4) << left << "mno" <<
    setw(4) << left << "rev" <<
    setw(16) << left << "p1" <<
    setw(16) << left << "p2" <<
    setw(16) << left << "p3" <<
    setw(16) << left << "p4" << endl;
    
  for (auto& m: modelData)
  {
   if (! m.modelUsedFlag)
     continue;

    cout << 
      setw(4) << left << m.mno <<
      setw(4) << left << (m.reverseFlag ? "R" : "") <<
      setw(16) << left << PeakRepair::strIndex(m, 0) <<
      setw(16) << left << PeakRepair::strIndex(m, 1) <<
      setw(16) << left << PeakRepair::strIndex(m, 2) <<
      setw(16) << left << PeakRepair::strIndex(m, 3) << "\n";
  }
  cout << endl;
}

