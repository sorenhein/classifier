#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cmath>

#include "Peak.h"
#include "PeakMinima.h"
#include "Except.h"


#define SLIDING_LOWER 0.9f
#define SLIDING_UPPER 1.1f
#define SLIDING_UPPER_SQ (SLIDING_UPPER * SLIDING_UPPER)

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


PeakMinima::PeakMinima()
{
  PeakMinima::reset();
}


PeakMinima::~PeakMinima()
{
}


void PeakMinima::reset()
{
  offset = 0;
}


void PeakMinima::findFirstLargeRange(
  const vector<unsigned>& dists,
  Gap& gap,
  const unsigned lowerCount) const
{
  // We consider a sliding window with range +/- 10% relative to its
  // center.  We want to find the first maximum (i.e. the value at which
  // the count of distances in dists within the range reaches a local
  // maximum).  This is a somewhat rough way to find the lowest
  // "cluster" of values.

  // If an entry in dists is d, then it creates two DistEntry values.
  // One is at 0.9 * d and is a +1, and one is a -1 at 1.1 * d.

  vector<DistEntry> steps;
  for (auto d: dists)
  {
    steps.emplace_back(DistEntry());
    DistEntry& de1 = steps.back();
    de1.index = static_cast<unsigned>(SLIDING_LOWER * d);
    de1.direction = 1;
    de1.origin = d;

    steps.emplace_back(DistEntry());
    DistEntry& de2 = steps.back();
    de2.index = static_cast<unsigned>(SLIDING_UPPER * d);
    de2.direction = -1;
    de2.origin = d;
  }

  sort(steps.begin(), steps.end());

/*
cout << "STEPS\n";
for (auto s: steps)
{
  cout << s.index << ";" << s.direction << "\n";
}
cout << "\n";
*/

  int bestCount = 0;
  unsigned bestValue = 0;
  unsigned bestUpperValue = 0;
  unsigned bestLowerValue = 0;
  int count = 0;
  unsigned dindex = steps.size();
  unsigned i = 0;

/*
cout << "STEPS\n";
while (i < dindex)
{
  // There could be several entries at the same index.
  const unsigned step = steps[i].index;
  unsigned upperValue = 0;
  while (i < dindex && steps[i].index == step)
  {
      count += steps[i].direction;
    // Note one origin (the +1 ones all have the same origin).
    if (steps[i].direction == 1)
      upperValue = steps[i].origin;
    i++;
  }
  cout << step << ";" << count << "\n";
}
*/

  count = 0;
  i = 0;
  while (i < dindex)
  {
    // There could be several entries at the same index.
    const unsigned step = steps[i].index;
    unsigned upperValue = 0;
    while (i < dindex && steps[i].index == step)
    {
      count += steps[i].direction;
      // Note one origin (the +1 ones all have the same origin).
      if (steps[i].direction == 1)
        upperValue = steps[i].origin;
      i++;
    }
// cout << step << ";" << count << "\n";

    if (count > bestCount)
    {
      bestCount = count;
      bestValue = step;
      bestUpperValue = upperValue;

      if (i == dindex)
        THROW(ERR_NO_PEAKS, "Does not come back to zero");

      // Locate the first of the following -1's and use its origin
      // (which will be lower than upperValue).
      unsigned j = i;
      while (j < dindex && steps[j].direction == 1)
        j++;

      if (j == dindex || steps[j].direction == 1)
        THROW(ERR_NO_PEAKS, "Does not come back to zero");

      bestLowerValue = steps[j].origin;
    }
    else if (bestCount > 0 && count == 0)
    {
      if (bestCount < static_cast<int>(lowerCount))
      {
        // Don't take a "small" maximum - start over instead.  
        // The caller decides the minimum size of the local maximum.  
        // We should probably be more discerning.
        bestCount = 0;
        bestUpperValue = 0;
        bestLowerValue = 0;
      }
      else
        break;
    }
  }

  // So now we have a range of values that are all equally good from
  // the point of view of covering as many steps as possible +/- 10%.
  // We make a useful interval out of this.

  const unsigned mid = (bestLowerValue + bestUpperValue) / 2;
  gap.lower = static_cast<unsigned>(mid * SLIDING_LOWER);
  gap.upper = static_cast<unsigned>(mid * SLIDING_UPPER);
  gap.count = static_cast<unsigned>(bestCount);
}


