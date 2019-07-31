#include <list>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <limits>

#include "PeakMatch.h"
#include "stats/PeakStats.h"

#include "Except.h"
#include "errors.h"

// A measure (in s) of how close we would like real and seen peaks
// to be, for purposes of guessing the right shift between seen and
// true peaks.
#define TIME_PROXIMITY 0.03f

// This fraction of the seen peaks should count in the sipmle score.
// Effectively this fraction of the peaks must match up in order for
//  the shift to be considered valid.
#define SCORE_CUTOFF 0.75

// These are the maximum shifts that we will consider when matching
// true and seen peaks.  If the number is 3, then we will skip up
// to this many peaks.
#define MAX_SHIFT_SEEN 3
#define MAX_SHIFT_TRUE 3

PeakMatch::PeakMatch()
{
  PeakMatch::reset();
}


PeakMatch::~PeakMatch()
{
}


void PeakMatch::reset()
{
  peaksWrapped.clear();
  trueMatches.clear();
}


void PeakMatch::pos2time(
  const vector<float>& posTrue, 
  const double speed,
  vector<float>& timesTrue) const
{
  for (unsigned i = 0; i < posTrue.size(); i++)
    timesTrue[i] = posTrue[i] / static_cast<float>(speed);
}


bool PeakMatch::advance(list<PeakWrapper>::iterator& peak) const
{
  while (peak != peaksWrapped.end() && ! peak->peakPtr->isSelected())
    peak++;
  return (peak != peaksWrapped.end());
}


float PeakMatch::simpleScore(
  const vector<float>& timesTrue,
  const float offsetScore,
  const bool logFlag,
  float& shift)
{
  // This is similar to Align::simpleScore.
  // For each true peak, we find the closest seen peak.
  // It can happen that multiple true peaks map to the same seen peak.
  // (This is why it's considered a simple score.)
  // For peaks that are matched up, a distance measure is applied
  // which is 0 if they are too far apart, and which otherwise drops
  // off linearly from 1 in case of a perfect match.
  
  list<PeakWrapper>::iterator peak = peaksWrapped.begin();
  if (! PeakMatch::advance(peak))
    THROW(ERR_ALGO_PEAK_MATCH, "No peaks present or selected");

  list<PeakWrapper>::iterator peakBest, peakPrev;
  float score = 0.f;
  unsigned numScores = 0;

  for (unsigned tno = 0; tno < timesTrue.size(); tno++)
  {
    const float timeTrue = timesTrue[tno];
    float timeSeen = (peak == peaksWrapped.end() ?
      static_cast<float>(peakPrev->peakPtr->getTime()) :
      static_cast<float>(peak->peakPtr->getTime())) - offsetScore;

    float d = timeSeen - timeTrue;
    float dabs = TIME_PROXIMITY;
    if (d >= 0.)
    {
      dabs = d;
      peakBest = (peak == peaksWrapped.end() ? peakPrev : peak);
    }
    else if (peak == peaksWrapped.end())
    {
      dabs = -d;
      peakBest = peakPrev;
    }
    else
    {
      float dleft = numeric_limits<float>::max();
      float dright = numeric_limits<float>::max();
      while (PeakMatch::advance(peak))
      {
        timeSeen = static_cast<float>(peak->peakPtr->getTime()) - offsetScore;
        dright = timeSeen - timeTrue;
        if (dright >= 0.)
          break;
        else
        {
          peakPrev = peak;
          peak++;
        }
      }

      if (dright < 0. || peak == peaksWrapped.end())
      {
        // We reached the end.
        dabs = -dright;
        d = dright;
        peakBest = peakPrev;
      }
      else 
      {
        dleft = timeTrue - (static_cast<float>(peakPrev->peakPtr->getTime()) 
          - offsetScore);
        if (dleft <= dright)
        {
          dabs = dleft;
          d = -dleft;
          peakBest = peakPrev;
        }
        else
        {
          dabs = dright;
          d = dright;
          peakBest = peak;
        }
      }
    }

    if (dabs <= TIME_PROXIMITY)
    {
      score += (TIME_PROXIMITY - dabs) / TIME_PROXIMITY;
      shift += d;
      numScores++;
    }

    if (logFlag)
    {
      if (dabs < peakBest->bestDistance)
      {
        peakBest->match = static_cast<int>(tno);
        peakBest->bestDistance = dabs;
      }
      trueMatches[tno] = peakBest->peakPtr;
    }
  }

  if (numScores)
    shift /= numScores;

  return score;
}


