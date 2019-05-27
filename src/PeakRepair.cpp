#include <string>
#include <iostream>
#include <iomanip>
#include <algorithm>

#include "PeakRepair.h"
#include "CarModels.h"
#include "PeakPool.h"
#include "PeakRange.h"
#include "CarDetect.h"

#define REPAIR_TOL 0.1f

// 1.2x is a normal range for short cars, plus a bit of buffer.
#define BOGIE_GENERAL_FACTOR 0.4f

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
  const CarModels& models,
  const unsigned start)
{
  partialData.clear();

  for (unsigned i = 0; i < models.size(); i++)
  {
    if (models.empty(i))
      continue;

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


bool PeakRepair::updatePossibleModels(
  RepairRange& range,
  const bool specialFlag,
  const PeakPtrs& peakPtrsUsed,
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

    // Start from the last seen peak.
    range.start = p.latest();

    Peak const * pptr = nullptr;
    unsigned indexUsed;

    if (specialFlag && 
        range.leftDirection &&
        peakPtrsUsed.back()->fitsType(BOGIE_RIGHT, WHEEL_RIGHT))
    {
      // Fourth wheel (first car)
      pptr = peakPtrsUsed.back();
      indexUsed = peakPtrsUsed.size()-1;
    }
    else if (specialFlag &&
         ! range.leftDirection &&
         peakPtrsUsed.front()->fitsType(BOGIE_LEFT, WHEEL_LEFT))
    {
      // First wheel (last car).
      pptr = peakPtrsUsed.front();
      indexUsed = 0;
    }
    else
    {
      const unsigned gap = models.getGap(
        p.number(),
        (p.reversed() != range.leftDirection),
        specialFlag, 
        p.skipped(), 
        (p.reversed() ? 3-peakNo : peakNo));

// cout << "model " << p.number() << ", " << p.reversed() << ", " <<
  // peakNo << ": " << gap << endl;
      if (gap == 0)
      {
        p.registerFinished();
        continue;
      }

      unsigned lower, upper;
      if (! PeakRepair::bracket(range, gap, lower, upper))
      {
        p.registerFinished();
        continue;
      }
// cout << "lower " << lower << " upper " << upper << endl;

      pptr = peakPtrsUsed.locate(lower, upper, (lower+upper) / 2,
        &Peak::always, indexUsed);
      if (! pptr)
        p.registerRange(peakNo, lower, upper);
    }

    if (pptr)
      aliveFlag = true;

    p.registerPtr(peakNo, pptr, indexUsed);
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


bool PeakRepair::getDominantModel(PeakPartial& dominantModel) const
{
  // Pick a longest model to start.
  const PeakPartial * superBest = nullptr;
  unsigned bestCount = 0;
  for (auto& p: partialData)
  {
    if (p.count() > bestCount)
    {
      bestCount = p.count();
      superBest = &p;
    }
  }
  dominantModel = * superBest;

  bool superFlag = true;
  for (auto& p: partialData)
  {
    if (! dominantModel.supersede(p))
    {
      cout << dominantModel.strHeader();
      cout << dominantModel.str(offset);
      cout << p.str(offset);

      superFlag = false;
      break;
    }
  }

  if (superFlag)
  {
    cout << "WARNREPAIR: Dominant model " << 
      dominantModel.count() << "\n";
    cout << dominantModel.strHeader();
    cout << dominantModel.str(offset);
    return true;
  }
  else
  {
    cout << "WARNREPAIR: No dominant model " << 
      dominantModel.count() << "\n";
    return false;
  }
}


void PeakRepair::repairFourth(
  const CarPosition carpos,
  const PeakRange& range,
  PeakPool& peaks,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
cout << "repairFourth " << static_cast<unsigned>(carpos) << ", " << 
  peakPtrsUsed.size()  << endl;
  // TODO For now.
  if (carpos != CARPOSITION_INNER_SINGLE)
    return;

  vector<unsigned> pp;
  for (auto& ptr: peakPtrsUsed)
  {
    if (ptr)
      pp.push_back(ptr->getIndex());
  }

  const unsigned d10 = pp[1] - pp[0];
  const unsigned d21 = pp[2] - pp[1];

  bool leftBogieFlag;
  if (d10 < d21)
    leftBogieFlag = true;
  else 
    leftBogieFlag = false;

  unsigned indexSingle;
  unsigned dBogie;
  unsigned dEdgeCertain;
  unsigned dEdgeOther;
  unsigned outerPeakExpect;
  unsigned innerPeakExpect;
  unsigned peakNo;

  if (leftBogieFlag)
  {
    // Assume that 0 and 1 belong together in a bogie.
    indexSingle = pp[2];
    dBogie = d10;
    dEdgeCertain = pp[0] - range.startValue();
    dEdgeOther = range.endValue() - indexSingle;
    outerPeakExpect = range.endValue() - dEdgeCertain;
    innerPeakExpect = outerPeakExpect - dBogie;
  }
  else
  {
    // Assume that 1 and 2 belong together in a bogie.
    indexSingle = pp[0];
    dBogie = d21;
    dEdgeCertain = range.endValue() - pp[2];
    dEdgeOther = indexSingle - range.startValue();
    outerPeakExpect = range.startValue() + dEdgeCertain;
    innerPeakExpect = outerPeakExpect + dBogie;
  }

  unsigned delta = static_cast<unsigned>(BOGIE_GENERAL_FACTOR * dBogie);
  Peak peakHint;

cout << "indexSingle " << indexSingle + offset << 
  "\nrange " << range.startValue() + offset << 
    " - " << range.endValue() + offset << 
  "\ndBogie " << dBogie << 
  "\nEdgeCertain " << dEdgeCertain << 
  "\ndEdgeOther " << dEdgeOther << 
  "\ninnerPeakExpect " << innerPeakExpect + offset << 
  "\nouterPeakExpect " << outerPeakExpect + offset << 
  "\ndelta " << delta << endl << endl;
  if (indexSingle >= outerPeakExpect - delta &&
      indexSingle <= outerPeakExpect + delta)
  {
    // Expect the missing peak to be in the inward direction.
    peakHint.logPosition(innerPeakExpect,
      innerPeakExpect - delta, innerPeakExpect + delta);
    peakNo = (leftBogieFlag ? 2 : 1);
  }
  else if (indexSingle >= innerPeakExpect - delta &&
      indexSingle <= innerPeakExpect + delta)
  {
    // Expect the missing peak to be in the outward direction.
    peakHint.logPosition(outerPeakExpect,
      outerPeakExpect - delta, outerPeakExpect + delta);
    peakNo = (leftBogieFlag ? 3 : 0);
  }
  else
  {
    // Not sure.
    cout << "Give up on\n";
    cout << innerPeakExpect - delta << " - " << innerPeakExpect + delta << "\n";
    cout << outerPeakExpect - delta << " - " << outerPeakExpect + delta << "\n";
    return;
  }

  cout << "Look for " << peakHint.strQuality(offset) << endl;
  Peak * pptr = peaks.repair(peakHint, &Peak::goodQuality, offset);
  if (pptr)
  {
    cout << "INNER REPAIR: " << pptr->strQuality(offset) << endl;

    for (auto p = peakPtrsUsed.begin(); p != peakPtrsUsed.end(); p++)
    {
      if (* p == nullptr)
      {
        * p = pptr;
        break;
      }
    }

    peakPtrsUsed.sort();

    // Remove from Unused if needed
    for (auto p = peakPtrsUnused.begin(); p != peakPtrsUnused.end(); )
    {
      if (*p == pptr)
        p = peakPtrsUnused.erase(p);
      else
        p++;
    }
  }
  else
  {
    cout << "NO INNER REPAIR" << endl;
  }
}


bool PeakRepair::edgeCar(
  const CarModels& models,
  const unsigned offsetIn,
  const CarPosition carpos,
  PeakPool& peaks,
  PeakRange& range,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused)
{
  offset = offsetIn;

  RepairRange repairRange;
  list<unsigned> peakOrder;
  if (carpos == CARPOSITION_FIRST)
  {
    repairRange.start = range.endValue();
    repairRange.end = 0;
    repairRange.leftDirection = true;

    PeakRepair::init(models, repairRange.start);

    peakOrder.insert(peakOrder.end(), {3, 2, 1, 0});
  }
  else
  {
    // Both for inner and right-facing (left) car.
    repairRange.start = range.startValue();
    repairRange.end = range.endValue();
    repairRange.leftDirection = false;

    PeakRepair::init(models, repairRange.start);

    peakOrder.insert(peakOrder.end(), {0, 1, 2, 3});
  }

  unsigned skips = 0;
  bool firstElemFlag = true;
  for (unsigned peakNo: peakOrder)
  {
    if (! PeakRepair::updatePossibleModels(repairRange, firstElemFlag,
        peakPtrsUsed, peakNo, models))
      skips++;
    if (skips == 2)
      break;
    firstElemFlag = false;
  }

  const unsigned numModelMatches = PeakRepair::numMatches();
  if (numModelMatches == 0)
  {
    bool edgeFlag;
    if (carpos == CARPOSITION_FIRST)
      edgeFlag = peakPtrsUsed.back()->fitsType(BOGIE_RIGHT, WHEEL_RIGHT);
    else
      edgeFlag = peakPtrsUsed.front()->fitsType(BOGIE_LEFT, WHEEL_LEFT);

    bool modelEndFlag = models.hasAnEndGap();

    cout << PeakRepair::prefix(carpos) <<
      "QUADRANT " << edgeFlag << ", " << modelEndFlag << endl;

cout << peakPtrsUsed.strQuality("No model match", offset);

    cout << "WARNREPAIR: No model match\n";
    return false;
  }

cout << "MATCH" << PeakRepair::prefix(carpos) <<  "\n";

  PeakPartial superModel;
  if (! PeakRepair::getDominantModel(superModel))
  {
cout << "NODOM" << PeakRepair::prefix(carpos) << "\n";
  // PeakRepair::printMatches(carpos);

    return false;
  }

  PeakRepair::printMatches(carpos);
  superModel.makeCodes(peakPtrsUsed, offset);
  superModel.printSituation(carpos);

  // Get some typical model data.
  unsigned bogieTypical, longTypical, sideTypical;
  models.getTypical(bogieTypical, longTypical, sideTypical);
cout << "Typical " << bogieTypical << ", " << 
  longTypical << ", " <<
  sideTypical << endl;

  // Fill out Used with the peaks actually used.

  vector<Peak const *> peakPtrsUsedFlat, peakPtrsUnusedFlat;
  peakPtrsUsed.flatten(peakPtrsUsedFlat);

  superModel.getPeaks(bogieTypical, longTypical, carpos,
    peakPtrsUsedFlat, peakPtrsUnusedFlat);

  if (peakPtrsUnusedFlat.size() > 0)
  {
    cout << "WARNSPARE: " << peakPtrsUnusedFlat.size() << endl;
    cout << peakPtrsUnusedFlat.front()->strHeaderQuality();

    for (auto& p: peakPtrsUnusedFlat)
      cout<< p->strQuality(offset);
  }

  unsigned count = 0;
  for (auto& p: peakPtrsUsedFlat)
    if (p)
      count++;

  range.split(peakPtrsUsedFlat, peakPtrsUsed, peakPtrsUnused);

cout << "TRY " << count << endl;
  if (count == 3)
  {
    // Try to repair the fourth peak.
    if (peakPtrsUsed.size() != 3)
      cout << "ERROR\n";

cout << "USED BEFORE: " << peakPtrsUsed.size() << endl;
cout << peakPtrsUsed.front()->strHeaderQuality();

for (auto& p: peakPtrsUsed)
  if (p)
    cout<< p->strQuality(offset);

if (peakPtrsUnused.size())
{
cout << "UNUSED BEFORE: " << peakPtrsUnused.size() << endl;
cout << peakPtrsUnused.front()->strHeaderQuality();

for (auto& p: peakPtrsUnused)
  if (p)
    cout<< p->strQuality(offset);
}

    PeakRepair::repairFourth(carpos, range, peaks, 
      peakPtrsUsed, peakPtrsUnused);

cout << "USED AFTER: " << peakPtrsUsed.size() << endl;
cout << peakPtrsUsed.front()->strHeaderQuality();

for (auto& p: peakPtrsUsed)
  if (p)
    cout<< p->strQuality(offset);

if (peakPtrsUnused.size())
{
cout << "UNUSED AFTER: " << peakPtrsUnused.size() << endl;
cout << peakPtrsUnused.front()->strHeaderQuality();

for (auto& p: peakPtrsUnused)
  if (p)
    cout<< p->strQuality(offset);
}

  }

  if (carpos == CARPOSITION_INNER_MULTI)
  {
    // We started from the left, so we'll only return unused peaks
    // up to the right end of the used peaks.

  unsigned last = 0;
  for (auto p: peakPtrsUsedFlat)
  {
    if (p)
      last = p->getIndex();
  }
  if (last == 0)
    return false;
  else
    peakPtrsUnused.erase_above(last);
  }

  return true;
}


string PeakRepair::prefix(const CarPosition carpos) const
{
  if (carpos == CARPOSITION_FIRST)
    return "FIRST";
  else if (carpos == CARPOSITION_INNER_SINGLE ||
      carpos == CARPOSITION_INNER_MULTI)
    return "INNER";
  else if (carpos == CARPOSITION_LAST)
    return "LAST";
  else
    return "";
}


void PeakRepair::printMatches(const CarPosition carpos) const
{
  const unsigned n = PeakRepair::numMatches();
  if (n == 0)
    return;

  cout << PeakRepair::prefix(carpos) << "CAR matches: " << n << "\n";

  cout << partialData.front().strHeader();
  
  for (auto& p: partialData)
  {
    if (p.used())
      cout << p.str(offset);
  }

  cout << endl;
}

