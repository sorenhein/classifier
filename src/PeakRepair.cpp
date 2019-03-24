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

  PeakRepair::init(models.size(), range.endValue());

  RepairRange repairRange;
  list<unsigned> peakOrder;
  if (carpos == CARPOSITION_FIRST)
  {
    repairRange.start = range.endValue();
    repairRange.end = 0;
    repairRange.leftDirection = true;

    peakOrder.insert(peakOrder.end(), {3, 2, 1, 0});
  }
  else
  {
    // Both for inner and left car.
    repairRange.start = range.startValue();
    repairRange.end = range.endValue();
    repairRange.leftDirection = false;

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

/*
  if (superModel.count() == 4)
    cout << "WARNREPAIR: Full car (dominant)\n";
  else
  {
    for (unsigned peakNo = 0; peakNo < 4; peakNo++)
    {
      if (superModel.hasPeak(peakNo) || ! superModel.hasRange(peakNo))
        continue;

      // Is there almost a peak that fits already?
      Peak * pptr;
      unsigned lower, upper;
      if (superModel.getRange(peakNo, lower, upper))
      {
        unsigned indexUsed;
        peakPtrsUsed.locate(lower, upper, (lower+upper)/2,
          &Peak::always, indexUsed);
        
        if (pptr)
        {
          cout << "WARNREPAIR: Found use for peak\n";
          superModel.registerPtr(peakNo, pptr, indexUsed);
          continue;
        }
      } 

      // See if we could complete the peak.
      Peak peakHint;
      superModel.getPeak(peakNo, peakHint);

      pptr = peaks.repair(peakHint, &Peak::goodQuality, offset);
      if (pptr)
      {
        cout << "WARNREPAIR: " << superModel.strTarget(peakNo, offset) <<
          " fixable\n";
        
        // Add to superModel.
        superModel.registerPtr(peakNo, pptr);

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
*/
UNUSED(peaks);

  // May have changed.
  // superModel.makeCodes(peakPtrsUsed, offset);

  // Get some typical model data.
  unsigned bogieTypical, longTypical;
  models.getTypical(bogieTypical, longTypical);
cout << "Typical " << bogieTypical << ", " << longTypical << endl;

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

  range.split(peakPtrsUsedFlat, peakPtrsUsed, peakPtrsUnused);

  if (carpos == CARPOSITION_INNER_MULTI)
  {
    // We started from the left, so we'll only return unused peaks
    // up to the right end of the used peaks.

/*
cout << "MULTIUSED: " << peakPtrsUsed.size() << endl;
cout << peakPtrsUsed.front()->strHeaderQuality();

for (auto& p: peakPtrsUsed)
  cout<< p->strQuality(offset);

cout << "MULTIUNUSED: " << peakPtrsUnused.size() << endl;
cout << peakPtrsUnused.front()->strHeaderQuality();

for (auto& p: peakPtrsUnused)
  cout<< p->strQuality(offset);
  */

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
/*
cout << "ERASE left " << peakPtrsUnused.size() << endl;
cout << "AFTERERASE: " << peakPtrsUnused.size() << endl;
cout << peakPtrsUnused.front()->strHeaderQuality();

for (auto& p: peakPtrsUnused)
  cout<< p->strQuality(offset);
*/
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

