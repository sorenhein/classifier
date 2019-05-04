#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "PeakPattern.h"
#include "PeakRange.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))

// If we try to interpolate an entire car, the end gap should not be
// too large relative to the bogie gap.
#define NO_BORDER_FACTOR 3.0f

// The inter-car gap should be larger than effectively zero.
#define SMALL_BORDER_FACTOR 0.1f

// Limits for match between model and actual gap.
#define LEN_FACTOR_GREAT 0.05f
#define LEN_FACTOR_GOOD 0.10f


PeakPattern::PeakPattern()
{
  PeakPattern::reset();
}


PeakPattern::~PeakPattern()
{
}


void PeakPattern::reset()
{
  fullEntries.clear();
}


bool PeakPattern::getRangeQuality(
  const CarModels& models,
  const PeakRange& range,
  CarDetect const * carPtr,
  const bool leftFlag,
  RangeQuality& quality,
  unsigned& gap) const
{
  gap = 0;
  quality = QUALITY_NONE;

  bool hasGap = (leftFlag ? range.hasLeftGap() : range.hasRightGap());
  if (hasGap)
  {
    cout << "SUGGEST-ERR: Range already has gap\n";
    return false;
  }

  if (! carPtr)
  {
    cout << "SUGGEST-ERR: Expected non-null car pointer\n";
    return false;
  }

  // The previous car could be contained in a whole car.
  const unsigned modelIndex = carPtr->getMatchData()->index;
  ModelData const * data = models.getData(modelIndex);
  const bool revFlag = carPtr->isReversed();

  if (data->fullFlag)
  {
    cout << "SUGGEST-ERR: Car should not already be full\n";
    return false;
  }

  if (leftFlag)
  {
    if ((revFlag && data->gapLeftFlag) || 
        (! revFlag && data->gapRightFlag))
    {
      cout << "SUGGEST-ERR: Left car should not have abutting gap\n";
      cout << data->str();
      return false;
    }
  }
  else
  {
    if ((revFlag && data->gapRightFlag) ||
        (! revFlag && data->gapLeftFlag))
    {
      cout << "SUGGEST-ERR: Right car should not have abutting gap\n";
      cout << data->str();
      return false;
    }
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
    cout << "SUGGEST-ERR: No general gap (yet?)\n";
    return false;
  }

  return true;
}


