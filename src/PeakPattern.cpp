#include <iostream>
#include <sstream>
#include <mutex>

#include "PeakPattern.h"

#include "CarModels.h"
#include "const.h"
#include "Except.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


typedef bool (PeakPattern::*FindPatternPtr)(const CarModels& models);
  
struct PatternFncGroup
{
  FindPatternPtr fptr;
  string name;
  bool forceFlag;
  unsigned number;
};

static mutex mtx;
static list<PatternFncGroup> patternMethods;


PeakPattern::PeakPattern()
{
  mtx.lock();
  PeakPattern::setMethods();
  mtx.unlock();

  PeakPattern::reset();
}


PeakPattern::~PeakPattern()
{
}


void PeakPattern::reset()
{
  carBeforePtr = nullptr;
  carAfterPtr = nullptr;

  modelsActive.clear();
  targets.clear();
}


void PeakPattern::setMethods()
{
  patternMethods.push_back(
    { &PeakPattern::guessNoBorders, "by no borders", false, 0});

  patternMethods.push_back(
    { &PeakPattern::guessBothSingle, "by both single", true, 1});

  patternMethods.push_back(
    { &PeakPattern::guessBothDoubleLeft, "by double left", false, 2});

  patternMethods.push_back(
    { &PeakPattern::guessBothDoubleRight, "by double right", false, 3});

  patternMethods.push_back(
    { &PeakPattern::guessBothSingleShort, "by both single short", true, 4});

  patternMethods.push_back(
    { &PeakPattern::guessLeft, "by left", false, 5});

  patternMethods.push_back(
    { &PeakPattern::guessRight, "by right", false, 6});
}


bool PeakPattern::setGlobals(
  const CarModels& models,
  const PeakRange& range,
  const unsigned offsetIn)
{
  if (models.size() == 0)
    return false;

  carBeforePtr = range.carBeforePtr();
  carAfterPtr = range.carAfterPtr();

  offset = offsetIn;

  models.getTypical(bogieTypical, longTypical, sideTypical);
  if (sideTypical == 0)
    sideTypical = bogieTypical;

  return (range.characterize(models, rangeData));
}


void PeakPattern::getActiveModels(const CarModels& models)
{
  modelsActive.clear();

  for (unsigned index = 0; index < models.size(); index++)
  {
    if (models.empty(index))
      continue;

    ModelData const * data = models.getData(index);
    if (data->containedFlag || ! data->bothBogiesFlag)
      continue;

    modelsActive.emplace_back(ModelActive());
    ModelActive& ma = modelsActive.back();
    ma.data = data;
    ma.index = index;
    ma.fullFlag = data->fullFlag;

    unsigned len;
    if (data->gapLeft == 0)
      len = data->lenPP + 2 * data->gapRight;
    else if (data->gapRight == 0)
      len = data->lenPP + 2 * data->gapLeft;
    else
      len = data->lenPP + data->gapLeft + data->gapRight;

    ma.lenLo.resize(QUALITY_NONE);
    ma.lenHi.resize(QUALITY_NONE);

    const unsigned d1 = static_cast<unsigned>(CAR_LEN_FACTOR_GREAT * len);
    ma.lenLo[QUALITY_ACTUAL_GAP] = len - d1;
    ma.lenHi[QUALITY_ACTUAL_GAP] = len + d1;

    const unsigned d2 = static_cast<unsigned>(CAR_LEN_FACTOR_GOOD * len);
    ma.lenLo[QUALITY_BY_SYMMETRY] = len - d2;
    ma.lenHi[QUALITY_BY_SYMMETRY] = len + d2;
  }
}


bool PeakPattern::addModelTargets(
  const CarModels& models,
  const unsigned indexModel,
  const bool symmetryFlag,
  const BordersType patternType)
{
  CarDetect car;
  models.getCar(car, indexModel);
  const unsigned weight = models.count(indexModel);

  list<unsigned> carPoints;
  models.getCarPoints(indexModel, carPoints);

  Target target;
  bool seenFlag = false;

  if (car.hasLeftGap())
  {
    if (target.fill(indexModel, weight, false, 
          rangeData.indexLeft, rangeData.indexRight, 
          patternType, carPoints))
    {
      targets.emplace_back(target);
      seenFlag = true;
    }
  }

  if (! symmetryFlag && car.hasRightGap())
  {
    if (target.fill(indexModel, weight, true, 
          rangeData.indexLeft, rangeData.indexRight, 
          patternType, carPoints))
    {
      targets.emplace_back(target);
      seenFlag = true;
    }
  }

  return seenFlag;
}