void PeakMinima::markWheelPair(
  Peak& p1,
  Peak& p2,
  const string& text) const
{
  if (text != "")
    PeakMinima::printRange(p1.getIndex(), p2.getIndex(),
      text + " wheel pair at");
    
  p1.select();
  p2.select();

  p1.markWheel(WHEEL_LEFT);
  p2.markWheel(WHEEL_RIGHT);
}



void PeakMinima::markBogieShortGap(
  Peak& p1,
  Peak& p2) const
{
  p1.markBogieAndWheel(BOGIE_RIGHT, WHEEL_RIGHT);
  p2.markBogieAndWheel(BOGIE_LEFT, WHEEL_LEFT);
}


void PeakMinima::markBogieLongGap(
  Peak& p1,
  Peak& p2) const
{
  p1.markBogieAndWheel(BOGIE_LEFT, WHEEL_RIGHT);
  p2.markBogieAndWheel(BOGIE_RIGHT, WHEEL_LEFT);
}


void PeakMinima::reseedWheelUsingQuality(PeakPool& peaks) const
{
  PeakPtrs& candidates = peaks.candidates();
  PPLiterator cbegin = candidates.begin();
  PPLiterator cend = candidates.end();
  for (auto cit = cbegin; cit != cend; cit++)
  {
    Peak * cand = * cit;
    if (cand->greatQuality())
      cand->select();
    else
    {
      cand->unselect();
      cand->markNoBogie();
      cand->markNoWheel();
    }
  }
}


void PeakMinima::reseedBogiesUsingQuality(
  PeakPool& peaks,
  const vector<Peak>& bogieScale) const
{
  PeakPtrs& candidates = peaks.candidates();
  PPLiterator cbegin = candidates.begin();
  PPLiterator cend = candidates.end();
  for (auto cit = cbegin; cit != cend; cit++)
  {
    Peak * cand = * cit;
    if (! cand->isWheel())
      cand->calcQualities(bogieScale);
    else if (cand->isLeftWheel())
      cand->calcQualities(bogieScale[0]);
    else if (cand->isRightWheel())
      cand->calcQualities(bogieScale[1]);

    if (cand->greatQuality())
      cand->select();
    else if (cand->isWheel() && cand->acceptableQuality())
      cand->select();
    else
    {
      cand->unselect();
      cand->markNoBogie();
      cand->markNoWheel();
    }
  }
}


void PeakMinima::reseedLongGapsUsingQuality(
  PeakPool& peaks,
  const vector<Peak>& longGapScale) const
{
  const PeakPtrs& candidates = peaks.candidatesConst();
  PPLciterator cbegin = candidates.cbegin();
  PPLciterator cend = candidates.cend();
  for (PPLciterator cit = cbegin; cit != cend; cit++)
  {
    Peak * cand = * cit;
    if (! cand->isWheel())
      cand->calcQualities(longGapScale);
    else if (cand->isLeftBogie())
    {
      if (cand->isLeftWheel())
        cand->calcQualities(longGapScale[0]);
      else if (cand->isRightWheel())
        cand->calcQualities(longGapScale[1]);
      else
      {
        // TODO
        // THROW(ERR_ALGO_PEAK_CONSISTENCY, "No wheel in bogie?");
      }
    }
    else if (cand->isRightBogie())
    {
      if (cand->isLeftWheel())
        cand->calcQualities(longGapScale[2]);
      else if (cand->isRightWheel())
        cand->calcQualities(longGapScale[3]);
      else
      {
        // TODO
        // THROW(ERR_ALGO_PEAK_CONSISTENCY, "No wheel in bogie?");
      }
    }
    else
    {
      // TODO
      // THROW(ERR_ALGO_PEAK_CONSISTENCY, "No bogie for wheel?");
    }

    if (cand->greatQuality())
      cand->select();
    else if (cand->isWheel() && cand->acceptableQuality())
      cand->select();
    else
    {
      cand->unselect();
      cand->markNoBogie();
      cand->markNoWheel();
    }
  }
}