bool PeakPattern::guessNoBorders(list<PatternEntry>& candidates) const
{
  // This is a half-hearted try to fill in exactly one car of the 
  // same type as its neighbors if those neighbors do not have any
  // edge gaps at all.

  if (! carBeforePtr || ! carAfterPtr)
  {
    cout << "SUGGEST-ERR: guessNoBorders one+ pointer is null\n";
    return false;
  }


  if (carBeforePtr->index() != carAfterPtr->index())
  {
    cout << "SUGGEST-ERR: guessNoBorders cars are different\n";
    return false;
  }

  // We will not test for symmetry.
  const CarPeaksPtr& peaksBefore = carBeforePtr->getPeaksPtr();
  const CarPeaksPtr& peaksAfter = carAfterPtr->getPeaksPtr();

  const unsigned bogieGap = carBeforePtr->getLeftBogieGap();

  const unsigned avgLeftLeft = 
    (peaksBefore.firstBogieLeftPtr->getIndex() +
     peaksAfter.firstBogieLeftPtr->getIndex()) / 2;
  const unsigned avgLeftRight =
    (peaksBefore.firstBogieRightPtr->getIndex() +
     peaksAfter.firstBogieRightPtr->getIndex()) / 2;
  const unsigned avgRightLeft =
    (peaksBefore.secondBogieLeftPtr->getIndex() +
     peaksAfter.secondBogieLeftPtr->getIndex()) / 2;
  const unsigned avgRightRight =
    (peaksBefore.secondBogieRightPtr->getIndex() +
     peaksAfter.secondBogieRightPtr->getIndex()) / 2;

  // Disqualify if the resulting car is implausible.
  if (avgLeftLeft < peaksBefore.secondBogieRightPtr->getIndex())
  {
    cout << "SUGGEST-ERR: guessNoBorders left gap way too small\n";
    cout << "SUGGEST-YY0: avgLL " << avgLeftLeft << 
      " vs. " << peaksBefore.secondBogieRightPtr->getIndex() + offset << 
      endl;
    return false;
  }

  if (avgRightRight > peaksAfter.firstBogieLeftPtr->getIndex())
  {
    cout << "SUGGEST-ERR: guessNoBorders right gap way too small\n";
    cout << "SUGGEST-YY1: avgRR " << avgRightRight << 
      " vs. " << peaksAfter.firstBogieLeftPtr->getIndex() + offset << endl;
    return false;
  }

  const unsigned delta = 
    avgLeftLeft - peaksBefore.secondBogieRightPtr->getIndex();

  if (delta > NO_BORDER_FACTOR * bogieGap)
  {
    cout << "SUGGEST-ERR: guessNoBorders cars are different vs bogie\n";
    cout << "SUGGEST-YY2: gap " << delta << " vs. " << bogieGap << endl;
    return false;
  }

  if (delta < SMALL_BORDER_FACTOR * bogieGap)
  {
    cout << "SUGGEST-ERR: guessNoBorders left gap tiny\n";
    cout << "SUGGEST-YY3: gap " << delta << " vs. " << bogieGap << endl;
    return false;
  }

  const unsigned mid = carBeforePtr->getMidGap();
  if (delta > mid)
  {
    cout << "SUGGEST-ERR: guessNoBorders cars are different vs mid gap\n";
    cout << "SUGGEST-YY4: gap " << delta << " vs. " << mid << endl;
    return false;
  }

  candidates.emplace_back(PatternEntry());
  PatternEntry& pe = candidates.back();

  pe.modelNo = carBeforePtr->index();
  pe.reverseFlag = false;
  pe.abutLeftFlag = false;
  pe.abutRightFlag = false;
  pe.start = avgLeftLeft - delta/2;
  pe.end = (peaksAfter.firstBogieLeftPtr->getIndex() - avgRightRight) / 2;

  pe.indices.push_back(avgLeftLeft);
  pe.indices.push_back(avgLeftRight);
  pe.indices.push_back(avgRightLeft);
  pe.indices.push_back(avgRightRight);

  pe.borders = PATTERN_NO_BORDERS;

cout << "NOBORDER\n";
cout << pe.strAbs("SUGGEST-YY5", offset);

  return true;
}


void PeakPattern::getFullModels(const CarModels& models)
{
  for (unsigned index = 0; index < models.size(); index++)
  {
    if (models.empty(index))
      continue;

    ModelData const * data = models.getData(index);
    if (! data->fullFlag)
      continue;

cout << "TTT got model " << index << endl;
cout << data->str();

    fullEntries.emplace_back(FullEntry());
    FullEntry& fe = fullEntries.back();
    fe.data = data;
    fe.index = index;

    // Three different qualities; only two used for now.
    fe.lenLo.resize(3);
    fe.lenHi.resize(3);

    const unsigned len = data->lenPP + data->gapLeft + data->gapRight;

    fe.lenLo[QUALITY_WHOLE_MODEL] =
      static_cast<unsigned>((1.f - LEN_FACTOR_GREAT) * len);
    fe.lenHi[QUALITY_WHOLE_MODEL] =
      static_cast<unsigned>((1.f + LEN_FACTOR_GREAT) * len);

    fe.lenLo[QUALITY_SYMMETRY] = 
      static_cast<unsigned>((1.f - LEN_FACTOR_GOOD) * len);
    fe.lenHi[QUALITY_SYMMETRY] =
      static_cast<unsigned>((1.f + LEN_FACTOR_GOOD) * len);
  }
}