bool PeakPattern::guessBothDouble(
  const CarModels& models,
  const bool leftFlag)
{
  if (rangeData.qualWorst == QUALITY_NONE)
    return false;

  targets.clear();

  for (auto& ae1: modelsActive)
  {
    if (! ae1.fullFlag)
      continue;

    for (auto& ae2: modelsActive)
    {
      if (! ae2.fullFlag)
        continue;

      if (rangeData.lenRange >= 
          ae1.lenLo[rangeData.qualBest] + ae2.lenLo[rangeData.qualBest] &&
          rangeData.lenRange <= 
          ae1.lenHi[rangeData.qualBest] + ae2.lenHi[rangeData.qualBest])
      {
        if (leftFlag)
          PeakPattern::addModelTargets(models, ae1.index, 
            ae1.data->symmetryFlag, 
            BORDERS_DOUBLE_SIDED_DOUBLE);
        else
          PeakPattern::addModelTargets(models, ae2.index, 
            ae2.data->symmetryFlag, 
            BORDERS_DOUBLE_SIDED_DOUBLE);
      }
    }
  }

  return (! targets.empty());
}


bool PeakPattern::guessNoBorders(const CarModels& models)
{
  // This is a half-hearted try to fill in exactly one car of the 
  // same type as its neighbors if those neighbors do not have any
  // edge gaps at all.

  // TODO A lot of these seem to be misalignments of cars with peaks.
  // So it's not clear that we should recognize them.

  UNUSED(models);

  if (! carBeforePtr || ! carAfterPtr)
    return false;

  if (carBeforePtr->index() != carAfterPtr->index())
    return false;

  if (rangeData.qualLeft != QUALITY_NONE ||
      rangeData.qualRight != QUALITY_NONE)
    return false;


  const CarPeaksPtr& peaksBefore = carBeforePtr->getPeaksPtr();
  const unsigned b1 = peaksBefore.firstBogieLeftPtr->getIndex();
  const unsigned b2 = peaksBefore.firstBogieRightPtr->getIndex();
  const unsigned b3 = peaksBefore.secondBogieLeftPtr->getIndex();
  const unsigned b4 = peaksBefore.secondBogieRightPtr->getIndex();

  const CarPeaksPtr& peaksAfter = carAfterPtr->getPeaksPtr();
  const unsigned a1 = peaksAfter.firstBogieLeftPtr->getIndex();
  const unsigned a2 = peaksAfter.firstBogieRightPtr->getIndex();
  const unsigned a3 = peaksAfter.secondBogieLeftPtr->getIndex();
  const unsigned a4 = peaksAfter.secondBogieRightPtr->getIndex();

  const unsigned avgLeftLeft = (b1 + a1) / 2;
  const unsigned avgLeftRight = (b2 + a2) / 2;
  const unsigned avgRightLeft = (b3 + a3) / 2;
  const unsigned avgRightRight = (b4 + a4) / 2;

  // Disqualify if the resulting car is implausible.
  // We will not test for symmetry.
  if (avgLeftLeft < b4 || avgRightRight > a1)
    return false;

  const unsigned delta = avgLeftLeft - b4;
  const unsigned bogieGap = carBeforePtr->getLeftBogieGap();

  // It is implausible for the intra-car gap to be too large.
  if (delta > CAR_NO_BORDER_FACTOR * bogieGap)
    return false;

  // It is implausible for the intra-car gap to be tiny.
  if (delta < CAR_SMALL_BORDER_FACTOR * bogieGap)
    return false;

  // It is implausible for the intra-car gap to exceed a mid gap.
  if (delta > carBeforePtr->getMidGap())
    return false;

  targets.clear();
  targets.emplace_back(Target());
  Target& target = targets.back();

  const unsigned start = avgLeftLeft - delta/2;
  const unsigned end = (a1 - avgRightRight) / 2;

  list<unsigned> carPoints;
  carPoints.push_back(0);
  carPoints.push_back(avgLeftLeft - start);
  carPoints.push_back(avgLeftRight - start);
  carPoints.push_back(avgRightLeft - start);
  carPoints.push_back(avgRightRight - start);
  carPoints.push_back(end - start);

  target.fill(
    carBeforePtr->index(), 1, false, start, end, BORDERS_NONE, carPoints);
    
  return true;
}


bool PeakPattern::guessBothSingle(const CarModels& models)
{
  if (rangeData.qualWorst == QUALITY_NONE)
    return false;

  targets.clear();
  for (auto& ae: modelsActive)
  {
    if (! ae.fullFlag)
      continue;

    if (rangeData.lenRange >= ae.lenLo[rangeData.qualBest] &&
        rangeData.lenRange <= ae.lenHi[rangeData.qualBest])
    {
      PeakPattern::addModelTargets(models, ae.index, 
        ae.data->symmetryFlag, BORDERS_DOUBLE_SIDED_SINGLE);
    }
  }
  return (! targets.empty());
}