void PeakMinima::makeWheelAverage(
  const PeakPool& peaks,
  Peak& wheel) const
{
  // TODO This and the next two methods could become a single PeakPool
  // method with an array of fptrs as argument.
  wheel.reset();

  unsigned count = 0;
  const PeakPtrs& candidates = peaks.candidatesConst();
  PPLciterator cbegin = candidates.cbegin();
  PPLciterator cend = candidates.cend();
  for (PPLciterator cit = cbegin; cit != cend; cit++)
  {
    Peak const * cand = * cit;
    if (cand->isSelected())
    {
      wheel += * cand;
      count++;
    }
  }

  if (count)
    wheel /= count;
}


void PeakMinima::makeBogieAverages(
  const PeakPool& peaks,
  vector<Peak>& wheels) const
{
  wheels.clear();
  wheels.resize(2);

  unsigned cleft = 0, cright = 0;
  const PeakPtrs& candidates = peaks.candidatesConst();
  PPLciterator cbegin = candidates.cbegin();
  PPLciterator cend = candidates.cend();
  for (PPLciterator cit = cbegin; cit != cend; cit++)
  {
    Peak const * cand = * cit;
    if (cand->isLeftWheel())
    {
      wheels[0] += * cand;
      cleft++;
    }
    else if (cand->isRightWheel())
    {
      wheels[1] += * cand;
      cright++;
    }
  }

  if (cleft)
    wheels[0] /= cleft;
  if (cright)
    wheels[1] /= cright;
}


void PeakMinima::makeCarAverages(
  const PeakPool& peaks,
  vector<Peak>& wheels) const
{

  wheels.clear();
  wheels.resize(4);

  vector<unsigned> count;
  count.resize(4);

  const PeakPtrs& candidates = peaks.candidatesConst();
  PPLciterator cbegin = candidates.cbegin();
  PPLciterator cend = candidates.cend();
  for (PPLciterator cit = cbegin; cit != cend; cit++)
  {
    Peak const * cand = * cit;
    if (cand->isLeftBogie())
    {
      if (cand->isLeftWheel())
      {
        wheels[0] += * cand;
        count[0]++;
      }
      else if (cand->isRightWheel())
      {
        wheels[1] += * cand;
        count[1]++;
      }
    }
    else if (cand->isRightBogie())
    {
      if (cand->isLeftWheel())
      {
        wheels[2] += * cand;
        count[2]++;
      }
      else if (cand->isRightWheel())
      {
        wheels[3] += * cand;
        count[3]++;
      }
    }
  }

  for (unsigned i = 0; i < 4; i++)
  {
    if (count[i])
      wheels[i] /= count[i];
  }
}


void PeakMinima::markSinglePeaks(
  PeakPool& peaks,
  const vector<Peak>& peakCenters) const
{
  if (peaks.candidates().size() == 0)
    THROW(ERR_NO_PEAKS, "No tall peaks");

  // Use the peak centers as a first yardstick for calculating qualities.
  PeakPtrs& candidates = peaks.candidates();
  PPLiterator cbegin = candidates.begin();
  PPLiterator cend = candidates.end();
  for (PPLiterator cit = cbegin; cit != cend; cit++)
    (* cit)->calcQualities(peakCenters);

  cout << peaks.candidates().strQuality("All negative minima", offset);
  cout << peaks.candidates().strQuality("Seeds", 
    offset, &Peak::isSelected);

  // Modify selection based on quality.
  PeakMinima::reseedWheelUsingQuality(peaks);

  cout << peaks.candidates().strQuality("Great-quality seeds", offset);
}


