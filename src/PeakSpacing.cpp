#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "CarModels.h"
#include "PeakSpacing.h"
#include "PeakRange.h"
#include "Except.h"
#include "struct.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


PeakSpacing::PeakSpacing()
{
  PeakSpacing::reset();
}


PeakSpacing::~PeakSpacing()
{
}


void PeakSpacing::reset()
{
}


bool PeakSpacing::setGlobals(
  const CarModels& models,
  const PeakRange& range,
  const unsigned offsetIn)
{
  if (models.size() == 0)
    return false;

  offset = offsetIn;

  models.getTypical(bogieTypical, longTypical, sideTypical);
  if (sideTypical == 0)
    sideTypical = bogieTypical;

  if (range.characterize(models, rangeData))
  {
    cout << "\n" << rangeData.str("Range globals", offset) << "\n";
    return true;
  }
  else
    return false;
}


// TODO These two methods can go in PeakPtrs

void PeakSpacing::updateUnused(
  const Target& target,
  PeakPtrs& peakPtrsUnused) const
{
  unsigned limitLower, limitUpper;
  target.limits(limitLower, limitUpper);
  if (limitLower)
    peakPtrsUnused.erase_below(limitLower);
  if (limitUpper)
    peakPtrsUnused.erase_above(limitUpper);
}


void PeakSpacing::updateUsed(
  const vector<Peak const *>& peaksClosest,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  // There may be some peaks that have to be moved.
  auto pu = peakPtrsUsed.begin();
  for (auto& peak: peaksClosest)
  {
    if (peak == nullptr)
      continue;

    const unsigned indexClose = peak->getIndex();
    while (pu != peakPtrsUsed.end() && (* pu)->getIndex() < indexClose)
    {
      peakPtrsUnused.add(* pu);
      pu = peakPtrsUsed.erase(pu);
    }

    if (pu == peakPtrsUsed.end())
      break;

    // Preserve those peaks that are also in peaksClose.
    if ((* pu)->getIndex() == indexClose)
      pu = peakPtrsUsed.next(pu);
  }

  // Erase trailing peaks.
  while (pu != peakPtrsUsed.end())
  {
    peakPtrsUnused.add(* pu);
    pu = peakPtrsUsed.erase(pu);
  }
}


void PeakSpacing::update(
  const NoneEntry& none,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  PeakSpacing::updateUsed(none.peaksClose, peakPtrsUsed, peakPtrsUnused);
  PeakSpacing::updateUnused(none.pe, peakPtrsUnused);
}


void PeakSpacing::getSpacings(PeakPtrs& peakPtrsUsed)
{
  for (auto pit = peakPtrsUsed.begin(); pit != prev(peakPtrsUsed.end()); 
      pit++)
  {
    spacings.emplace_back(SpacingEntry());
    SpacingEntry& se = spacings.back();

    se.peakLeft = * pit;
    se.peakRight = * next(pit);
    se.dist = se.peakRight->getIndex() - se.peakLeft->getIndex();

    se.numBogies = (bogieTypical == 0 ? 1.f : 
      se.dist / static_cast<float>(bogieTypical));
    // TODO #define
    se.bogieLikeFlag = (se.numBogies >= 0.7f && se.numBogies <= 1.75f);

    const PeakQuality pqWL = se.peakLeft->getQualityWhole();
    const PeakQuality pqWR = se.peakRight->getQualityWhole();
    se.qualityWholeLower = (pqWL >= pqWR ? pqWL : pqWR);

    const PeakQuality pqSL = se.peakLeft->getQualityShape();
    const PeakQuality pqSR = se.peakRight->getQualityShape();
    se.qualityShapeLower = (pqSL >= pqSR ? pqSL : pqSR);

  }

  for (auto& sit: spacings)
    cout << sit.str(offset);
}