bool PeakPattern::guessBothSingleShort(const CarModels& models)
{
  UNUSED(models);

  if (rangeData.qualWorst == QUALITY_NONE)
    return false;

  unsigned lenTypical = 2 * (sideTypical + bogieTypical) + longTypical;
  unsigned lenShortLo = static_cast<unsigned>
    (CAR_SHORT_FACTOR_LO * lenTypical);
  unsigned lenShortHi = static_cast<unsigned>
    (CAR_SHORT_FACTOR_HI * lenTypical);

  if (rangeData.lenRange < lenShortLo ||
      rangeData.lenRange > lenShortHi)
    return false;

  targets.clear();
  Target target;

  // Guess that particularly the middle part is shorter in a short car.
  list<unsigned> carPoints;
  carPoints.push_back(0);
  carPoints.push_back(sideTypical);
  carPoints.push_back(sideTypical + bogieTypical);
  carPoints.push_back(rangeData.lenRange - sideTypical - bogieTypical);
  carPoints.push_back(rangeData.lenRange - sideTypical);
  carPoints.push_back(rangeData.lenRange);

  if (target.fill(
    0, // Doesn't matter
    1, false, rangeData.indexLeft, rangeData.indexRight,
    BORDERS_DOUBLE_SIDED_SINGLE_SHORT, carPoints))
  {
    targets.emplace_back(target);
  }

  return (! targets.empty());
}


bool PeakPattern::guessBothDoubleLeft(const CarModels& models)
{
  return PeakPattern::guessBothDouble(models, true);
}


bool PeakPattern::guessBothDoubleRight(const CarModels& models)
{
  return PeakPattern::guessBothDouble(models, false);
}


bool PeakPattern::guessLeft(const CarModels& models)
{
  // Starting from the left, so open towards the end.

  if (carBeforePtr == nullptr)
    return false;

  if (rangeData.qualLeft == QUALITY_NONE)
    return false;

  targets.clear();
  for (auto& ae: modelsActive)
  {
    PeakPattern::addModelTargets(models, ae.index, ae.data->symmetryFlag,
      BORDERS_SINGLE_SIDED_LEFT);
  }
  return (! targets.empty());
}


bool PeakPattern::guessRight(const CarModels& models)
{
  // Starting from the right, so open towards the beginning.

  if (carAfterPtr == nullptr)
    return false;

  if (rangeData.qualRight == QUALITY_NONE)
    return false;

  if (rangeData.gapRight >= 
      carAfterPtr->getPeaksPtr().firstBogieLeftPtr->getIndex())
    return false;

  targets.clear();
  for (auto& ae: modelsActive)
  {
    PeakPattern::addModelTargets(models, ae.index, ae.data->symmetryFlag,
      BORDERS_SINGLE_SIDED_RIGHT);
  }
  return (! targets.empty());
}


bool PeakPattern::looksEmptyFirst(const PeakPtrs& peakPtrsUsed) const
{
  if (rangeData.qualRight == QUALITY_NONE ||
      rangeData.qualLeft != QUALITY_NONE)
    return false;

  if (peakPtrsUsed.size() >= 2)
    return false;

  Peak const * pptr = peakPtrsUsed.front();
  if (pptr->goodQuality())
    return false;

  // This could become more elaborate, e.g. the peak is roughly
  // in an expected position.

  return true;
}


void PeakPattern::update(
  const vector<Peak const *>& closestPtrs,
  const unsigned limitLower,
  const unsigned limitUpper,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  peakPtrsUsed.moveOut(closestPtrs, peakPtrsUnused);
  peakPtrsUnused.truncate(limitLower, limitUpper);
}


void PeakPattern::targetsToCompletions(PeakPtrs& peakPtrsUsed)
{
  completions.reset();

  vector<Peak *> peaksClose;
  unsigned numClose;
  unsigned dist;

  for (auto& target: targets)
  {
    peakPtrsUsed.getClosest(target.indices(), peaksClose, numClose, dist);

    unsigned limitLower, limitUpper;
    target.limits(limitLower, limitUpper);

    CarCompletion& carCompl = completions.emplace_back();
    carCompl.setLimits(limitLower, limitUpper);

    const unsigned bogieTolerance = static_cast<unsigned>
      (CAR_BOGIE_TOLERANCE * target.bogieGap());

    for (unsigned i = 0; i < peaksClose.size(); i++)
    {
      if (peaksClose[i] == nullptr)
        carCompl.addMiss(target.index(i), bogieTolerance);
      else
        carCompl.addMatch(target.index(i), peaksClose[i]);
    }
  }
}