void PeakMinima::markBogiesOfSelects(
  PeakPool& peaks,
  const PeakFncPtr& fptr,
  const Gap& wheelGap,
  Gap& actualGap) const
{
  PeakPtrs& candidates = peaks.candidates();
  PPLiterator cbegin = candidates.begin();
  PPLiterator cend = candidates.end();
  actualGap.reset();

  // Here we mark bogies where both peaks are already selected.
  for (auto cit = cbegin; cit != prev(cend); cit++)
  {
    Peak * cand = * cit;
    if (! (cand->* fptr)() || cand->isWheel())
      continue;

    PPLiterator nextIter = candidates.next(cit, fptr);
    if (nextIter == cend)
      break;

    Peak * nextCand = * nextIter;
      
    if (cand->matchesGap(* nextCand, wheelGap))
    {
      PeakMinima::markWheelPair(* cand, * nextCand, "");
      
      const unsigned dist = nextCand->getIndex() - cand->getIndex();
      actualGap.update(dist);
    }
  }

  if (actualGap.isZero())
    actualGap = wheelGap;
}


void PeakMinima::fixBogieOrphans(PeakPool& peaks) const
{
  PeakPtrs& candidates = peaks.candidates();
  PPLiterator cbegin = candidates.begin();
  PPLiterator cend = candidates.end();

  for (auto cit = cbegin; cit != cend; cit++)
  {
    if ((* cit)->isLeftWheel())
    {
      PPLiterator ncit = candidates.next(cit, &Peak::isRightWheel);
      if (ncit == cend)
      {
        (* cit)->unselect();
        (* cit)->markNoBogie();
        (* cit)->markNoWheel();
      }
      else
        cit = ncit;
    }
    else if ((* cit)->isRightWheel())
    {
      PPLiterator pcit = candidates.prev(cit, &Peak::isRightWheel);
      if (pcit == cend)
      {
        (* cit)->unselect();
        (* cit)->markNoBogie();
        (* cit)->markNoWheel();
      }
    }
  }
}


void PeakMinima::updateGap(
  Gap& gap,
  const Gap& actualGap) const
{
  // actualGap may be narrower.
  const unsigned mid = (actualGap.lower + actualGap.upper) / 2;
  if (actualGap.upper <= static_cast<unsigned>(1.21f * actualGap.lower))
  {
    // Re-center.
    gap.lower = static_cast<unsigned>(mid / 1.1f) - 1;
    gap.upper = static_cast<unsigned>(mid * 1.1f) + 1;
  }
  else
  {
    gap.lower = actualGap.lower;
    gap.upper = actualGap.upper;
  }
}


void PeakMinima::markBogies(
  PeakPool& peaks,
  Gap& wheelGap)
{
  cout << peakPieces.str("For bogie gaps");

  peakPieces.guessBogieGap(wheelGap);

  cout << "Guessing wheel distance " << wheelGap.str() << "\n";

  Gap actualGap;
  PeakMinima::markBogiesOfSelects(peaks, &Peak::acceptableQuality, 
    wheelGap, actualGap);

  cout << "Got actual wheel distance " << actualGap.str() << "\n";

  // actualGap may be narrower.  We use this to re-center.
  PeakMinima::updateGap(wheelGap, actualGap);

  cout << "Wheel distance now " << wheelGap.str() << "\n";

  vector<Peak> bogieScale;
  makeBogieAverages(peaks, bogieScale);

  // Recalculate the peak qualities using both left and right peaks.
  PeakMinima::reseedBogiesUsingQuality(peaks, bogieScale);

  // Redo the marks with the new qualities.
  PeakMinima::markBogiesOfSelects(peaks, &Peak::acceptableQuality, 
    wheelGap, actualGap);

  cout << "Got actual wheel distance after recalculating " << 
    actualGap.str() << "\n";

  PeakMinima::updateGap(wheelGap, actualGap);

  cout << "Wheel distance after recalculating " << wheelGap.str() << "\n";


  // Redo the pieces.
  PeakMinima::makePieceList(peaks, 
    &Peak::acceptableQuality, &Peak::acceptableQuality,
    &Peak::arePartiallySelected);

  // Look at nearby pieces in order to catch near-misses.
  if (peakPieces.extendBogieGap(wheelGap))
  {
    cout << "Extended the bogie distance\n";
  cout << "Got wheel distance after extending " << 
    wheelGap.str() << "\n";
    actualGap.count = 0;
    PeakMinima::markBogiesOfSelects(peaks, &Peak::acceptableQuality, 
      wheelGap, actualGap);

  cout << "Got wheel distance after remarking " << 
    wheelGap.str() << "\n";
  cout << "Got actual wheel distance after extending " << 
    actualGap.str() << "\n";

    PeakMinima::updateGap(wheelGap, actualGap);

    cout << "Final wheel distance " << wheelGap.str() << "\n";
  }
  else
    cout << "No near misses for bogie distance\n";


  // Some halves of bogies may have been downgraded.
  PeakMinima::fixBogieOrphans(peaks);

  cout << peaks.candidates().strQuality(
    "All peaks using left/right scales", offset);

  // Recalculate the averages based on the new qualities.
  makeBogieAverages(peaks, bogieScale);
  PeakMinima::printPeakQuality(bogieScale[0], "Left-wheel average");
  PeakMinima::printPeakQuality(bogieScale[1], "Right-wheel average");
}