void PeakMatch::setOffsets(
  const PeakPool& peaks,
  const vector<float>& timesTrue,
  vector<float>& offsetList) const
{
  const unsigned lp = peaks.topConst().size();
  const unsigned lt = timesTrue.size();

  // Probably too many.
  if (lp <= MAX_SHIFT_SEEN || lt <= MAX_SHIFT_TRUE)
    THROW(ERR_NO_PEAKS, "Not enough peaks");

  offsetList.resize(MAX_SHIFT_SEEN + MAX_SHIFT_TRUE + 1);

  // Offsets are subtracted from the seen peaks.
  Pciterator peak = peaks.topConst().cend();
  for (unsigned i = 0; i <= MAX_SHIFT_SEEN; i++)
  {
    do
    {
      peak = prev(peak);
    }
    while (peak != peaks.topConst().cbegin() && ! peak->isSelected());

    offsetList[i] = static_cast<float>(peak->getTime()) - timesTrue[lt-1];
  }

  peak = peaks.topConst().cend();
  do
  {
    peak = prev(peak);
  }
  while (peak != peaks.topConst().cbegin() && ! peak->isSelected());

  const float lastTime = static_cast<float>(peak->getTime());

  for (unsigned i = 1; i <= MAX_SHIFT_TRUE; i++)
    offsetList[MAX_SHIFT_SEEN+i] = timesTrue[lt-1-i] - lastTime;
}


bool PeakMatch::findMatch(
  const PeakPool& peaks,
  const vector<float>& timesTrue,
  float& shift)
{
  vector<float> offsetList;
  PeakMatch::setOffsets(peaks, timesTrue, offsetList);

  float score = 0.f;
  unsigned ino = 0;
  shift = 0.;
  for (unsigned i = 0; i < offsetList.size(); i++)
  {
    float shiftNew = 0.f;
    float scoreNew = 
      PeakMatch::simpleScore(timesTrue, offsetList[i], false, shiftNew);

cout << "SCORE i " << i << " " << shiftNew << " " << scoreNew << endl;
    if (scoreNew > score)
    {
      score = scoreNew;
      shift = shiftNew;
      ino = i;
    }
  }

  // We detected a shift from the offset in the list above.
  // We compensate for this and do a final calcuation.
  shift += offsetList[ino];
  float tmp;
  score = PeakMatch::simpleScore(timesTrue, shift, true, tmp);

  return (score >= SCORE_CUTOFF * timesTrue.size());
}


void PeakMatch::correctTimesTrue(vector<float>& timesTrue) const
{
  const unsigned lt = timesTrue.size();

  bool firstSeen = false;
  unsigned lp = 0;
  Peak const * peakFirst = nullptr, * peakLast = nullptr;

  for (auto& pw: peaksWrapped)
  {
    if (pw.peakPtr->isSelected())
    {
      lp++;
      if (! firstSeen)
      {
        firstSeen = true;
        peakFirst = pw.peakPtr;
      }

      peakLast = pw.peakPtr;
    }
  }

  if (lp > lt)
    return;

  // Line up the ending peaks, assuming that any missing peaks
  // are at the beginning.

  const float factor = 
    static_cast<float>((peakLast->getTime() - peakFirst->getTime())) /
    (timesTrue[lt-1] - timesTrue[lt-lp]);

  const float base = timesTrue[0];
  for (auto& t: timesTrue)
    t = (t - base) * factor + base;
}
  