void PeakPattern::annotateCompletions(
  PeakPool& peaks,
  PeakPtrs& peakPtrsUnused,
  const bool forceFlag)
{
  // Mark up with relevant, unused peaks.
  for (auto pptr: peakPtrsUnused)
    completions.markWith(* pptr, COMP_UNUSED);

  // Mark up with repairable peaks.
  completions.makeRepairables();

  Peak peakRep;
  while (completions.nextRepairable(peakRep))
  {
    // Make a test run without actually repairing anything.
    unsigned testIndex;
    peaks.repair(peakRep, &Peak::borderlineQuality, offset,
      true, forceFlag, testIndex);

    if (testIndex > 0)
    {
      peakRep.logPosition(testIndex, testIndex, testIndex);
      completions.markWith(peakRep, COMP_REPAIRABLE);
    }
  }

  // If there is a systematic shift, correct for it.
  completions.makeShift();
}


void PeakPattern::fillCompletions(
  PeakPool& peaks,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused,
  const bool forceFlag)
{
  Peak peakRep;

  for (auto& cc: completions)
  {
    for (auto& pc: cc)
    {
      if (pc.source() == COMP_UNUSED)
      {
        Peak * ptr = pc.ptr();
        peakPtrsUsed.add(ptr);
        peakPtrsUnused.remove(ptr);
      }
      else if (pc.source() == COMP_REPAIRABLE)
      {
        unsigned testIndex;
        pc.fill(peakRep);
        Peak * ptr = peaks.repair(peakRep, &Peak::borderlineQuality, 
          offset, false, forceFlag, testIndex);

        if (! ptr)
          THROW(ERR_PATTERN_BAD_FIX, "Peak not repairable after all?");

        peakPtrsUsed.add(ptr);
        completions.markWith(* ptr, COMP_REPAIRED);
      }
    }
  }
}


bool PeakPattern::fix(
  PeakPool& peaks,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused,
  const bool forceFlag)
{
  if (targets.empty())
    return false;

  PeakPattern::targetsToCompletions(peakPtrsUsed);

  PeakPattern::annotateCompletions(peaks, peakPtrsUnused, forceFlag);

  PeakPattern::fillCompletions(peaks, peakPtrsUsed, peakPtrsUnused, 
    forceFlag);

  completions.condense();

  cout << "Completions\n\n";
  cout << completions.str(offset);

  CarCompletion * winnerPtr;
  const unsigned numComplete = completions.numComplete(winnerPtr);

  vector<Peak const *> closestPtrs;
  unsigned limitLower, limitUpper;

  if (numComplete == 1)
  {
    cout << "COMPLETION MATCH" << endl;

    limitLower = 0;
    limitUpper = 0;
    winnerPtr->getMatch(closestPtrs, limitLower, limitUpper);

    PeakPattern::update(closestPtrs, limitLower, limitUpper, 
      peakPtrsUsed, peakPtrsUnused);

    return true;
  }
  else if (numComplete > 1)
  {
    cout << "COMPLETION ABUNDANCE " << numComplete << endl;
    if (forceFlag)
      cout << "FORCE ABUNDANCE " << numComplete << endl;
  }
  else
  {
    cout << "COMPLETION MISS" << endl;
    if (forceFlag)
      cout << "FORCE MISS" << endl;
  }

  return false;
}


FindCarType PeakPattern::locate(
  const CarModels& models,
  const PeakRange& range,
  const unsigned offsetIn,
  PeakPool& peaks,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused)
{
  if (! PeakPattern::setGlobals(models, range, offsetIn))
    return FIND_CAR_NO_MATCH;

  cout << peakPtrsUsed.strQuality("Used", offset);
  cout << peakPtrsUnused.strQuality("Unused", offset);

  PeakPattern::getActiveModels(models);

  for (auto& fgroup: patternMethods)
  {
    if ((this->* fgroup.fptr)(models))
    {
      cout << "Try pattern " << fgroup.name <<  endl;
      if (PeakPattern::fix(peaks, peakPtrsUsed, peakPtrsUnused, 
          fgroup.forceFlag))
      {
        cout << "Hit pattern " << fgroup.name << endl;
        return FIND_CAR_MATCH;
      }
    }
  }

  if (PeakPattern::looksEmptyFirst(peakPtrsUsed))
  {
    peakPtrsUnused.moveFrom(peakPtrsUsed);
    return FIND_CAR_DOWNGRADE;
  }

  return FIND_CAR_NO_MATCH;
}