unsigned PeakMinima::markByDistance(
  PeakPool& peaks,
  const PeakFncPtr& fptr1,
  const PeakFncPtr& fptr2,
  const PeakFncPtr& qptr,
  const MinFncPtr& fptrMark,
  const Gap& gap) const
{
  //            bogie           short                   long
  //            -----           -----                   ----
  // fptr1                      isRightWheel            isRightWheel
  // fptr2                      isLeftWheel             isLeftWheel
  // qptr                       greatQuality            greatQuality
  // fptrMark                   markBogieShortGap       markBogieLongGap
  // gap                        shortGap                longGap

  unsigned count = 0;
  PeakPtrs& candidates = peaks.candidates();
  PPLiterator cbegin = candidates.begin();
  PPLiterator cend = candidates.end();

  for (PPLiterator cit = cbegin; cit != prev(cend); cit++)
  {
    // This is only for consecutive peaks.
    Peak * cand = * cit;
    PPLiterator ncit = next(cit);
    Peak * nextCand = * ncit;

    // If neither is set, or both are set, there is nothing to repair.
    if ((cand->* fptr1)() == (nextCand->* fptr2)())
      continue;

    if (! cand->matchesGap(* nextCand, gap))
      continue;

    // If the distance is right, we check the peak quality.
    if (((cand->* fptr1)() && (nextCand->* qptr)()) ||
        ((nextCand->* fptr2)() && (cand->* qptr)()))
    {
      (this->* fptrMark)(* cand, * nextCand);
      count++;
    }
  }
  return count;
}



void PeakMinima::markShortGaps(
  PeakPool& peaks,
  Gap& wheelGap,
  Gap& shortGap)
{
  PeakMinima::makePieceList(peaks, 
    &Peak::isRightWheel, &Peak::isLeftWheel, &Peak::areBogieGap);

  Gap longGap;
  if (peakPieces.guessGaps(wheelGap, shortGap, longGap))
  {
    cout << peakPieces.str("For short gaps by new method");

    cout << "Short gap by new method " << shortGap.str() << endl;
    if (shortGap.upper == 0)
      THROW(ERR_NO_PEAKS, "Short gap is zero");
  }
  else
  {
    THROW(ERR_NO_PEAKS, "New short gap fails");

    // The basis for eliminating small pieces would in general be
    // different, so for consistency we go back to the original value.
    PeakMinima::makePieceList(peaks, 
      &Peak::acceptableQuality, &Peak::acceptableQuality,
      &Peak::arentPartiallySelectedBogie, wheelGap.count);

    peakPieces.guessShortGap(wheelGap, shortGap);

    cout << peakPieces.str("For short gaps");

    PeakMinima::printDists(shortGap.lower, shortGap.upper,
      "Guessing short gap");

    if (shortGap.upper == 0)
      THROW(ERR_NO_PEAKS, "Short gap is zero");
  }

  // Mark short gaps (between cars).
  PeakMinima::markGapsOfSelects(peaks, 
    &Peak::isRightWheel, &Peak::isLeftWheel,
    &PeakMinima::markBogieShortGap,
    shortGap);

  // Look for unpaired short gaps (only among consecutive candidates).
  PeakMinima::markByDistance(peaks,
    &Peak::isRightWheel, &Peak::isLeftWheel, &Peak::greatQuality,
    &PeakMinima::markBogieShortGap, shortGap);

  // Make wheels within a bogie consistent.
  PeakMinima::makeLabelsConsistent(
    peaks,
    &Peak::isLeftWheel, &Peak::isRightWheel,
    &Peak::isRightBogie, &Peak::isLeftBogie,
    BOGIE_LEFT, BOGIE_RIGHT,
    wheelGap);

  // We could also make inter-bogie labels consistent, but in practice
  // they always are.

  // We will only recalculate qualities once we have done the long gaps
  // as well and marked up the bogies more thoroughly.
}


