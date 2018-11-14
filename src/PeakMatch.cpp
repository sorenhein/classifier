#include <list>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "PeakMatch.h"

#include "Except.h"
#include "errors.h"

// A measure (in s) of how close we would like real and seen peaks
// to be.
#define TIME_PROXIMITY 0.03

// This fraction of the peaks should be present in the sipmle score.
#define SCORE_CUTOFF 0.75


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
  const vector<PeakPos>& posTrue, 
  const double speed,
  vector<PeakTime>& timesTrue) const
{
  for (unsigned i = 0; i < posTrue.size(); i++)
  {
    timesTrue[i].time = posTrue[i].pos / speed;
    timesTrue[i].value = posTrue[i].value;
  }
}


bool PeakMatch::advance(list<PeakWrapper>::const_iterator& peak) const
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
  
  list<PeakWrapper>::iterator peak = peaksWrapped.begin();
  if (! PeakMatch::advance(peak))
    THROW(ERR_ALGO_PEAK_MATCH, "No peaks present or selected");

  list<PeakWrapper>::iterator peakBest, peakPrev;
  double score = 0.;
  unsigned scoring = 0;

  for (unsigned tno = 0; tno < timesTrue.size(); tno++)
  {
    double d = TIME_PROXIMITY, dabs = TIME_PROXIMITY;
    const double timeTrue = timesTrue[tno].time;
    double timeSeen = (peak == peaksWrapped.end() ?
      peakPrev->peakPtr->getTime() :
      peak->peakPtr->getTime()) - offsetScore;

    d = timeSeen - timeTrue;
    if (d >= 0.)
    {
      dabs = d;
      peakBest = peak;
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
      scoring++;
    }

    if (logFlag)
    {
      if (dabs < peakBest->bestDistance)
      {
        peakBest->match = tno;
        peakBest->bestDistance = dabs;
      }
      trueMatches[tno] = peakBest->peakPtr;
    }
  }

  if (scoring)
    shift /= scoring;

  return score;
}


void PeakMatch::setOffsets(
  const list<Peak>& peaks,
  const vector<PeakTime>& timesTrue,
  vector<double>& offsetList) const
{
  const unsigned lp = peaks.size();
  const unsigned lt = timesTrue.size();

  // Probably too many.
  const unsigned maxShiftSeen = 3;
  const unsigned maxShiftTrue = 3;
  if (lp <= maxShiftSeen || lt <= maxShiftTrue)
    THROW(ERR_NO_PEAKS, "Not enough peaks");

  offsetList.resize(maxShiftSeen + maxShiftTrue + 1);

  // Offsets are subtracted from the seen peaks.
  list<Peak>::const_iterator peak = peaks.end();
  for (unsigned i = 0; i <= maxShiftSeen; i++)
  {
    do
    {
      peak = prev(peak);
    }
    while (peak != peaks.begin() && ! peak->isSelected());

    offsetList[i] = peak->getTime() - timesTrue[lt-1].time;
  }

  peak = peaks.end();
  do
  {
    peak = prev(peak);
  }
  while (peak != peaks.begin() && ! peak->isSelected());

  const double lastTime = peak->getTime();

  for (unsigned i = 1; i <= maxShiftTrue; i++)
    offsetList[maxShiftSeen+i] = timesTrue[lt-1-i].time - lastTime;
}


bool PeakMatch::findMatch(
  const list<Peak>& peaks,
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

    if (scoreNew > score)
    {
      score = scoreNew;
      shift = shiftNew;
      ino = i;
    }
  }

  // This extra run could be eliminated at the cost of some copying
  // and intermediate storage.
  double tmp;
  shift += offsetList[ino]; // TODO WTF?
  score = PeakMatch::simpleScore(timesTrue, shift, true, tmp);

  if (score >= SCORE_CUTOFF * timesTrue.size())
    return true;
  else
    return false;
}
  

PeakType PeakMatch::findCandidate(
  const list<Peak>& peaks,
  const double time,
  const double shift) const
{
  double dist = numeric_limits<double>::max();
  PeakType type = PEAK_TRUE_MISSING;

  for (auto& peak: peaks)
  {
    if (! peak.isCandidate() || peak.isSelected())
      continue;
    
    const double pt = peak.getTime() - shift;
    const double distNew = time - pt;
    if (abs(distNew) < dist)
    {
      dist = abs(distNew);
      if (dist < TIME_PROXIMITY)
        type = peak.getType();
    }

    if (distNew < 0.)
      break;
  }
  return type;
}


void PeakMatch::logPeakStats(
  const list<Peak>& peaks,
  const vector<PeakPos>& posTrue,
  const string& trainTrue,
  const double speedTrue,
  PeakStats& peakStats)
{
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

  // Make a wrapper such that we don't have to modify the peaks
  // themselves
  for (auto& peak: peaks)
  {
    peaksWrapped.emplace_back(PeakWrapper());
    PeakWrapper& pw = peaksWrapped.back();

    pw.peakPtr = &peak;
    pw.match = -1;
    pw.bestDistance = numeric_limits<float>::max();
  }

  // Find a good line-up.
  double shift = 0.;
  if (! PeakMatch::findMatch(peaks, timesTrue, shift))
  {
    if (debug)
    {
      cout << "No good match to real " << posTrue.size() << " peaks.\n";
      cout << "\nTrue train " << trainTrue << " at " << 
        fixed << setprecision(2) << speedTrue << " m/s" << endl << endl;
    }
    return;
  }

  // TODO Isn't is possible that a single seen peak is the closest one
  // to several true peaks?  Then we should mark the one with the 
  // lowest distance in simpleDistance?

  // Go through the candidates we've seen (negative minima).  They were 
  // either mapped to a true peak or not.  Some of the candidates were
  // selected, so their type is TENTATIVE, TRANS_FRONT or TRANS_BACK.
  // Some of them weren't, so they still have the default type.
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
        if (m < PEAKSTATS_END_COUNT)
          peakStats.logSeenHit(PEAK_SEEN_EARLY);
        else if (static_cast<unsigned>(m) + (PEAKSTATS_END_COUNT+1) > lt)
          peakStats.logSeenHit(PEAK_SEEN_LATE);
        else
          peakStats.logSeenHit(PEAK_SEEN_CORE);

        seenTrue[m]++;
        seen++;
      }
      else
      {
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
  //
  // TODO Rename TRANS_* to SEEN_*, TENTATIVE to SEEN_MAIN

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
      peakStats.logTrueMiss(m, lt, PEAK_TRUE2_TOO_EARLY);
    else if (m > mLast)
      peakStats.logTrueMiss(m, lt, PEAK_TRUE2_TOO_LATE);
    else
      peakStats.logTrueMiss(m, lt, PEAK_TRUE2_MISSED);
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
  const list<Peak>& peaks,
  const vector<PeakTime>& timesTrue) const
{
  cout << "true\n";
  for (unsigned i = 0; i < timesTrue.size(); i++)
    cout << i << ";" <<
      fixed << setprecision(6) << timesTrue[i].time << "\n";

  cout << "\nseen\n";
  unsigned pp = 0;
  for (auto& peak: peaks)
  {
    if (peak.isCandidate() && peak.getType() == PEAK_TENTATIVE)
    {
      cout << pp << ";" <<
        fixed << setprecision(6) << peak.getTime() << endl;
      pp++;
    }
  }
  cout << "\n";
}

