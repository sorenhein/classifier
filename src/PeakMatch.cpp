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
#define TIME_PROXIMITY 0.03

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
  const vector<double>& posTrue, 
  const double speed,
  vector<PeakTime>& timesTrue) const
{
  for (unsigned i = 0; i < posTrue.size(); i++)
  {
    timesTrue[i].time = posTrue[i] / speed;
    timesTrue[i].value = 1.;
  }
}


bool PeakMatch::advance(list<PeakWrapper>::iterator& peak) const
{
  while (peak != peaksWrapped.end() && ! peak->peakPtr->isSelected())
    peak++;
  return (peak != peaksWrapped.end());
}


double PeakMatch::simpleScore(
  const vector<PeakTime>& timesTrue,
  const double offsetScore,
  const bool logFlag,
  double& shift)
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
  double score = 0.;
  unsigned numScores = 0;

  for (unsigned tno = 0; tno < timesTrue.size(); tno++)
  {
    const double timeTrue = timesTrue[tno].time;
    double timeSeen = (peak == peaksWrapped.end() ?
      peakPrev->peakPtr->getTime() :
      peak->peakPtr->getTime()) - offsetScore;

    double d = timeSeen - timeTrue;
    double dabs = TIME_PROXIMITY;
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
      double dleft = numeric_limits<double>::max();
      double dright = numeric_limits<double>::max();
      while (PeakMatch::advance(peak))
      {
        timeSeen = peak->peakPtr->getTime() - offsetScore;
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
        dleft = timeTrue - (peakPrev->peakPtr->getTime() - offsetScore);
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
  const vector<PeakTime>& timesTrue,
  vector<double>& offsetList) const
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

    offsetList[i] = peak->getTime() - timesTrue[lt-1].time;
  }

  peak = peaks.topConst().cend();
  do
  {
    peak = prev(peak);
  }
  while (peak != peaks.topConst().cbegin() && ! peak->isSelected());

  const double lastTime = peak->getTime();

  for (unsigned i = 1; i <= MAX_SHIFT_TRUE; i++)
    offsetList[MAX_SHIFT_SEEN+i] = timesTrue[lt-1-i].time - lastTime;
}


bool PeakMatch::findMatch(
  const PeakPool& peaks,
  const vector<PeakTime>& timesTrue,
  double& shift)
{
  vector<double> offsetList;
  PeakMatch::setOffsets(peaks, timesTrue, offsetList);

  double score = 0.;
  unsigned ino = 0;
  shift = 0.;
  for (unsigned i = 0; i < offsetList.size(); i++)
  {
    double shiftNew = 0.;
    double scoreNew = 
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
  double tmp;
  score = PeakMatch::simpleScore(timesTrue, shift, true, tmp);

  return (score >= SCORE_CUTOFF * timesTrue.size());
}


void PeakMatch::correctTimesTrue(vector<PeakTime>& timesTrue) const
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

  const double factor = 
    (peakLast->getTime() - peakFirst->getTime()) /
    (timesTrue[lt-1].time - timesTrue[lt-lp].time);

  const double base = timesTrue[0].time;
  for (auto& t: timesTrue)
    t.time = (t.time - base) * factor + base;
}
  

void PeakMatch::logPeakStats(
  const PeakPool& peaks,
  const vector<double>& posTrue,
  const string& trainTrue,
  const double speedTrue,
  PeakStats& peakStats)
{
  // TODO Could be global flags.
  const bool debug = true;
  const bool debugDetails = false;

  const unsigned lt = posTrue.size();

  // Scale true positions to true times.
  vector<PeakTime> timesTrue;
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
  double shift = 0.;
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
  const vector<PeakTime>& timesTrue) const
{
  cout << "true\n";
  for (unsigned i = 0; i < timesTrue.size(); i++)
    cout << i << ";" <<
      fixed << setprecision(6) << timesTrue[i].time << "\n";

  cout << "\n" << peaks.topConst().strTimesCSV(&Peak::isCandidate, "seen");
}