void PeakMinima::guessLongGapDistance(
  const PeakPool& peaks,
  const unsigned shortGapCount,
  Gap& longGap) const
{
  vector<unsigned> dists;
  const PeakPtrs & candidates = peaks.candidatesConst();
  PPLciterator cbegin = candidates.cbegin();
  PPLciterator cend = candidates.cend();

  for (PPLciterator cit = cbegin; cit != prev(cend); cit++)
  {
    Peak * cand = * cit;
    if (! cand->isRightWheel() || cand->isRightBogie())
      continue;

    PPLciterator ncit = candidates.next(cit, &Peak::isSelected);
    if (ncit == cend)
      break;

    Peak * nextCand = * ncit;
    if (nextCand->isLeftWheel())
      dists.push_back(nextCand->getIndex() - cand->getIndex());
  }
  sort(dists.begin(), dists.end());

  // Guess the intra-car gap.
  PeakMinima::findFirstLargeRange(dists, longGap, shortGapCount / 2);
}


unsigned PeakMinima::makeLabelsConsistent(
  PeakPool& peaks,
  const PeakFncPtr& wptr1,
  const PeakFncPtr& wptr2,
  const PeakFncPtr& bptrLeft, 
  const PeakFncPtr& bptrRight, 
  const BogieType bogieMatchLeft,
  const BogieType bogieMatchRight,
  const Gap& gap) const
{
  // Left-right wheels within range should have consistent bogie labels.
  //
  // wptr1: First wheel type (must match)
  // wptr2: Second wheel type (must match)
  // bptr1: Mismatch to isLeftBogie (diagnostics only)
  // bptr2: Mismatch to isRightBogie (diagnostics only)
  // bogieMatchLeft: 
  //
  //                    bogie gap       other gap
  //                    ---------       ---------
  // wptr1              isLeftWheel     isRightWheel
  // wptr2              isRightWheel    isLeftWheel
  // bptrLeft           isRightBogie    isLeftBogie
  // bptrRight          isLeftBogie     isRightBogie
  // bogieMatchLeft     BOGIE_LEFT      BOGIE_RIGHT
  // bogieMatchRight    BOGIE_RIGHT     BOGIE_LEFT

  unsigned count = 0;
  PeakPtrs& candidates = peaks.candidates();
  PPLiterator cbegin = candidates.begin();
  PPLiterator cend = candidates.end();
  for (PPLiterator cit = cbegin; cit != prev(cend); cit++)
  {
    Peak * cand = * cit;
    if (! (cand->* wptr1)())
      continue;

    PPLiterator ncit = candidates.next(cit, &Peak::isSelected);
    if (ncit == cend)
      break;

    Peak * nextCand = * ncit;
    if (! (nextCand->* wptr2)())
      continue;

    if (! cand->matchesGap(* nextCand, gap))
      continue;

    if (cand->isLeftBogie())
    {
      if (! nextCand->isBogie())
      {
        nextCand->markBogie(bogieMatchLeft);
        count++;
      }
      else if ((nextCand->* bptrLeft)())
        cout << "WARNING: Mismatch in bogies (1)\n";
    }
    else if (cand->isRightBogie())
    {
      if (! nextCand->isBogie())
      {
        nextCand->markBogie(bogieMatchRight);
        count++;
      }
      else if ((nextCand->* bptrRight)())
        cout << "WARNING: Mismatch in bogies (2)\n";
    }
    else if (nextCand->isLeftBogie())
    {
      cand->markBogie(bogieMatchLeft);
      count++;
    }
    else if (nextCand->isRightBogie())
    {
      cand->markBogie(bogieMatchRight);
      count++;
    }
  }
  return count;
}