bool PeakSpacing::plausibleCar(
  const bool leftFlag,
  const unsigned index1,
  const unsigned index2) const
{
  const float bogieRatio = 
    spacings[index1].numBogies / spacings[index2].numBogies;
  if (bogieRatio < 0.67f || bogieRatio > 1.5f)
  {
cout << "FAIL on bogie ratio: " << bogieRatio << "\n";
    return false;
  }

  const float bogieAvg = static_cast<float>(
    (spacings[index1].dist + spacings[index2].dist)) / 2.f;
  const unsigned mid = spacings[index2].peakLeft->getIndex() -
    spacings[index1].peakRight->getIndex();
  const float midRatio = static_cast<float>(mid) / bogieAvg;
  if (midRatio < 1.2f || midRatio > 10.f)
  {
cout << "FAIL on mid ratio\n";
    return false;
  }

  unsigned gap;
  if (leftFlag)
    gap = spacings[index1].peakLeft->getIndex() - rangeData.indexLeft;
  else
    gap = rangeData.indexRight - spacings[index2].peakRight->getIndex();
  const float gapRatio = static_cast<float>(gap) / bogieAvg;
  if (gapRatio < 0.3f || gapRatio > 2.f)
  {
cout << "FAIL on gap ratio: gap " << gap << endl;
    return false;
  }

  // TODO Also check quality in some cases
  return true;
}


void PeakSpacing::fixShort(
  const string& text,
  const unsigned indexFirst,
  const unsigned indexLast,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  NoneEntry none;
  none.peaksClose.push_back(spacings[indexFirst].peakLeft);
  none.peaksClose.push_back(spacings[indexFirst].peakRight);
  none.peaksClose.push_back(spacings[indexLast].peakLeft);
  none.peaksClose.push_back(spacings[indexLast].peakRight);

  BordersType border;

  if (rangeData.gapLeft)
  {
    border = (rangeData.gapRight ?
      BORDERS_DOUBLE_SIDED_SINGLE_SHORT :
      BORDERS_SINGLE_SIDED_LEFT);
  }
  else
    border = BORDERS_SINGLE_SIDED_RIGHT;

  list<unsigned> carPoints;

  TargetData tdata;
  tdata.modelNo = 0; // Doesn't matter
  tdata.reverseFlag = false;
  tdata.weight = 1; // Doesn't matter
  tdata.forceFlag = false;  // Doesn't matter

  none.pe.fill(
    tdata,
    spacings[indexFirst].peakLeft->getIndex(),
    spacings[indexLast].peakRight->getIndex(),
    border,
    carPoints); // Doesn't matter

cout << text << ": " <<
  none.peaksClose[0]->getIndex() + offset << ", " <<
  none.peaksClose[1]->getIndex() + offset << ", " <<
  none.peaksClose[2]->getIndex() + offset << ", " <<
  none.peaksClose[3]->getIndex() + offset << endl;

  PeakSpacing::update(none, peakPtrsUsed, peakPtrsUnused);
}


bool PeakSpacing::guessAndFixShortFromSpacings(
  const string& text,
  const bool leftFlag,
  const unsigned indexFirst,
  const unsigned indexLast,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  const SpacingEntry& spFirst = spacings[indexFirst];
  const SpacingEntry& spLast = spacings[indexLast];

  if (spFirst.bogieLikeFlag &&
      spLast.bogieLikeFlag &&
      spFirst.qualityWholeLower <= PEAK_QUALITY_GOOD &&
      spLast.qualityWholeLower <= PEAK_QUALITY_GOOD &&
      PeakSpacing::plausibleCar(leftFlag, indexFirst, indexLast))
  {
    PeakSpacing::fixShort(text, indexFirst, indexLast,
      peakPtrsUsed, peakPtrsUnused);
    return true;
  }
  else
    return false;
}