void PeakMatch::logPeakStats(
  const PeakPool& peaks,
  const vector<float>& posTrue,
  const string& trainTrue,
  const double speedTrue,
  PeakStats& peakStats)
{
  // TODO Could be global flags.
  const bool debug = true;
  const bool debugDetails = false;

  const unsigned lt = posTrue.size();

  // Scale true positions to true times.
  vector<float> timesTrue;
  timesTrue.resize(lt);
  PeakMatch::pos2time(posTrue, speedTrue, timesTrue);

  trueMatches.resize(lt);

  if (debugDetails)
    PeakMatch::printPeaks(peaks, timesTrue);

  // Make a wrapper such that we don't have to modify the peaks.
  for (Pciterator pit = peaks.topConst().cbegin();
      pit != peaks.topConst().cend(); pit++)
  {
    peaksWrapped.emplace_back(PeakWrapper());
    PeakWrapper& pw = peaksWrapped.back();

    pw.peakPtr = &* pit;
    pw.match = -1;
    pw.bestDistance = numeric_limits<float>::max();
  }

  // Find a good line-up.
  float shift = 0.;
  if (! PeakMatch::findMatch(peaks, timesTrue, shift))
  {
    // Could be that the "true" speed is off enough to cause a
    // wrong line-up.  Try a different speed.

    trueMatches.clear();
    trueMatches.resize(lt);

    for (auto& pw: peaksWrapped)
    {
      pw.match = -1;
      pw.bestDistance = numeric_limits<float>::max();
    }

    PeakMatch::correctTimesTrue(timesTrue);

    if (! PeakMatch::findMatch(peaks, timesTrue, shift))
    {
      if (debug)
      {
        cout << "Warning: No good match to real " << 
          posTrue.size() << " peaks.\n";
        cout << "\nTrue train " << trainTrue << " at " << 
          fixed << setprecision(2) << speedTrue << " m/s" << 
          endl << endl;
      }
      // Falling through to statistics anyway.
    }
  }

  // Go through the candidates we've seen (negative minima).  They were 
  // either mapped to a true peak or not.
  vector<unsigned> seenTrue(posTrue.size(), 0);
  unsigned seen = 0;
  for (auto& pw: peaksWrapped)
  {
    auto& peak = * pw.peakPtr;
    if (peak.isCandidate())
    {
      const int m = pw.match;
      if (m >= 0)
      {
        // The seen peak was matched to a true peak.
        if (m < PEAKSTATS_END_COUNT)
          peakStats.logSeenHit(PEAK_SEEN_EARLY);
        else if (static_cast<unsigned>(m) + (PEAKSTATS_END_COUNT+1) > lt)
          peakStats.logSeenHit(PEAK_SEEN_LATE);
        else
          peakStats.logSeenHit(PEAK_SEEN_CORE);

        seenTrue[static_cast<unsigned>(m)]++;
        seen++;
      }
      else
      {
        // The seen peak was not matched to a true peak.
        const unsigned pi = peak.getIndex();

        if (pi < trueMatches[0]->getIndex())
          peakStats.logSeenMiss(PEAK_SEEN_TOO_EARLY);
        else if (pi <= trueMatches[PEAKSTATS_END_COUNT]->getIndex())
          peakStats.logSeenMiss(PEAK_SEEN_EARLY);
        else if (pi <= trueMatches[lt-PEAKSTATS_END_COUNT]->getIndex())
          peakStats.logSeenMiss(PEAK_SEEN_CORE);
        else if (pi <= trueMatches[lt-1]->getIndex())
          peakStats.logSeenMiss(PEAK_SEEN_LATE);
        else
          peakStats.logSeenMiss(PEAK_SEEN_TOO_LATE);
      }
    }
  }

  // Several true peaks could be mapped to the same seen peak (only one
  // of them will be thus marked).  The other true peaks were either
  // early, late or just plain missing.
  unsigned mFirst = 0;
  for (unsigned m = 0; m < posTrue.size(); m++)
  {
    if (seenTrue[m])
    {
      mFirst = m;
      break;
    }
  }

  unsigned mLast = 0;
  for (unsigned m = 0; m < posTrue.size(); m++)
  {
    const unsigned mrev = posTrue.size() - m - 1;
    if (seenTrue[mrev])
    {
      mLast = m;
      break;
    }
  }

  for (unsigned m = 0; m < posTrue.size(); m++)
  {
    if (seenTrue[m])
      peakStats.logTrueHit(m, lt);
    else if (m < mFirst)
      peakStats.logTrueMiss(m, lt, PEAK_TRUE_TOO_EARLY);
    else if (m > mLast)
      peakStats.logTrueMiss(m, lt, PEAK_TRUE_TOO_LATE);
    else
      peakStats.logTrueMiss(m, lt, PEAK_TRUE_MISSED);
  }

  if (debug)
  {
    cout << "True train " << trainTrue << " at " << 
      fixed << setprecision(2) << speedTrue << " m/s\n\n";

    cout << seen << " of the observed peaks are close to true ones (" << 
      posTrue.size() << " true peaks)" << endl;
  }
}


void PeakMatch::printPeaks(
  const PeakPool& peaks,
  const vector<float>& timesTrue) const
{
  cout << "true\n";
  for (unsigned i = 0; i < timesTrue.size(); i++)
    cout << i << ";" <<
      fixed << setprecision(6) << timesTrue[i] << "\n";

  cout << "\n" << peaks.topConst().strTimesCSV(&Peak::isCandidate, "seen");
}