void PeakMinima::markGapsOfSelects(
  PeakPool& peaks,
  const PeakFncPtr& fptr1,
  const PeakFncPtr& fptr2,
  const MinFncPtr& fptrMark,
  const Gap& gap) const
{
  // Does gaps between bogies.
  //
  //            short gaps              long gaps
  //            ----------              ---------
  // fptr1      isRightWheel            isRightWheelNonrightBogie
  // fptr2      isLeftWheel             isLeftWheelNonleftBogie
  // fptrMark   markBogieShortGap       markBogieLongGap

  PeakPtrs& candidates = peaks.candidates();
  PPLiterator cbegin = candidates.begin();
  PPLiterator cend = candidates.end();

  // Here we mark short gaps where both peaks are already wheels.
  for (auto cit = cbegin; cit != cend; cit++)
  {
    Peak * cand = * cit;
    if (! (cand->* fptr1)())
      continue;

    PPLiterator ncit = candidates.next(cit, &Peak::isSelected);
    if (ncit == cend)
      break;

    Peak * nextCand = * ncit;
    if (! (nextCand->* fptr2)())
      continue;
      
    if (! cand->matchesGap(* nextCand, gap))
      continue;

    (this->* fptrMark)(* cand, * nextCand);
  }
}


void PeakMinima::markLongGaps(
  PeakPool& peaks,
  const Gap& wheelGap,
  const unsigned shortGapCount,
  Gap& longGap)
{
  // Look for intra-car (long) gaps.
  PeakMinima::guessLongGapDistance(peaks, shortGapCount, longGap);

  PeakMinima::printDists(longGap.lower, longGap.upper, "Guessing long gap");

  // Label intra-car gaps (within cars).
  PeakMinima::markGapsOfSelects(peaks,
    &Peak::isRightWheelNonrightBogie, &Peak::isLeftWheelNonleftBogie,
    &PeakMinima::markBogieLongGap, longGap);

  // We are not currently looking for unpaired peaks in long gaps.

  vector<Peak> bogies;
  PeakMinima::makeCarAverages(peaks, bogies);

  PeakMinima::printPeakQuality(bogies[0], "Left bogie, left wheel avg");
  PeakMinima::printPeakQuality(bogies[1], "Left bogie, right wheel avg");
  PeakMinima::printPeakQuality(bogies[2], "Right bogie, left wheel avg");
  PeakMinima::printPeakQuality(bogies[3], "Right bogie, right wheel avg");

  // Recalculate the peak qualities using both left and right peaks.
  PeakMinima::reseedLongGapsUsingQuality(peaks, bogies);

  // Some peaks might have become good enough to lead to cars,
  // so we re-label.  First we look again for bogies.
  Gap actualGap;
  PeakMinima::markBogiesOfSelects(peaks, &Peak::acceptableQuality,
    wheelGap, actualGap);

  PeakMinima::markGapsOfSelects(peaks,
    &Peak::isRightWheelNonrightBogie, &Peak::isLeftWheelNonleftBogie,
    &PeakMinima::markBogieLongGap, longGap);

  cout << peaks.candidates().strQuality("peaks with all four wheels", 
    offset);

  // Recalculate the averages based on the new qualities.
  PeakMinima::makeCarAverages(peaks, bogies);

  PeakMinima::printPeakQuality(bogies[0], "Left bogie, left wheel avg");
  PeakMinima::printPeakQuality(bogies[1], "Left bogie, right wheel avg");
  PeakMinima::printPeakQuality(bogies[2], "Right bogie, left wheel avg");
  PeakMinima::printPeakQuality(bogies[3], "Right bogie, right wheel avg");

  // Redo the distances using the new qualities (all four peaks).
  PeakMinima::guessLongGapDistance(peaks, shortGapCount, longGap);

  PeakMinima::printDists(longGap.lower, longGap.upper, 
    "Guessing new long gap");

  // Mark more bogies with the refined peak qualities.
  PeakMinima::markGapsOfSelects(peaks,
    &Peak::isRightWheelNonrightBogie, &Peak::isLeftWheelNonleftBogie,
    &PeakMinima::markBogieLongGap, longGap);

  // Mark bogies with the right distance.
  PeakMinima::markByDistance(peaks,
    &Peak::isRightWheel, &Peak::isLeftWheel, &Peak::greatQuality,
    &PeakMinima::markBogieLongGap, longGap);

  // This makes wheels within a bogie consistent.
  PeakMinima::makeLabelsConsistent(
    peaks,
    &Peak::isLeftWheel, &Peak::isRightWheel,
    &Peak::isRightBogie, &Peak::isLeftBogie,
    BOGIE_LEFT, BOGIE_RIGHT,
    wheelGap);

  // Labels between long gaps are generally consistent,
  // so no need to call makeLabelsConsistent.

  // Store the average peaks for later reference.
  peaks.logAverages(bogies);
}