bool PeakSpacing::guessAndFixShort(
  const bool leftFlag,
  const unsigned indexFirst,
  const unsigned indexLast,
  PeakPool& peaks,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  const unsigned numSpaces = indexLast + 1 - indexFirst;
cout << "NUMSPACES " << numSpaces << endl;

  const SpacingEntry& spFirst = spacings[indexFirst];
  const SpacingEntry& spLast = spacings[indexLast];

  if (numSpaces == 2)
  {
    // TODO Could extend
    /*
    if (rangeData.qualWorst != QUALITY_NONE)
    {
      cout << "PPDOUBLE 2\n";
    }
    else
    {
      cout << "PPSINGLE 2\n";
    }
    */
  }
  else if (numSpaces == 3)
  {
    // Could simply be one car.
    return PeakSpacing::guessAndFixShortFromSpacings("PPINDEX 3",
      leftFlag, indexFirst, indexLast, peakPtrsUsed, peakPtrsUnused);
  }
  else if (numSpaces == 4)
  {
    // There could be one spurious peak in the middle.
    if (spFirst.bogieLikeFlag &&
        spLast.bogieLikeFlag)
    {
      if (spFirst.qualityWholeLower <= PEAK_QUALITY_GOOD &&
          spLast.qualityWholeLower <= PEAK_QUALITY_GOOD &&
          PeakSpacing::plausibleCar(leftFlag, indexFirst, indexLast))
      {
        PeakSpacing::fixShort("PPINDEX 4a", indexFirst, indexLast,
          peakPtrsUsed, peakPtrsUnused);
        return true;
      }
      else if (spFirst.qualityShapeLower <= PEAK_QUALITY_GOOD &&
          spLast.qualityShapeLower <= PEAK_QUALITY_GOOD &&
          PeakSpacing::plausibleCar(leftFlag, indexFirst, indexLast))
      {
        PeakSpacing::fixShort("PPINDEX 4b", indexFirst, indexLast,
          peakPtrsUsed, peakPtrsUnused);
        return true;
      }
    }
  }
  else if (numSpaces == 5)
  {
cout << "TRYING RIGHT5\n";
cout << "LEFT " << leftFlag << "\n";
    if (leftFlag)
    {
cout << indexFirst << " " << indexLast-2 << "\n";
      // There could be a simple car at the left end.
      return PeakSpacing::guessAndFixShortFromSpacings("PPINDEX 4c",
        leftFlag, indexFirst, indexLast-2, peakPtrsUsed, peakPtrsUnused);
    }
    else
    {
cout << indexFirst+2 << " " << indexLast << "\n";
      // There could be a simple car at the right end.
      return PeakSpacing::guessAndFixShortFromSpacings("PPINDEX 4d",
        leftFlag, indexFirst+2, indexLast, peakPtrsUsed, peakPtrsUnused);
    }
  }

  UNUSED(peaks);

  return false;
}


bool PeakSpacing::guessAndFixShortLeft(
  PeakPool& peaks,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  if (rangeData.qualLeft == QUALITY_NONE)
    return false;

  cout << "Trying guessAndFixShortLeft\n";

  const unsigned ss = spacings.size();
  return PeakSpacing::guessAndFixShort(
    true, 0, (ss > 4 ? 4 : ss-1), peaks, peakPtrsUsed, peakPtrsUnused);
}


bool PeakSpacing::guessAndFixShortRight(
  PeakPool& peaks,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  if (rangeData.qualRight == QUALITY_NONE || 
      (rangeData.qualLeft != QUALITY_NONE && peakPtrsUsed.size() <= 6))
    return false;

  cout << "Trying guessAndFixShortRight\n";

  const unsigned ss = spacings.size();
  return PeakSpacing::guessAndFixShort(
    false, (ss <= 5 ? 0 : ss-5), ss-1, peaks, peakPtrsUsed, peakPtrsUnused);
}


bool PeakSpacing::locate(
  const CarModels& models,
  PeakPool& peaks,
  const PeakRange& range,
  const unsigned offsetIn,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused)
{
  if (! PeakSpacing::setGlobals(models, range, offsetIn))
    return false;

cout << peakPtrsUsed.strQuality("Used", offset);
cout << peakPtrsUnused.strQuality("Unused", offset);

  if (peakPtrsUsed.size() <= 2)
    return false;

  if (rangeData.qualBest != QUALITY_NONE)
  {
    // Make an attempt to find short cars without a model.
    PeakSpacing::getSpacings(peakPtrsUsed);

    if (PeakSpacing::guessAndFixShortLeft(peaks, 
        peakPtrsUsed, peakPtrsUnused))
      return true;

    if (PeakSpacing::guessAndFixShortRight(peaks, 
        peakPtrsUsed, peakPtrsUnused))
      return true;

  }

  return false;
}

