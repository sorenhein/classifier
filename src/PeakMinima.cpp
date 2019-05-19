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


void PeakMinima::makeDistances(
  const PeakPool& peaks,
  const PeakFncPtr& fptr,
  vector<unsigned>& dists) const
{
  // Make list of distances between neighbors for which fptr
  // evaluates to true.
  dists.clear();

  const PeakPtrs& candidates = peaks.candidatesConst();
  PPLciterator cbegin = candidates.cbegin();
  PPLciterator cend = candidates.cend();
  PPLciterator npit;

  for (PPLciterator pit = cbegin; pit != cend; pit = npit)
  {
    if (! ((* pit)->* fptr)())
    {
      npit = next(pit);
      continue;
    }

    npit = candidates.next(pit, &Peak::isSelected);
    if (npit == cend)
      break;

    if (! ((* npit)->* fptr)())
      continue;

    // At least one should also be selected.
    if (! (* pit)->isSelected() && ! (* npit)->isSelected())
      continue;

    dists.push_back((* npit)->getIndex() - (* pit)->getIndex());
  }
}


void PeakMinima::makeSteps(
  const vector<unsigned>& dists,
  list<DistEntry>& steps) const
{
  // We consider a sliding window with range +/- 10% relative to its
  // center.  We want to find the first maximum (i.e. the value at which
  // the count of distances in dists within the range reaches a local
  // maximum).  This is a somewhat rough way to find the lowest
  // "cluster" of values.

  // If an entry in dists is d, then it creates two DistEntry values.
  // One is at 0.9 * d and is a +1, and one is a -1 at 1.1 * d.

  steps.clear();
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

  if (steps.size() == 0)
    return;

  steps.sort();

  // Collapse those steps that have the same index.
  int cumul = 0;
  for (auto sit = steps.begin(); sit != steps.end(); )
  {
    sit->count = 0;
    auto nsit = sit;
    do
    {
      sit->count += nsit->direction;
      nsit++;
    }
    while (nsit != steps.end() && nsit->index == sit->index);

    cumul += sit->count;
    sit->cumul = cumul;

    // Skip the occasional zero count as well (canceling out).
    if (sit->count)
      sit++;

    if (sit != nsit) 
      sit = steps.erase(sit, nsit);
  }

cout << "NEWCOLLAPSE\n";
for (auto& s: steps)
  cout << s.index << ";" << s.count << ";" << s.cumul << endl;
}


