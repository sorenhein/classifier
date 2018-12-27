#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "Peak.h"
#include "PeakMinima.h"
#include "Except.h"


#define SLIDING_LOWER 0.9f
#define SLIDING_UPPER 1.1f

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

  struct DistEntry
  {
    unsigned index;
    int direction;
    unsigned origin;

    bool operator < (const DistEntry& de2)
    {
      return (index < de2.index);
    };
  };

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

  int bestCount = 0;
  unsigned bestValue = 0;
  unsigned bestUpperValue = 0;
  unsigned bestLowerValue = 0;
  int count = 0;
  unsigned dindex = steps.size();

  unsigned i = 0;
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

  gap.lower = bestLowerValue;
  gap.upper = bestUpperValue;
  gap.count = bestCount;
}


bool PeakMinima::bothSelected(
  const Peak * p1,
  const Peak * p2) const
{
  return (p1->isSelected() && p2->isSelected());
}


bool PeakMinima::bothPlausible(
  const Peak * p1,
  const Peak * p2) const
{
  if (! p1->isSelected() && ! p2->isSelected())
    return false;

  return (p1->acceptableQuality() && p2->acceptableQuality());
}


bool PeakMinima::formBogeyGap(
  const Peak * p1,
  const Peak * p2) const
{
  return (p1->isRightWheel() && p2->isLeftWheel());
}


