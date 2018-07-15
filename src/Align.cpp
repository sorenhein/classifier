#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "Align.h"
#include "Database.h"

#include "Metrics.h"

extern Metrics metrics;


Align::Align()
{
}


Align::~Align()
{
}


void Align::scaleToRef(
  const vector<PeakTime>& times,
  const vector<PeakPos>& refPeaks,
  const int left,
  const int right,
  vector<PeakPos>& positions) const
{
  double s0, s1, t0, t1;
  const int lr = static_cast<int>(refPeaks.size());
  const int lt = static_cast<int>(times.size());

  if (left <= 0)
  {
    if (-left >= lt)
    {
      cout << "Time trace too short\n";
      return;
    }

    s0 = refPeaks[0].pos;
    t0 = times[-left].time;
  }
  else
  {
    if (left >= lr)
    {
      cout << "Reference trace too short\n";
      return;
    }

    s0 = refPeaks[left].pos;
    t0 = times[0].time;
  }

  if (right >= 0)
  {
    if (right >= lt)
    {
      cout << "Time trace too short\n";
      return;
    }

    s1 = refPeaks[lr-1].pos;
    t1 = times[lt-1-right].time;
  }
  else
  {
    if (-right >= lr)
    {
      cout << "Reference trace too short\n";
      return;
    }
    s1 = refPeaks[lr-1+right].pos;
    t1 = times[lt-1].time;
  }

  positions.resize(lt);
  const double factor = (s1 - s0) / (t1 - t0);

  for (int i = 0; i < lt; i++)
  {
    positions[i].pos = s0 + factor * (times[i].time - t0);
    positions[i].value = 1.;
  }
}


void Align::getAlignment(
  const vector<PeakPos>& refPeaks,
  const vector<PeakPos>& positions,
  Alignment& alignment) const
{
  const unsigned lp = positions.size();
  const unsigned lr = refPeaks.size();

  alignment.numAdd = 0;
  alignment.numDelete = 0;
  alignment.dist = 0.;
  alignment.actualToRef.resize(lp);

  unsigned lastRef = 0;
  double lastDist = numeric_limits<double>::max();
  vector<int> seen(lr);
  for (unsigned j = 0; j < lr; j++)
    seen[j] = 0;

  double tmp;
  for (unsigned i = 0; i < lp; i++)
  {
    const double pos = positions[i].pos;
    tmp = pos - refPeaks[lastRef].pos;
    const double distBack = tmp * tmp;

    // Look for the best forward match.
    double distBest = numeric_limits<double>::max();
    unsigned jBest = 0;
    for (unsigned j = lastRef+1; j < lr; j++)
    {
      tmp = pos - refPeaks[j].pos;
      const double dist = tmp * tmp;
      if (dist < distBest)
      {
        jBest = j;
        distBest = dist;
      }

      if (refPeaks[j].pos > pos)
        break;
    }

    if (distBest < distBack)
    {
      // Go forward.
      alignment.actualToRef[i] = jBest;
      alignment.dist += distBest;
      seen[jBest] = 1;
      lastDist = distBest;
      lastRef = jBest;
    }
    else if (distBack < lastDist)
    {
      // We're closer than the previous one was.
      alignment.actualToRef[i] = lastRef;
      alignment.dist += distBack;
      lastDist = distBack;
      if (i > 0)
      {
        alignment.actualToRef[i-1] = -1;
        alignment.numAdd++;
      }
    }
    else
    {
      // End.  The peak was surplus (added).
      alignment.actualToRef[i] = -1;
      alignment.numAdd++;
    }
  }

  // Count the unused reference peaks (deletions).
  for (unsigned j = 0; j < lr; j++)
  {
    if (! seen[j])
      alignment.numDelete++;
  }

  // Very simple combination -- TODO.
  alignment.dist += alignment.numAdd + alignment.numDelete;
}


void Align::bestMatches(
  const vector<PeakTime>& times,
  const Database& db,
  const int trainNo,
  const unsigned maxFronts,
  const vector<int>& matchesHist,
  const unsigned tops,
  vector<Alignment>& matches) const
{

  vector<PeakPos> refPeaks, positions; 
  Alignment a;

  for (auto m: matchesHist)
  {
    a.trainNo = m;
    db.getPerfectPeaks(m, refPeaks);

    for (int left = -2; left <= static_cast<int>(maxFronts+2); left++)
    {
      for (int right = -2; right <= 2; right++)
      {
        Align::scaleToRef(times, refPeaks, left, right, positions);
        Align::getAlignment(refPeaks, positions, a);

        const unsigned numUsed = times.size() - a.numAdd;
        const bool correctFlag = (trainNo == m && left == 0 && right == 0);
        a.dist = metrics.distanceAlignment(
          a.dist, numUsed, a.numAdd, a.numDelete, correctFlag);

a.left = left;
a.right = right;
        matches.push_back(a);
      }
    }
  }

  sort(matches.begin(), matches.end());

  if (tops < matches.size())
    matches.resize(tops);
}


void Align::print(const string& fname) const
{
  ofstream fout;
  fout.open(fname);

  fout.close();
}