bool PeakPattern::fillPoints(
  const list<unsigned>& carPoints,
  const unsigned indexBase,
  const bool reverseFlag,
  PatternEntry& pe) const
{
  if (pe.abutLeftFlag)
  {
    pe.indices.resize(4);

    if (! reverseFlag)
    {
      unsigned pi = 0;
      for (auto i = next(carPoints.begin()); i != prev(carPoints.end()); 
          i++, pi++)
        pe.indices[pi] = indexBase + * i;
    }
    else
    {
      const unsigned pointLast = carPoints.back();
      unsigned pi = 3;
      for (auto i = next(carPoints.begin()); i != prev(carPoints.end()); 
          i++, pi--)
        pe.indices[pi] = indexBase + pointLast - * i;
    }
  }
  else
  {
    const unsigned pointLast = carPoints.back();
    const unsigned pointLastPeak = * prev(prev(carPoints.end()));
    const unsigned pointFirstPeak = * next(carPoints.begin());

    if (indexBase + pointFirstPeak <= pointLast)
    {
      cout << "TTT no room for left car\n";
      return false;
    }

    pe.indices.resize(4);

    if (! reverseFlag)
    {
      unsigned pi = 0;
      for (auto i = next(carPoints.begin()); i != prev(carPoints.end()); 
          i++, pi++)
        pe.indices[pi] = indexBase + * i - pointLast;
    }
    else
    {
      unsigned pi = 3;
      for (auto i = next(carPoints.begin()); i != prev(carPoints.end()); 
          i++, pi--)
        pe.indices[pi] = indexBase - * i;
    }
  }

  return true;
}


bool PeakPattern::fillFromModel(
  const CarModels& models,
  const unsigned indexModel,
  const bool symmetryFlag,
  const unsigned indexRangeLeft,
  const unsigned indexRangeRight,
  const PatternType patternType,
  list<PatternEntry>& candidates) const
{
  PatternEntry pe;

  pe.modelNo = indexModel;
  pe.reverseFlag = false;
  pe.abutLeftFlag = (indexRangeLeft != 0);
  pe.abutRightFlag = (indexRangeRight != 0);
  pe.start = indexRangeLeft;
  pe.end = indexRangeRight;
  pe.borders = patternType;

  list<unsigned> carPoints;
  models.getCarPoints(indexModel, carPoints);
  if (! PeakPattern::fillPoints(carPoints, 
      (pe.abutLeftFlag ? indexRangeLeft : indexRangeRight), false, pe))
    return false;

  candidates.emplace_back(pe);
cout << pe.strAbs("SUGGEST-ZZ1", offset);

  // Two options if asymmetric.
  if (symmetryFlag)
    return true;

  pe.reverseFlag = true;

  if (! PeakPattern::fillPoints(carPoints, 
      (pe.abutLeftFlag ? indexRangeLeft : indexRangeRight), true, pe))
    return false;

  candidates.emplace_back(pe);

cout << pe.strAbs("SUGGEST-ZZ1rev", offset);

  return true;
}


bool PeakPattern::guessLeft(
  const CarModels& models,
  list<PatternEntry>& candidates) const
{
  // Starting from the left, so open towards the end.
  const unsigned indexLeft = 
    carBeforePtr->getPeaksPtr().secondBogieRightPtr->getIndex() + gapLeft;

cout << "TTT guessLeft actual abutting peak: " <<
  carBeforePtr->getPeaksPtr().secondBogieRightPtr->getIndex() + offset << 
  "\n";

  if (qualLeft == QUALITY_GENERAL || qualLeft == QUALITY_NONE)
  {
    cout << "SUGGEST-ERR: guessLeft unexpected quality\n";
    return false;
  }

  for (auto& fe: fullEntries)
  {
cout << " TTT got left model: " << fe.index << endl;
    PeakPattern::fillFromModel(models, fe.index, fe.data->symmetryFlag,
      indexLeft, 0, PATTERN_SINGLE_SIDED_LEFT, candidates);
  }

  return (candidates.size() > 0);
}


bool PeakPattern::guessRight(
  const CarModels& models,
  list<PatternEntry>& candidates) const
{
  // Starting from the right, so open towards the beginning.
  if (gapRight >= carAfterPtr->getPeaksPtr().firstBogieLeftPtr->getIndex())
  {
    cout << "SUGGEST-ERR: guessRight no room\n";
    return false;
  }

  const unsigned indexRight = 
    carAfterPtr->getPeaksPtr().firstBogieLeftPtr->getIndex() - gapRight;

cout << "TTT guessRight actual abutting peak: " <<
  carAfterPtr->getPeaksPtr().firstBogieLeftPtr->getIndex() + offset << "\n";

  if (qualRight == QUALITY_GENERAL || qualRight == QUALITY_NONE)
  {
    cout << "SUGGEST-ERR: guessRight unexpected quality\n";
    return false;
  }

  for (auto& fe: fullEntries)
  {
cout << " TTT got right model: " << fe.index << endl;
    PeakPattern::fillFromModel(models, fe.index, fe.data->symmetryFlag,
      0, indexRight, PATTERN_SINGLE_SIDED_RIGHT, candidates);
  }

  return (candidates.size() > 0);
}


