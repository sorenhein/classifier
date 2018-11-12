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


bool PeakMatch::advance(
  const list<Peak>& peaks,
  list<Peak>::const_iterator& peak) const
{
  while (peak != peaks.end() && ! peak->isSelected())
    peak++;
  return (peak != peaks.end());
}


double PeakMatch::simpleScore(
  const list<Peak>& peaks,
  const vector<PeakTime>& timesTrue,
  const double offsetScore,
  double& shift)
{
  // This is similar to Align::simpleScore.
  
  list<Peak>::const_iterator peak = peaks.begin();
  if (! PeakMatch::advance(peaks, peak))
    THROW(ERR_NO_PEAKS, "No peaks present or selected");

  const auto peakFirst = peak;
  list<Peak>::const_iterator peakBest, peakPrev;
  double score = 0.;
  unsigned scoring = 0;

  for (unsigned tno = 0; tno < timesTrue.size(); tno++)
  {
    double d = TIME_PROXIMITY, dabs = TIME_PROXIMITY;
    const double timeTrue = timesTrue[tno].time;
    double timeSeen = peak->getTime() - offsetScore;

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
      while (PeakMatch::advance(peaks, peak))
      {
        timeSeen = peak->getTime() - offsetScore;
        dright = timeSeen - timeTrue;
        if (dright >= 0.)
          break;
        else
        {
          peakPrev = peak;
          peak++;
        }
      }

      if (dright < 0.)
      {
        // We reached the end.
        dabs = -dright;
        d = dright;
        peakBest = peakPrev;
      }
      else 
      {
        dleft = timeTrue - (peakPrev->getTime() - offsetScore);
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
      // TODO
      // if (logFlag)
      //   peakBest->logMatch(tno);
      // In firstMatch, the first call has logFlag == false,
      // the second one true.
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
      PeakMatch::simpleScore(peaks, timesTrue, offsetList[i], shiftNew);

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
  shift += offsetList[ino];
  score = PeakMatch::simpleScore(peaks, timesTrue, shift, tmp);

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

  if (debugDetails)
    PeakMatch::printPeaks(peaks, timesTrue);

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

  // Make statistics.
  vector<unsigned> seenTrue(posTrue.size(), 0);
  unsigned seen = 0;
  for (auto& peak: peaks)
  {
    if (peak.isCandidate())
    {
      const int m = peak.getMatch();
      const PeakType pt = peak.getType();
      if (m >= 0)
      {
        peakStats.logSeenMatch(static_cast<unsigned>(m), lt, pt);
        if (seenTrue[m] && debug)
          cout << "Already saw true peak " << m << endl;
        seenTrue[m]++;
        seen++;
      }
      else
      {
        peakStats.logSeenMiss(pt);
      }
    }
  }

  if (debug)
  {
    cout << "True train " << trainTrue << " at " << 
      fixed << setprecision(2) << speedTrue << " m/s\n\n";

    cout << seen << " of the observed peaks are close to true ones (" << 
      posTrue.size() << " true peaks)" << endl;
  }

  for (unsigned m = 0; m < posTrue.size(); m++)
  {
    if (seenTrue[m])
      continue;

    // Look for posTrue[m].time vs (peaks time - shift),
    // must be within TIME_PROXIMITY.
    PeakType ctype = 
      PeakMatch::findCandidate(peaks, timesTrue[m].time, shift);

    if (ctype == PEAK_TENTATIVE)
      THROW(ERR_ALGO_PEAK_MATCH, "Odd: Tentative matched again");

    // The missed ones are detected as early/late/missing.
    if (ctype == PEAK_TRUE_MISSING)
      peakStats.logTrueReverseMiss(m, lt);
    else
      peakStats.logTrueReverseMatch(m, lt, ctype);
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