void PeakMinima::makePieceList(
  const PeakPool& peaks,
  const PeakFncPtr& fptr1,
  const PeakFncPtr& fptr2,
  const PeakPairFncPtr& includePtr,
  const unsigned smallPieceLimit)
{
  vector<unsigned> dists;
  peaks.candidatesConst().makeDistances(fptr1, fptr2, includePtr, dists);

// cout << "DISTS\n";
// for (auto d: dists)
  // cout << d << endl;

  peakPieces.make(dists, smallPieceLimit);
}


void PeakMinima::mark(
  PeakPool& peaks,
  const vector<Peak>& peakCenters,
  const unsigned offsetIn)
{
  offset = offsetIn;

  PeakMinima::markSinglePeaks(peaks, peakCenters);

unsigned countAll = peaks.candidates().size();
unsigned countSelected = peaks.candidates().count(&Peak::isSelected);
cout << "FRAC " << countSelected << " " << 
  countAll << " " <<
  fixed << setprecision(2) << 100. * countSelected / countAll << endl;

  PeakMinima::makePieceList(peaks, 
    &Peak::acceptableQuality, &Peak::acceptableQuality,
    &Peak::arePartiallySelected);

  if (peakPieces.empty())
    THROW(ERR_NO_PEAKS, "Piece list is empty");

  Gap wheelGap;
  PeakMinima::markBogies(peaks, wheelGap);

peaks.mergeSplits((wheelGap.lower + wheelGap.upper) / 2, offset);

  Gap shortGap;
  PeakMinima::markShortGaps(peaks, wheelGap, shortGap);

  Gap longGap;
  PeakMinima::markLongGaps(peaks, wheelGap, shortGap.count, longGap);

cout << peaks.candidates().strQuality(
  "All selected peaks at end of PeakMinima", offset, &Peak::isSelected);

  cout << peakPieces.str("end");
  cout << peakPieces.strModality("QQQ");
}


void PeakMinima::printPeakQuality(
  const Peak& peak,
  const string& text) const
{
  cout << text << "\n";
  cout << peak.strHeaderQuality();
  cout << peak.strQuality(offset) << endl;
}


void PeakMinima::printDists(
  const unsigned start,
  const unsigned end,
  const string& text) const
{
  cout << text << " " << start << "-" << end << endl;
}


void PeakMinima::printRange(
  const unsigned start,
  const unsigned end,
  const string& text) const
{
  cout << text << " " << start + offset << "-" << end + offset << endl;
}