bool PeakPattern::findSingleModel(
  const RangeQuality qualOverall,
  const unsigned lenRange,
  FullEntry const *& fep) const
{
  unsigned count = 0;
  for (auto& fe: fullEntries)
  {
cout << "TTT comparing to model " << fe.index << ": lenPP " <<
  fe.data->lenPP << ", " << fe.lenLo[qualOverall] << ", " <<
  fe.lenHi[qualOverall] << endl;

    if (lenRange >= fe.lenLo[qualOverall] &&
        lenRange <= fe.lenHi[qualOverall])
    {
      count++;
      fep = &fe;
    }
  }

  if (count == 0)
    return false;
  else if (count >= 2)
  {
    cout << "SUGGEST-ERR: guessBoth several matches to single car\n";
    return false;
  }
  else
    return true;
}


bool PeakPattern::findDoubleModel(
  const RangeQuality qualOverall,
  const unsigned lenRange,
  list<FullEntry const *>& feps) const
{
  feps.clear();
  for (auto& fe1: fullEntries)
  {
    for (auto& fe2: fullEntries)
    {
      if (lenRange >= fe1.lenLo[qualOverall] + fe2.lenLo[qualOverall] &&
          lenRange <= fe1.lenHi[qualOverall] + fe2.lenHi[qualOverall])
      {
        // We just fill out the first model which is then implicitly
        // to the left (earlier in time) than the other one.
        feps.push_back(&fe1);
      }
    }
  }
  return (feps.size() > 0);
}


bool PeakPattern::guessBoth(
  const CarModels& models,
  list<PatternEntry>& candidates) const
{
  const unsigned indexLeft = 
    carBeforePtr->getPeaksPtr().secondBogieRightPtr->getIndex() + gapLeft;
  const unsigned indexRight = 
    carAfterPtr->getPeaksPtr().firstBogieLeftPtr->getIndex() - gapRight;

cout << "TTT actual abutting peaks: " <<
    carBeforePtr->getPeaksPtr().secondBogieRightPtr->getIndex() + offset <<
    ", " <<
    carAfterPtr->getPeaksPtr().firstBogieLeftPtr->getIndex() + offset << "\n";

  if (indexRight <= indexLeft)

  {
    cout << "SUGGEST-ERR: guessBoth no range\n";
    return false;
  }

  RangeQuality qualOverall = (qualLeft <= qualRight ? qualLeft : qualRight);
  if (qualOverall == QUALITY_GENERAL || qualOverall == QUALITY_NONE)
  {
    cout << "SUGGEST-ERR: guessBoth unexpected quality\n";
    return false;
  }

  const unsigned lenRange = indexRight - indexLeft;
cout << "TTT looking for actual length " << lenRange << endl;

  FullEntry const * fep = nullptr;
  if (PeakPattern::findSingleModel(qualOverall, lenRange, fep))
  {
    PeakPattern::fillFromModel(models, fep->index, 
      fep->data->symmetryFlag, indexLeft, indexRight, 
      PATTERN_DOUBLE_SIDED_SINGLE, candidates);

    return true;
  }

  list<FullEntry const *> feps;
  if (PeakPattern::findDoubleModel(qualOverall, lenRange, feps))
  {
cout << " TTT got double-models: " << feps.size() << endl;
    for (auto fp: feps)
    {
      PeakPattern::fillFromModel(models, fp->index, 
        fp->data->symmetryFlag, indexLeft, indexRight, 
        PATTERN_DOUBLE_SIDED_DOUBLE, candidates);
    }

    return true;
  }
  else
    return false;
}