void PeakMinima::makePieces(
  const list<DistEntry>& steps,
  list<PieceEntry>& pieces) const
{
  // Segment the steps into pieces where the cumulative value reaches
  // all the way down to zero.  Split each piece into extrema.
  
  pieces.clear();
  for (auto sit = steps.begin(); sit != steps.end(); )
  {
    auto nsit = sit;
    do
    {
      nsit++;
    }
    while (nsit != steps.end() && nsit->cumul != 0);

    // We've got a piece at [sit, nsit).
    pieces.emplace_back(PieceEntry());
    PieceEntry& pe = pieces.back();
    pe.modality = 0;
    pe.extrema.clear();

    for (auto it = sit; it != nsit; it++)
    {
      const bool leftUp = 
        (it == sit ? true : (it->cumul > prev(it)->cumul));
      const bool rightUp = 
        (next(it) == sit ? false : (it->cumul < next(it)->cumul));

      if (leftUp && ! rightUp)
      {
        pe.modality++;
        pe.extrema.emplace_back(DistEntry());
        DistEntry& de = pe.extrema.back();
        de.index = it->index;
        de.direction = 1;
        de.cumul = it->cumul;
      }
      else if (! leftUp && rightUp)
      {
        pe.extrema.emplace_back(DistEntry());
        DistEntry& de = pe.extrema.back();
        de.index = it->index;
        de.direction = -1;
        de.cumul = it->cumul;
      }
    }

    if (nsit == steps.end())
      break;
    else
      sit = next(nsit);
  }

cout << "PIECES\n";
for (auto& p: pieces)
{
  cout << "Modality " << p.modality << "\n";
  for (auto& e: p.extrema)
    cout << "index " << e.index << ", " <<
      (e.direction == 1 ? "MAX" : "min") << ", " <<
      e.cumul << endl;
}
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
cout << step << ";" << count << "\n";

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


bool PeakMinima::formBogieGap(
  const Peak * p1,
  const Peak * p2) const
{
  return (p1->isRightWheel() && p2->isLeftWheel());
}


bool PeakMinima::guessNeighborDistance(
  const PeakPool& peaks,
  const CandFncPtr fptr,
  Gap& gap,
  const unsigned minCount) const
{
  // Make list of distances between neighbors for which fptr
  // evaluates to true.
  vector<unsigned> dists;
  const PeakPtrs& candidates = peaks.candidatesConst();
  PPLciterator cbegin = candidates.cbegin();
  PPLciterator cend = candidates.cend();
  for (PPLciterator pit = cbegin; pit != cend; pit++)
  {
    PPLciterator npit = candidates.next(pit, &Peak::isSelected);
    if (npit == cend)
      break;

    if ((this->* fptr)(* pit, * npit))
      dists.push_back(
        (*npit)->getIndex() - (*pit)->getIndex());
  }
// cout << "Preparing to guess distance: Have " <<
  // dists.size() << " vs. " << minCount << endl;

  if (dists.size() < minCount)
    return false;

  // Guess their distance range.
  sort(dists.begin(), dists.end());

  const unsigned revisedMinCount = min(minCount, dists.size() / 4);
// cout << "Revised count " << revisedMinCount << endl;
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



void PeakMinima::markBogieShortGap(
  Peak& p1,
  Peak& p2,
  PeakPool& peaks,
  PPLiterator& cit, // Iterator to p1
  PPLiterator& ncit, // Iterator to p2
  const string& text) const
{
  if (text != "")
    PeakMinima::printRange(p1.getIndex(), p2.getIndex(),
      text + " short car gap at");

  PeakPtrs& candidates = peaks.candidates();
  if (p1.isRightWheel() && ! p1.isRightBogie())
  {
    // Paired previous wheel was not yet marked a bogie.
    PPLiterator prevCand = candidates.prev(cit, &Peak::isLeftWheel);
    if (prevCand == candidates.end())
    {
      // TODO Is this an algorithmic error?
      cout << "Miss earlier matching peak";
    }
    else
      (* prevCand)->markBogieAndWheel(BOGIE_RIGHT, WHEEL_LEFT);
  }

  if (p2.isLeftWheel() && ! p2.isLeftBogie())
  {
    // Paired next wheel was not yet marked a bogie.
    PPLiterator nextCand = candidates.next(ncit, &Peak::isRightWheel);

    if (nextCand == candidates.end())
      cout << "Miss later matching peak";
    else
      (* nextCand)->markBogieAndWheel(BOGIE_LEFT, WHEEL_RIGHT);
  }

  p1.markBogieAndWheel(BOGIE_RIGHT, WHEEL_RIGHT);
  p2.markBogieAndWheel(BOGIE_LEFT, WHEEL_LEFT);
}


void PeakMinima::markBogieLongGap(
  Peak& p1,
  Peak& p2,
  const string& text) const
{
  if (text != "")
    PeakMinima::printRange(p1.getIndex(), p2.getIndex(),
      text + " long car gap at");
  
  p1.markBogie(BOGIE_LEFT);
  p2.markBogie(BOGIE_RIGHT);
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
  const Gap& wheelGap) const
{
  PeakPtrs& candidates = peaks.candidates();
  PPLiterator cbegin = candidates.begin();
  PPLiterator cend = candidates.end();

  // Here we mark bogies where both peaks are already selected.
  for (auto cit = cbegin; cit != prev(cend); cit++)
  {
    Peak * cand = * cit;
    if (cand->isWheel())
      continue;

    // Don't really need bothSelected then.  Test first one above.
    PPLiterator nextIter = candidates.next(cit, &Peak::isSelected);
    if (nextIter == cend)
      break;
    Peak * nextCand = * nextIter;
      
    if (PeakMinima::bothSelected(cand, nextCand))
    {
      if (cand->matchesGap(* nextCand, wheelGap))
      {
        if (cand->isWheel())
          THROW(ERR_NO_PEAKS, "Triple bogie?!");

        PeakMinima::markWheelPair(* cand, * nextCand, "");
      }
    }
  }
}


void PeakMinima::markBogiesOfUnpaired(
  PeakPool& peaks,
  const Gap& wheelGap) const
{
  PeakPtrs& candidates = peaks.candidates();
  PPLiterator cbegin = candidates.begin();
  PPLiterator cend = candidates.end();

  for (auto cit = cbegin; cit != cend; cit++)
  {
    if (! (* cit)->acceptableQuality())
      continue;

    // If the first one is of acceptable quality, find the first
    // such successor.  There could be poorer peaks in between.
    PPLiterator ncit = candidates.next(cit, &Peak::acceptableQuality);
    if (ncit == cend)
      break;

    // If they are both set already, move on.
    if ((* cit)->isSelected() && (* ncit)->isSelected())
      continue;

    // If the distance is right, we lower our quality requirements.
    if ((* cit)->matchesGap(** ncit, wheelGap))
      PeakMinima::markWheelPair(** cit, ** ncit, "Adding");
  }
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


bool PeakMinima::guessBogieDistance(
  PeakPool& peaks,
  Gap& wheelGap) const
{
  // The wheel gap is only plausible if it hits a certain number of peaks.
  unsigned numGreat = peaks.candidates().count(&Peak::greatQuality);

  if (PeakMinima::guessNeighborDistance(peaks,
        &PeakMinima::bothSelected, wheelGap, numGreat/4) &&
      wheelGap.upper != 0)
    return true;

  // We may get here when one side of the peak pair is so strong
  // and different that the other side never gets picked up.
  // Try again, and lower our standards to acceptable peak quality.
  cout << "First attempt at wheel distance failed: " << numGreat << ".\n";

  if (PeakMinima::guessNeighborDistance(peaks,
        &PeakMinima::bothPlausible, wheelGap, numGreat/4) &&
      wheelGap.upper != 0)
    return true;

  if (PeakMinima::guessNeighborDistance(peaks,
        &PeakMinima::bothPlausible, wheelGap, numGreat/8) &&
      wheelGap.upper != 0)
    return true;

  return false;
}


void PeakMinima::markBogies(
  PeakPool& peaks,
  Gap& wheelGap) const
{
  if (! PeakMinima::guessBogieDistance(peaks, wheelGap))
    THROW(ERR_ALGO_NO_WHEEL_GAP, "Couldn't find wheel gap");

  PeakMinima::printDists(wheelGap.lower, wheelGap.upper,
    "Guessing wheel distance");

  PeakMinima::markBogiesOfSelects(peaks, wheelGap);

  // Look for unpaired wheels where there is a nearby peak that is
  // not too bad.  If there is a spurious peak in between, we'll fail...
  PeakMinima::markBogiesOfUnpaired(peaks, wheelGap);

  vector<Peak> bogieScale;
  makeBogieAverages(peaks, bogieScale);

  // Recalculate the peak qualities using both left and right peaks.
  PeakMinima::reseedBogiesUsingQuality(peaks, bogieScale);

  // Some halves of bogies may have been downgraded.
  PeakMinima::fixBogieOrphans(peaks);

  cout << peaks.candidates().strQuality(
    "All peaks using left/right scales", offset);

  // Recalculate the averages based on the new qualities.
  makeBogieAverages(peaks, bogieScale);
  PeakMinima::printPeakQuality(bogieScale[0], "Left-wheel average");
  PeakMinima::printPeakQuality(bogieScale[1], "Right-wheel average");

  // Redo the distances using the new qualities (left and right peaks).
  const unsigned numGreat = peaks.candidates().count(&Peak::greatQuality);

  PeakMinima::guessNeighborDistance(peaks,
    &PeakMinima::bothSelected, wheelGap, numGreat/4);

  PeakMinima::printDists(wheelGap.lower, wheelGap.upper,
    "Guessing new wheel distance");

  // Mark more bogies with the refined peak qualities.
  PeakMinima::markBogiesOfSelects(peaks, wheelGap);

  PeakMinima::markBogiesOfUnpaired(peaks, wheelGap);

  cout << peaks.candidates().strQuality(
    "All peaks again using left/right scales", offset);
}


void PeakMinima::markShortGapsOfSelects(
  PeakPool& peaks,
  const Gap& shortGap) const
{
  PeakPtrs& candidates = peaks.candidates();
  PPLiterator cbegin = candidates.begin();
  PPLiterator cend = candidates.end();

  // Here we mark short gaps where both peaks are already wheels.
  for (auto cit = cbegin; cit != cend; cit++)
  {
    Peak * cand = * cit;
    if (! cand->isRightWheel())
      continue;

    PPLiterator ncit = candidates.next(cit, &Peak::isSelected);
    if (ncit == cend)
      break;

    Peak * nextCand = * ncit;
    if (! nextCand->isLeftWheel())
      continue;
      
    if (! cand->matchesGap(* nextCand, shortGap))
      continue;

    PeakMinima::markBogieShortGap(* cand, * nextCand, peaks, cit, ncit, "");
  }
}


void PeakMinima::markShortGapsOfUnpaired(
  PeakPool& peaks,
  const Gap& shortGap) const
{
  PeakPtrs& candidates = peaks.candidates();
  PPLiterator cbegin = candidates.begin();
  PPLiterator cend = candidates.end();

  for (PPLiterator cit = cbegin; cit != prev(cend); cit++)
  {
    Peak * cand = * cit;
    PPLiterator ncit = next(cit);
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
      PeakMinima::markBogieShortGap(* cand, * nextCand, peaks,
        cit, ncit, "");
    }
  }
}


void PeakMinima::markShortGaps(
  PeakPool& peaks,
  Gap& shortGap)
{
  // Look for inter-car short gaps.
  PeakMinima::guessNeighborDistance(peaks,
    &PeakMinima::formBogieGap, shortGap);

  PeakMinima::printDists(shortGap.lower, shortGap.upper,
    "Guessing short gap");

  // Mark short gaps (between cars).
  PeakMinima::markShortGapsOfSelects(peaks, shortGap);

  // Look for unpaired short gaps.  If there is a spurious peak
  // in between, we will fail.
  PeakMinima::markShortGapsOfUnpaired(peaks, shortGap);

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


void PeakMinima::markLongGapsOfSelects(
  PeakPool& peaks,
  const Gap& longGap) const
{
  PeakPtrs& candidates = peaks.candidates();
  PPLiterator cbegin = candidates.begin();
  PPLiterator cend = candidates.end();
  for (PPLiterator cit = cbegin; cit != prev(cend); cit++)
  {
    Peak * cand = * cit;
    if (! cand->isWheel() || cand->isRightBogie())
      continue;

    PPLiterator ncit = candidates.next(cit, &Peak::isSelected);
    if (ncit == cend)
      break;

    Peak * nextCand = * ncit;
    if (! nextCand->isWheel())
      continue;

    // Tolerate left-left or right-right.
    if (! cand->isRightWheel() && ! nextCand->isLeftWheel())
      continue;

    if (cand->matchesGap(* nextCand, longGap))
    {
      PeakMinima::markBogieLongGap(* cand, * nextCand, "");

      // Neighboring wheels may not have bogies yet, so we mark them.
      PPLiterator ppcit = candidates.prev(cit, &Peak::isLeftWheel);
      if (ppcit == candidates.end())
        continue;

      if (! (* ppcit)->isBogie())
      {
        (* ppcit)->markBogie(BOGIE_LEFT);
        (* cit)->markBogie(BOGIE_LEFT);
        (* cit)->markWheel(WHEEL_RIGHT);
      }

      PPLiterator nncit = candidates.next(ncit, &Peak::isRightWheel);
      if (nncit != cend)
      {
        (* ncit)->markBogie(BOGIE_RIGHT);
        (* ncit)->markWheel(WHEEL_LEFT);
        (* nncit)->markBogie(BOGIE_RIGHT);
      }
    }
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
  PeakMinima::markLongGapsOfSelects(peaks, longGap);

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
  PeakMinima::markBogiesOfSelects(peaks, wheelGap);
  PeakMinima::markLongGapsOfSelects(peaks, longGap);

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
  PeakMinima::markLongGapsOfSelects(peaks, longGap);

  // Store the average peaks for later reference.
  peaks.logAverages(bogies);
}


void PeakMinima::mark(
  PeakPool& peaks,
  const vector<Peak>& peakCenters,
  const unsigned offsetIn)
{
  offset = offsetIn;

vector<unsigned> dists;
PeakMinima::makeDistances(peaks, &Peak::acceptableQuality, dists);
list<DistEntry> steps;
PeakMinima::makeSteps(dists, steps);
list<PieceEntry> pieces;
PeakMinima::makePieces(steps, pieces);

  PeakMinima::markSinglePeaks(peaks, peakCenters);

unsigned countAll = peaks.candidates().size();
unsigned countSelected = peaks.candidates().count(&Peak::isSelected);
cout << "FRAC " << countSelected << " " << 
  countAll << " " <<
  fixed << setprecision(2) << 100. * countSelected / countAll << endl;

  Gap wheelGap;
  PeakMinima::markBogies(peaks, wheelGap);

peaks.mergeSplits((wheelGap.lower + wheelGap.upper) / 2, offset);

  Gap shortGap;
  PeakMinima::markShortGaps(peaks, shortGap);

  Gap longGap;
  PeakMinima::markLongGaps(peaks, wheelGap, shortGap.count, longGap);

cout << peaks.candidates().strQuality(
  "All selected peaks at end of PeakMinima", offset, &Peak::isSelected);
  
cout << wheelGap.str("PM bogie") << " " <<
  shortGap.str("PM short") << " " <<
  longGap.str("PM long") << "\n";
if (wheelGap.upper == 0)
  cout << "PM ERROR wheel 0\n";
else if (shortGap.upper == 0)
  cout << "PM ERROR short 0\n";
else if (longGap.upper == 0)
  cout << "PM ERROR long 0\n";
else if (shortGap.upper < wheelGap.upper)
  cout << "PM ERROR short < bogie\n";
else if (longGap.lower < shortGap.upper)
  cout << "PM ERROR long < short\n";
else if (longGap.lower < wheelGap.upper)
  cout << "PM ERROR long < wheel\n";
else
  cout << "PM not obviously flawed\n";
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

