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


bool PeakMinima::formBogeyGap(
  const Peak * p1,
  const Peak * p2) const
{
  return (p1->isRightWheel() && p2->isLeftWheel());
}


void PeakMinima::guessNeighborDistance(
  const list<Peak *>& candidates,
  const CandFncPtr fptr,
  Gap& gap) const
{
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

  // Guess their distance range.
  sort(dists.begin(), dists.end());
  PeakMinima::findFirstLargeRange(dists, gap);
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
  const string& text) const
{
  if (text != "")
    PeakMinima::printRange(p1.getIndex(), p2.getIndex(),
      text + " short car gap at");
  
  p1.markBogey(BOGEY_RIGHT);
  p2.markBogey(BOGEY_LEFT);
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


void PeakMinima::reseedUsingQuality(list<Peak *>& candidates)
{
  for (auto candidate: candidates)
  {
    if (candidate->greatQuality())
      candidate->select();
    else
      candidate->unselect();
  }
}


void PeakMinima::makeWheelAverage(
  list<Peak *>& candidates,
  Peak& seed) const
{
  seed.reset();

  unsigned count = 0;
  for (auto& cand: candidates)
  {
    if (cand->isSelected())
    {
      seed += * cand;
      count++;
    }
  }

  if (count)
    seed /= count;
}


void PeakMinima::makeBogeyAverages(
  list<Peak *>& candidates,
  vector<Peak>& wheels) const
{
  wheels.clear();
  wheels.resize(2);

  unsigned cleft = 0, cright = 0;
  for (auto& cand: candidates)
  {
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
  list<Peak *>& candidates,
  vector<Peak>& wheels) const
{
  wheels.clear();
  wheels.resize(4);

  vector<unsigned> count;
  count.resize(4);

  for (auto& cand: candidates)
  {
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
  list<Peak>& peaks,
  list<Peak *>& candidates)
{
  for (auto pit = peaks.begin(); pit != peaks.end(); pit++)
  {
    if (! pit->isCandidate())
      continue;

    // Exclude tall peaks without a right neighbor.
    auto npit = next(pit);
    if (npit != peaks.end())
      pit->logNextPeak(&*npit);
  }

  for (auto& peak: peaks)
  {
    if (peak.isCandidate())
      candidates.push_back(&peak);
  }
}


void PeakMinima::markSinglePeaks(list<Peak *>& candidates)
{
  if (candidates.empty())
    THROW(ERR_NO_PEAKS, "No tall peaks");

  // Find the average candidate peak.
  Peak wheelPeak;
  PeakMinima::makeWheelAverage(candidates, wheelPeak);

  // Use this as a first yardstick for calculating qualities.
  for (auto candidate: candidates)
    candidate->calcQualities(wheelPeak);

  PeakMinima::printAllCandidates(candidates, "All negative minima");
  PeakMinima::printSeedCandidates(candidates, "Seeds");

  PeakMinima::makeWheelAverage(candidates, wheelPeak);
  PeakMinima::printPeakQuality(wheelPeak, "Seed average");

  // Modify selection based on quality.
  PeakMinima::reseedUsingQuality(candidates);

  PeakMinima::printSeedCandidates(candidates, "Great-quality seeds");

  PeakMinima::makeWheelAverage(candidates, wheelPeak);
  PeakMinima::printPeakQuality(wheelPeak, "Great-quality average");
}


void PeakMinima::markBogeys(list<Peak *>& candidates)
{
  Gap wheelGap;
  PeakMinima::guessNeighborDistance(candidates, 
    &PeakMinima::bothSelected, wheelGap);

  cout << "Guessing wheel distance " << wheelGap.lower << "-" <<
    wheelGap.upper << "\n\n";

  // Tentatively mark wheel pairs (bogeys).  If there are only 
  // single wheels, we might be marking the wagon gaps instead.
  for (auto cit = candidates.begin(); cit != prev(candidates.end()); cit++)
  {
    Peak * cand = * cit;
    Peak * nextCand = * next(cit);
    if (PeakMinima::bothSelected(cand, nextCand))
    {
      const unsigned dist = nextCand->getIndex() - cand->getIndex();
      if (dist >= wheelGap.lower && dist <= wheelGap.upper)
      {
        if (cand->isWheel())
          THROW(ERR_NO_PEAKS, "Triple bogey?!");

        PeakMinima::markWheelPair(* cand, * nextCand, "");
      }
    }
  }

  // Look for unpaired wheels where there is a nearby peak that is
  // not too bad.  If there is a spurious peak in between, we'll fail...
  for (auto cit = candidates.begin(); cit != prev(candidates.end()); cit++)
  {
    Peak * cand = * cit;
    Peak * nextCand = * next(cit);
    // If neither is set, or both are set, there is nothing to repair.
    if (cand->isSelected() == nextCand->isSelected())
      continue;

    const unsigned dist = nextCand->getIndex() - cand->getIndex();
    if (dist < wheelGap.lower || dist > wheelGap.upper)
      continue;

    if ((cand->isSelected() && nextCand->acceptableQuality()) ||
        (nextCand->isSelected() && cand->acceptableQuality()))
      PeakMinima::markWheelPair(* cand, * nextCand, "Adding");
  }

  vector<Peak> candidateSize;
  makeBogeyAverages(candidates, candidateSize);

  // Recalculate the peak qualities using both left and right peaks.
  for (auto cand: candidates)
  {
    if (! cand->isWheel())
      cand->calcQualities(candidateSize);
    else if (cand->isLeftWheel())
      cand->calcQualities(candidateSize[0]);
    else if (cand->isRightWheel())
      cand->calcQualities(candidateSize[1]);

    if (cand->greatQuality())
      cand->select();
    else
      cand->unselect();
  }

  PeakMinima::printAllCandidates(candidates,
    "All peaks using left/right scales");

  makeBogeyAverages(candidates, candidateSize);
  PeakMinima::printPeakQuality(candidateSize[0], "Left-wheel average");
  PeakMinima::printPeakQuality(candidateSize[1], "Right-wheel average");

  // Redo the distances using the new qualities (left and right peaks).
  PeakMinima::guessNeighborDistance(candidates,
    &PeakMinima::bothSelected, wheelGap);
  cout << "Guessing new wheel distance " << wheelGap.lower << "-" <<
    wheelGap.upper << "\n\n";


  // Mark more bogeys with the refined peak qualities.
  for (auto cit = candidates.begin(); cit != prev(candidates.end()); cit++)
  {
    Peak * cand = * cit;
    if (cand->isWheel())
      continue;

    Peak * nextCand = * next(cit);
    if (PeakMinima::bothSelected(cand, nextCand))
    {
      const unsigned dist = nextCand->getIndex() - cand->getIndex();
      if (dist >= wheelGap.lower && dist <= wheelGap.upper)
      {
        if (cand->isWheel())
          THROW(ERR_NO_PEAKS, "Triple bogey?!");

        PeakMinima::markWheelPair(* cand, * nextCand, "");
      }
    }
  }
}


void PeakMinima::markShortGaps(
  list<Peak *>& candidates,
  Gap& shortGap)
{
  // Look for inter-car short gaps.
  PeakMinima::guessNeighborDistance(candidates,
    &PeakMinima::formBogeyGap, shortGap);

  cout << "Guessing short gap " << shortGap.lower << "-" <<
    shortGap.upper << "\n\n";


  // Tentatively mark short gaps (between cars).
  for (auto cit = candidates.begin(); cit != prev(candidates.end()); cit++)
  {
    Peak * cand = * cit;
    Peak * nextCand = * next(cit);

    if (! cand->isRightWheel() || ! nextCand->isLeftWheel())
      continue;

    const unsigned dist = nextCand->getIndex() - cand->getIndex();
    if (dist >= shortGap.lower && dist <= shortGap.upper)
      PeakMinima::markBogeyShortGap(* cand, * nextCand, "");
  }


  // Look for unpaired short gaps.  If there is a spurious peak
  // in between, we will fail.
  for (auto cit = candidates.begin(); cit != prev(candidates.end()); cit++)
  {
    Peak * cand = * cit;
    Peak * nextCand = * next(cit);

    // If neither is set, or both are set, there is nothing to repair.
    if (cand->isRightWheel() == nextCand->isLeftWheel())
      continue;

    const unsigned dist = nextCand->getIndex() - cand->getIndex();
    if (dist < shortGap.lower || dist > shortGap.upper)
      continue;

    if ((cand->isRightWheel() && nextCand->greatQuality()) ||
        (nextCand->isLeftWheel() && cand->greatQuality()))
      PeakMinima::markBogeyShortGap(* cand, * nextCand, "");
  }
}


void PeakMinima::markLongGaps(
  list<Peak *>& candidates,
  const unsigned shortGapCount)
{
  // Look for intra-car (long) gaps.
  vector<unsigned> dists;
  for (auto cit = candidates.begin(); cit != prev(candidates.end()); cit++)
  {
    Peak * cand = * cit;
    if (! cand->isRightWheel() || cand->isBogey())
      continue;

    auto ncit = cit;
    do
    {
      ncit = next(ncit);
    }
    while (ncit != candidates.end() && ! (* ncit)->isSelected());

    if (ncit == candidates.end())
      break;

    Peak * nextCand = * ncit;
    if (nextCand->isLeftWheel())
      dists.push_back(nextCand->getIndex() - cand->getIndex());
  }
  sort(dists.begin(), dists.end());

  // Guess the intra-car gap.
  Gap longGap;
  PeakMinima::findFirstLargeRange(dists, longGap, shortGapCount / 2);
  cout << "Guessing long gap " << longGap.lower << "-" <<
    longGap.upper << "\n\n";

  // Label intra-car gaps.
  for (auto cit = candidates.begin(); cit != prev(candidates.end()); cit++)
  {
    Peak * cand = * cit;
    if (! cand->isRightWheel() || cand->isBogey())
      continue;

    auto ncit = cit;
    do
    {
      ncit = next(ncit);
    }
    while (ncit != candidates.end() && ! (* ncit)->isSelected());

    if (ncit == candidates.end())
      break;

    Peak * nextCand = * ncit;
    if (! nextCand->isLeftWheel())
      continue;

    const unsigned dist = nextCand->getIndex() - cand->getIndex();
    if (dist >= longGap.lower && dist <= longGap.upper)
    {
      PeakMinima::markBogeyLongGap(* cand, * nextCand, "");

      // Neighboring wheels may not have bogeys yet.
      if (cit != candidates.begin())
      {
        Peak * prevCand = * prev(cit);
        if (prevCand->isLeftWheel())
          prevCand->markBogey(BOGEY_LEFT);
      }

      if (next(ncit) != candidates.end())
      {
        Peak * nextNextCand = * next(ncit);
        if (nextNextCand->isRightWheel())
          nextNextCand->markBogey(BOGEY_RIGHT);
      }
    }
  }

  PeakMinima::printAllCandidates(candidates, "All peaks using bogeys");

  vector<Peak> bogeys;
  PeakMinima::makeCarAverages(candidates, bogeys);

  PeakMinima::printPeakQuality(bogeys[0], "Left bogey, left wheel average");
  PeakMinima::printPeakQuality(bogeys[1], "Left bogey, right wheel average");
  PeakMinima::printPeakQuality(bogeys[2], "Right bogey, left wheel average");
  PeakMinima::printPeakQuality(bogeys[3], "Right bogey, right wheel average");
}


void PeakMinima::printPeak(
  const Peak& peak,
  const string& text) const
{
  cout << text << "\n";
  cout << peak.strHeader();
  cout << peak.str(0) << endl;
}


void PeakMinima::printPeakQuality(
  const Peak& peak,
  const string& text) const
{
  cout << text << "\n";
  cout << peak.strHeaderQuality();
  cout << peak.strQuality(offset) << endl;
}


void PeakMinima::printRange(
  const unsigned start,
  const unsigned end,
  const string& text) const
{
  cout << text << " " << start + offset << "-" << end + offset << endl;
}


void PeakMinima::printWheelCount(
  const list<Peak *>& candidates,
  const string& text) const
{
  unsigned count = 0;
  for (auto cand: candidates)
  {
    if (cand->isWheel())
      count++;
  }
  cout << text << " " << count << " peaks" << endl;
}


void PeakMinima::printAllCandidates(
  const list<Peak *>& candidates,
  const string& text) const
{
  if (candidates.empty())
    return;

  if (text != "")
    cout << text << "\n";
  cout << candidates.front()->strHeaderQuality();
  for (auto cand: candidates)
    cout << cand->strQuality(offset);
  cout << endl;
}


void PeakMinima::printSeedCandidates(
  const list<Peak *>& candidates,
  const string& text) const
{
  if (candidates.empty())
    return;

  if (text != "")
    cout << text << "\n";
  cout << candidates.front()->strHeaderQuality();
  for (auto candidate: candidates)
  {
    if (candidate->isSelected())
      cout << candidate->strQuality(offset);
  }
  cout << endl;
}


void PeakMinima::mark(
  list<Peak>& peaks,
  list<Peak *>& candidates,
  const unsigned offsetIn)
{
  candidates.clear();
  offset = offsetIn;

  PeakMinima::setCandidates(peaks, candidates);

  PeakMinima::markSinglePeaks(candidates);
  PeakMinima::markBogeys(candidates);

  Gap shortGap;
  PeakMinima::markShortGaps(candidates, shortGap);
  PeakMinima::markLongGaps(candidates, shortGap.count);
}