bool PeakPattern::suggest(
  const CarModels& models,
  const PeakRange& range,
  const unsigned offsetIn,
  list<PatternEntry>& candidates)
{
  offset = offsetIn;

  carBeforePtr = range.carBeforePtr();
  carAfterPtr = range.carAfterPtr();

  if (! carBeforePtr && ! carAfterPtr)
  {
    cout << "SUGGEST-ERR: Entire range\n";
    return false;
  }


  if (! PeakPattern::getRangeQuality(models, range, carBeforePtr,
      true, qualLeft, gapLeft))
    qualLeft = QUALITY_NONE;

  if (! PeakPattern::getRangeQuality(models, range, carAfterPtr,
      false, qualRight, gapRight))
    qualRight = QUALITY_NONE;

cout << "SUGGESTX " << qualLeft << "-" << qualRight << "\n";
cout << "SUGGEST " << qualLeft << "-" << qualRight << ": " <<
  gapLeft << ", " << gapRight << endl;

  candidates.clear();

  // TODO A lot of these seem to be misalignments of cars with peaks.
  if (qualLeft == QUALITY_NONE && qualRight == QUALITY_NONE)
    return PeakPattern::guessNoBorders(candidates);

  PeakPattern::getFullModels(models);

  if (qualLeft == QUALITY_NONE)
    return PeakPattern::guessRight(models, candidates);
  else if (qualRight == QUALITY_NONE)
    return PeakPattern::guessLeft(models, candidates);
  else
    return PeakPattern::guessBoth(models, candidates);
}


bool PeakPattern::acceptable(
  PatternEntry const * pep,
  const vector<Peak const *>& peaksClose,
  const unsigned numClose) const
{
  UNUSED(pep);
  UNUSED(peaksClose);
  UNUSED(numClose);
  return false;
}


bool PeakPattern::recoverable(
  const PeakPool& peaks,
  const vector<unsigned>& indices,
  const vector<Peak const *>& peaksClose,
  float& dist) const
{
  UNUSED(peaks);
  UNUSED(indices);
  UNUSED(peaksClose);
  UNUSED(dist);
  return false;
}


void PeakPattern::recover(
  const PeakPool& peaks,
  const vector<unsigned>& indices,
  const vector<Peak const *>& peaksClose) const
{
  UNUSED(peaks);
  UNUSED(indices);
  UNUSED(peaksClose);
}


void PeakPattern::update(
  PatternEntry const * pep,
  const vector<Peak const *>& peaksClose,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  UNUSED(pep);
  UNUSED(peaksClose);
  UNUSED(peakPtrsUsed);
  UNUSED(peakPtrsUnused);
}


void PeakPattern::printClosest(
  const vector<unsigned>& indices,
  const vector<Peak const *>& peaksClose) const
{
  cout << "printClosest\n";
  for (unsigned i = 0; i < indices.size(); i++)
  {
    cout << setw(4) << left << i <<
      setw(8) << right << indices[i] + offset <<
      setw(8) << 
        (peaksClose[i] ? to_string(peaksClose[i]->getIndex() + offset) : 
          "-") << 
        endl;
  }
}


bool PeakPattern::verify(
  list<PatternEntry>& candidates,
  PeakPool& peaks,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  float distBest = numeric_limits<float>::max();
  PatternEntry const * candBest = nullptr;
  vector<Peak const *> peaksBest;

  for (auto& pe: candidates)
  {
    vector<Peak const *> peaksClose;
    unsigned numClose;
    PatternEntry const * pep = &pe;

    // TODO Write this method in PeakPtrs.cpp
    peakPtrsUsed.getClosest(pe.indices, peaksClose, numClose);

    // TMP
    PeakPattern::printClosest(pe.indices, peaksClose);

    if (! PeakPattern::acceptable(pep, peaksClose, numClose))
      continue;

    float dist;
    if (! PeakPattern::recoverable(peaks, pe.indices, peaksClose, dist))
      continue;

    if (dist < distBest)
    {
      distBest = dist;
      candBest = pep;
      peaksBest = peaksClose;
    }
  }

  if (candBest == nullptr)
    return false;

  PeakPattern::recover(peaks, candBest->indices, peaksBest);

  PeakPattern::update(candBest, peaksBest, peakPtrsUsed, peakPtrsUnused);

  return true;
}