bool PeakMinima::guessNeighborDistance(
  const PeakPool& peaks,
  const PeakPtrList& candidates,
  const CandFncPtr fptr,
  Gap& gap,
  const unsigned minCount) const
{
UNUSED(peaks);
  // Make list of distances between neighbors for which fptr
  // evaluates to true.
  vector<unsigned> dists;
  for (auto pit = candidates.begin(); pit != prev(candidates.end()); pit++)
  {
    auto npit = next(pit);
    if ((this->* fptr)(* pit, * npit))
      dists.push_back(
        (*npit)->getIndex() - (*pit)->getIndex());
  }

  if (dists.size() < minCount)
    return false;

  // Guess their distance range.
  sort(dists.begin(), dists.end());

  const unsigned revisedMinCount = min(minCount, dists.size() / 4);
  PeakMinima::findFirstLargeRange(dists, gap, revisedMinCount);
  return true;
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



void PeakMinima::markBogeyShortGap(
  Peak& p1,
  Peak& p2,
  PeakCit& cit, // Iterator to "prev(p1)"
  PeakCit& ncit, // Iterator to "next(p2)"
  PeakCit& cbegin,
  PeakCit& cend,
  const string& text) const
{
  if (text != "")
    PeakMinima::printRange(p1.getIndex(), p2.getIndex(),
      text + " short car gap at");

  if (p1.isRightWheel() && ! p1.isRightBogey())
  {
    // Paired previous wheel was not yet marked a bogey.
    auto prevCand = PeakMinima::prevWithProperty(
      cit, cbegin, &Peak::isLeftWheel);
    
    (* prevCand)->markBogeyAndWheel(BOGEY_RIGHT, WHEEL_LEFT);
  }

  if (p2.isLeftWheel() && ! p2.isLeftBogey())
  {
    // Paired next wheel was not yet marked a bogey.
    auto nextCand =  PeakMinima::nextWithProperty(
      ncit, cend, &Peak::isRightWheel);

    (* nextCand)->markBogeyAndWheel(BOGEY_LEFT, WHEEL_RIGHT);
  }

  p1.markBogeyAndWheel(BOGEY_RIGHT, WHEEL_RIGHT);
  p2.markBogeyAndWheel(BOGEY_LEFT, WHEEL_LEFT);
}


void PeakMinima::markBogeyLongGap(
  Peak& p1,
  Peak& p2,
  const string& text) const
{
  if (text != "")
    PeakMinima::printRange(p1.getIndex(), p2.getIndex(),
      text + " long car gap at");
  
  p1.markBogey(BOGEY_LEFT);
  p2.markBogey(BOGEY_RIGHT);
}


void PeakMinima::reseedWheelUsingQuality(
  PeakPool& peaks,
  list<Peak *>& candidates) const
{
UNUSED(peaks);
  for (auto candidate: candidates)
  {
    if (candidate->greatQuality())
      candidate->select();
    else
      candidate->unselect();
  }
}


void PeakMinima::reseedBogeysUsingQuality(
  PeakPool& peaks,
  PeakPtrList& candidates,
  const vector<Peak>& bogeyScale) const
{
UNUSED(peaks);
  for (auto cand: candidates)
  {
    if (! cand->isWheel())
      cand->calcQualities(bogeyScale);
    else if (cand->isLeftWheel())
      cand->calcQualities(bogeyScale[0]);
    else if (cand->isRightWheel())
      cand->calcQualities(bogeyScale[1]);

    if (cand->greatQuality())
      cand->select();
    else if (cand->isWheel() && cand->acceptableQuality())
      cand->select();
    else
      cand->unselect();
  }
}


void PeakMinima::reseedLongGapsUsingQuality(
  PeakPool& peaks,
  PeakPtrList& candidates,
  const vector<Peak>& longGapScale) const
{
UNUSED(peaks);
  for (auto cand: candidates)
  {
    if (! cand->isWheel())
      cand->calcQualities(longGapScale);
    else if (cand->isLeftBogey())
    {
      if (cand->isLeftWheel())
        cand->calcQualities(longGapScale[0]);
      else if (cand->isRightWheel())
        cand->calcQualities(longGapScale[1]);
      else
      {
        // TODO
        // THROW(ERR_ALGO_PEAK_CONSISTENCY, "No wheel in bogey?");
      }
    }
    else if (cand->isRightBogey())
    {
      if (cand->isLeftWheel())
        cand->calcQualities(longGapScale[2]);
      else if (cand->isRightWheel())
        cand->calcQualities(longGapScale[3]);
      else
      {
        // TODO
        // THROW(ERR_ALGO_PEAK_CONSISTENCY, "No wheel in bogey?");
      }
    }
    else
    {
      // TODO
      // THROW(ERR_ALGO_PEAK_CONSISTENCY, "No bogey for wheel?");
    }

    if (cand->greatQuality())
      cand->select();
    else if (cand->isWheel() && cand->acceptableQuality())
      cand->select();
    else
      cand->unselect();
  }
}


void PeakMinima::makeWheelAverage(
  PeakPool& peaks,
  PeakPtrList& candidates,
  Peak& wheel) const
{
UNUSED(peaks);
  wheel.reset();

  unsigned count = 0;
  for (auto& cand: candidates)
  {
    if (cand->isSelected())
    {
      wheel += * cand;
      count++;
    }
  }

  if (count)
    wheel /= count;
}


void PeakMinima::makeBogeyAverages(
  const PeakPool& peaks,
  vector<Peak>& wheels) const
{
  wheels.clear();
  wheels.resize(2);

  unsigned cleft = 0, cright = 0;
  PPciterator cbegin = peaks.candcbegin();
  PPciterator cend = peaks.candcend();
  for (PPciterator cit = cbegin; cit != cend; cit++)
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

  PPciterator cbegin = peaks.candcbegin();
  PPciterator cend = peaks.candcend();
  for (PPciterator cit = cbegin; cit != cend; cit++)
  {
    Peak const * cand = * cit;
    if (cand->isLeftBogey())
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
    else if (cand->isRightBogey())
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


void PeakMinima::setCandidates(
  PeakPool& peaks,
  PeakPtrList& candidates) const
{
UNUSED(peaks);
  for (auto& peak: peaks)
  {
    if (peak.isCandidate())
      candidates.push_back(&peak);
  }
}


void PeakMinima::markSinglePeaks(
  PeakPool& peaks,
  PeakPtrList& candidates) const
{
UNUSED(peaks);
  if (candidates.empty())
    THROW(ERR_NO_PEAKS, "No tall peaks");

  // Find the average candidate peak.
  Peak wheelPeak;
  PeakMinima::makeWheelAverage(peaks, candidates, wheelPeak);

  // Use this as a first yardstick for calculating qualities.
  for (auto candidate: candidates)
    candidate->calcQualities(wheelPeak);

  cout << peaks.strAllCandsQuality("All negative minima", offset);
  cout << peaks.strSelectedCandsQuality("Seeds", offset);

  PeakMinima::makeWheelAverage(peaks, candidates, wheelPeak);
  PeakMinima::printPeakQuality(wheelPeak, "Seed average");

  // Modify selection based on quality.
  PeakMinima::reseedWheelUsingQuality(peaks, candidates);

  cout << peaks.strSelectedCandsQuality("Great-quality seeds", offset);

  // Remake the average.
  PeakMinima::makeWheelAverage(peaks, candidates, wheelPeak);
  PeakMinima::printPeakQuality(wheelPeak, "Great-quality average");
}


PeakCit PeakMinima::nextWithProperty(
  PeakCit& it,
  PeakCit& endList,
  const PeakFncPtr fptr) const
{
  auto itNext = it;
  do
  {
    itNext++;
  }
  while (itNext != endList && ! ((* itNext)->* fptr)());

  return itNext;
}


PeakCit PeakMinima::prevWithProperty(
  PeakCit& it,
  PeakCit& beginList,
  const PeakFncPtr fptr) const
{
  for (auto itPrev = it; ; itPrev = prev(itPrev))
  {
    if (((* itPrev)->* fptr)())
      return itPrev;
    else if (itPrev == beginList)
      THROW(ERR_ALGO_PEAK_CONSISTENCY, "Miss earlier matching peak");
  }
}


void PeakMinima::markBogeysOfSelects(
  PeakPool& peaks,
  const Gap& wheelGap) const
{
  PPiterator cbegin = peaks.candbegin();
  PPiterator cend = peaks.candend();

  // Here we mark bogeys where both peaks are already selected.
  for (auto cit = cbegin; cit != prev(cend); cit++)
  {
    Peak * cand = * cit;
    if (cand->isWheel())
      continue;

    Peak * nextCand = * next(cit);
    if (PeakMinima::bothSelected(cand, nextCand))
    {
      if (cand->matchesGap(* nextCand, wheelGap))
      {
        if (cand->isWheel())
          THROW(ERR_NO_PEAKS, "Triple bogey?!");

        PeakMinima::markWheelPair(* cand, * nextCand, "");
      }
    }
  }
}


void PeakMinima::markBogeysOfUnpaired(
  PeakPool& peaks,
  const Gap& wheelGap) const
{
  PPiterator cbegin = peaks.candbegin();
  PPiterator cend = peaks.candend();

  for (auto cit = cbegin; cit != prev(cend); cit++)
  {
    Peak * cand = * cit;
    Peak * nextCand = * next(cit);

    // If they are both set, move on.
    if (cand->isSelected() && nextCand->isSelected())
      continue;

    if (! cand->acceptableQuality())
      continue;

    // If the first one is of acceptable quality, find the first
    // such successor.  There could be poorer peaks in between.
    PPiterator ncit = peaks.nextCandExcl(cit, &Peak::acceptableQuality);

    if (ncit == cend)
      break;

    // If the distance is right, we lower our quality requirements.
    if (cand->matchesGap(** ncit, wheelGap))
      PeakMinima::markWheelPair(* cand, ** ncit, "Adding");
  }
}


void PeakMinima::markBogeys(
  PeakPool& peaks,
  PeakPtrList& candidates,
  Gap& wheelGap) const
{
  // The wheel gap is only plausible if it hits a certain number of peaks.
  unsigned numGreat = peaks.countCandidates(&Peak::greatQuality);

  if (! PeakMinima::guessNeighborDistance(peaks, candidates, 
      &PeakMinima::bothSelected, wheelGap, numGreat/4))
  {
    // This may happen when one side of the peak pair is so strong
    // and different that the other side never gets picked up.
    // Try again, and lower our standards to acceptable peak quality.
    cout << "First attempt at wheel distance failed: " << numGreat << ".\n";
    if (! PeakMinima::guessNeighborDistance(peaks, candidates, 
        &PeakMinima::bothPlausible, wheelGap, numGreat/4))
    {
      THROW(ERR_ALGO_NO_WHEEL_GAP, "Couldn't find wheel gap");
    }
  }

  PeakMinima::printDists(wheelGap.lower, wheelGap.upper,
    "Guessing wheel distance");

  PeakMinima::markBogeysOfSelects(peaks, wheelGap);

  // Look for unpaired wheels where there is a nearby peak that is
  // not too bad.  If there is a spurious peak in between, we'll fail...
  PeakMinima::markBogeysOfUnpaired(peaks, wheelGap);

  vector<Peak> bogeyScale;
  makeBogeyAverages(peaks, bogeyScale);

  // Recalculate the peak qualities using both left and right peaks.
  PeakMinima::reseedBogeysUsingQuality(peaks, candidates, bogeyScale);

  cout << peaks.strAllCandsQuality("All peaks using left/right scales",
    offset);

  // Recalculate the averages based on the new qualities.
  makeBogeyAverages(peaks, bogeyScale);
  PeakMinima::printPeakQuality(bogeyScale[0], "Left-wheel average");
  PeakMinima::printPeakQuality(bogeyScale[1], "Right-wheel average");

  // Redo the distances using the new qualities (left and right peaks).
  numGreat = peaks.countCandidates(&Peak::greatQuality);

  PeakMinima::guessNeighborDistance(peaks, candidates,
    &PeakMinima::bothSelected, wheelGap, numGreat/4);

  PeakMinima::printDists(wheelGap.lower, wheelGap.upper,
    "Guessing new wheel distance");

  // Mark more bogeys with the refined peak qualities.
  PeakMinima::markBogeysOfSelects(peaks, wheelGap);

  PeakMinima::markBogeysOfUnpaired(peaks, wheelGap);

  cout << peaks.strAllCandsQuality(
    "All peaks again using left/right scales", offset);

}


void PeakMinima::markShortGapsOfSelects(
  PeakPool& peaks,
  const Gap& shortGap) const
{
  PPiterator cbegin = peaks.candbegin();
  PPiterator cend = peaks.candend();

  for (auto cit = cbegin; cit != prev(cend); cit++)
  {
    Peak * cand = * cit;
    auto ncit = next(cit);
    Peak * nextCand = * ncit;

    if (! cand->isRightWheel() || ! nextCand->isLeftWheel())
      continue;

    if (! cand->matchesGap(* nextCand, shortGap))
      continue;

    PeakMinima::markBogeyShortGap(* cand, * nextCand, 
      cit, ncit, cbegin, cend, "");
  }
}


void PeakMinima::markShortGapsOfUnpaired(
  PeakPool& peaks,
  const Gap& shortGap) const
{
  PPiterator cbegin = peaks.candbegin();
  PPiterator cend = peaks.candend();

  for (PPiterator cit = cbegin; cit != prev(cend); cit++)
  {
    Peak * cand = * cit;
    auto ncit = next(cit);
    Peak * nextCand = * ncit;

    // If neither is set, or both are set, there is nothing to repair.
    if (cand->isRightWheel() == nextCand->isLeftWheel())
      continue;

    if (! cand->matchesGap(* nextCand, shortGap))
      continue;

    // If the distance is right, we can relax our quality requirements.
    if ((cand->isRightWheel() && nextCand->greatQuality()) ||
        (nextCand->isLeftWheel() && cand->greatQuality()))
    {
      PeakMinima::markBogeyShortGap(* cand, * nextCand, 
        cit, ncit, cbegin, cend, "");
    }
  }
}


void PeakMinima::markShortGaps(
  PeakPool& peaks,
  PeakPtrList& candidates,
  Gap& shortGap)
{
  // Look for inter-car short gaps.
  PeakMinima::guessNeighborDistance(peaks, candidates,
    &PeakMinima::formBogeyGap, shortGap);

  PeakMinima::printDists(shortGap.lower, shortGap.upper,
    "Guessing short gap");

  // Mark short gaps (between cars).
  PeakMinima::markShortGapsOfSelects(peaks, shortGap);

  // Look for unpaired short gaps.  If there is a spurious peak
  // in between, we will fail.
  PeakMinima::markShortGapsOfUnpaired(peaks, shortGap);

  // We will only recalculate qualities once we have done the long gaps
  // as well and marked up the bogeys more thoroughly.
}


void PeakMinima::guessLongGapDistance(
  const PeakPool& peaks,
  const unsigned shortGapCount,
  Gap& longGap) const
{
  vector<unsigned> dists;
  PPciterator cbegin = peaks.candcbegin();
  PPciterator cend = peaks.candcend();

  for (PPciterator cit = cbegin; cit != prev(cend); cit++)
  {
    Peak * cand = * cit;
    if (! cand->isRightWheel() || cand->isRightBogey())
      continue;

    PPciterator ncit = peaks.nextCandExcl(cit, &Peak::isSelected);
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


void PeakMinima::markLongGapsOfSelects(
  PeakPool& peaks,
  const Gap& longGap) const
{
  PPiterator cbegin = peaks.candbegin();
  PPiterator cend = peaks.candend();
  for (PPiterator cit = cbegin; cit != prev(cend); cit++)
  {
    Peak * cand = * cit;
    if (! cand->isRightWheel() || cand->isRightBogey())
      continue;

    PPiterator ncit = peaks.nextCandExcl(cit, &Peak::isSelected);
    if (ncit == cend)
      break;

    Peak * nextCand = * ncit;
    if (! nextCand->isLeftWheel())
      continue;

    if (cand->matchesGap(* nextCand, longGap))
    {
      PeakMinima::markBogeyLongGap(* cand, * nextCand, "");

      // Neighboring wheels may not have bogeys yet, so we mark them.
      PPiterator ppcit = peaks.prevCandExcl(cit, &Peak::isLeftWheel);
      if (! (* ppcit)->isBogey())
        (* ppcit)->markBogey(BOGEY_LEFT);

      PPiterator nncit = peaks.nextCandExcl(ncit, &Peak::isRightWheel);
      if (nncit != cend && ! (* nncit)->isBogey())
        (* nncit)->markBogey(BOGEY_RIGHT);
    }
  }
}


void PeakMinima::markLongGaps(
  PeakPool& peaks,
  PeakPtrList& candidates,
  const Gap& wheelGap,
  const unsigned shortGapCount)
{
  // Look for intra-car (long) gaps.
  Gap longGap;
  PeakMinima::guessLongGapDistance(peaks, shortGapCount, longGap);

  PeakMinima::printDists(longGap.lower, longGap.upper, "Guessing long gap");

  // Label intra-car gaps (within cars).
  PeakMinima::markLongGapsOfSelects(peaks, longGap);

  // We are not currently looking for unpaired peaks in long gaps.

  vector<Peak> bogeys;
  PeakMinima::makeCarAverages(peaks, bogeys);

  PeakMinima::printPeakQuality(bogeys[0], "Left bogey, left wheel avg");
  PeakMinima::printPeakQuality(bogeys[1], "Left bogey, right wheel avg");
  PeakMinima::printPeakQuality(bogeys[2], "Right bogey, left wheel avg");
  PeakMinima::printPeakQuality(bogeys[3], "Right bogey, right wheel avg");

  // Recalculate the peak qualities using both left and right peaks.
  PeakMinima::reseedLongGapsUsingQuality(peaks, candidates, bogeys);

  // Some peaks might have become good enough to lead to cars,
  // so we re-label.  First we look again for bogeys.
  PeakMinima::markBogeysOfSelects(peaks, wheelGap);
  PeakMinima::markLongGapsOfSelects(peaks, longGap);

  cout << peaks.strAllCandsQuality("peaks with all four wheels", offset);

  // Recalculate the averages based on the new qualities.
  PeakMinima::makeCarAverages(peaks, bogeys);

  PeakMinima::printPeakQuality(bogeys[0], "Left bogey, left wheel avg");
  PeakMinima::printPeakQuality(bogeys[1], "Left bogey, right wheel avg");
  PeakMinima::printPeakQuality(bogeys[2], "Right bogey, left wheel avg");
  PeakMinima::printPeakQuality(bogeys[3], "Right bogey, right wheel avg");

  // Redo the distances using the new qualities (all four peaks).
  PeakMinima::guessLongGapDistance(peaks, shortGapCount, longGap);

  PeakMinima::printDists(longGap.lower, longGap.upper, 
    "Guessing new long gap");

  // Mark more bogeys with the refined peak qualities.
  PeakMinima::markLongGapsOfSelects(peaks, longGap);
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


void PeakMinima::mark(
  PeakPool& peaks,
  PeakPtrList& candidates,
  const unsigned offsetIn)
{
  candidates.clear();
  offset = offsetIn;

  PeakMinima::setCandidates(peaks, candidates);
peaks.makeCandidates();

  PeakMinima::markSinglePeaks(peaks, candidates);

  Gap wheelGap;
  PeakMinima::markBogeys(peaks, candidates, wheelGap);

  Gap shortGap;
  PeakMinima::markShortGaps(peaks, candidates, shortGap);
  PeakMinima::markLongGaps(peaks, candidates, wheelGap, shortGap.count);
}

