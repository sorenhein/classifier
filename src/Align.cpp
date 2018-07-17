#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "Align.h"
#include "Database.h"

#include "Metrics.h"

extern Metrics metrics;

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))

#define INDEL_PENALTY 10.
#define EARLY_MISS_PENALTY 3.
#define MAX_EARLY_MISSES 2


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

  for (unsigned j = 0; j < lr; j++)
  {
    // Try to fix (n, -, -, n+1, n+4) -> (n, n+1, n+2, n+3, n+4).
    if (seen[j] || j == 0 || alignment.actualToRef[j-1] == -1)
      continue;

    unsigned k;
    for (k = j+1; k < lr && ! seen[k]; k++);
    if (! seen[k])
      continue;

    if (k == lr-1 || ! seen[k+1])
      continue;

    if (alignment.actualToRef[k+1] - alignment.actualToRef[j-1] == 
      static_cast<int>(k+2-j))
    {
      for (unsigned l = j; l <= k; l++)
        alignment.actualToRef[l] = alignment.actualToRef[j-1] + l+1-j;
      alignment.numDelete -= k-j;
      alignment.numAdd -= k-j;
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


void Align::NeedlemanWunsch(
  const vector<PeakPos>& refPeaks,
  const vector<PeakTime>& times,
  double scale,
  Alignment alignment) const
{
  // https://en.wikipedia.org/wiki/Needleman%E2%80%93Wunsch_algorithm
  // This is an adaptation of the sequence-matching algorithm.
  // The "letters" are peak positions in meters, so the metric
  // between letters is the square of the physical distance.
  // The time sequence is scaled by the factor that is approximated from
  // the histogram matching.
  // The first dimension is refPeaks, the second is the synthetic one.
  // The penalty for an addition (to the synthetic trace) is constant.
  // The penalty for a deletion (from the synthetic one, i.e. a spare
  // peak in the reference) is lower for the first couple of deletions,
  // as the sensor tends to miss more in the beginning.
  
  const unsigned lr = refPeaks.size();
  const unsigned lt = times.size();

  vector<PeakPos> scaledPeaks(lt);
  for (unsigned j = 0; j < lt; j++)
    scaledPeaks[j].pos = scale * times[j].time;

  // Set up the matrix.
  enum Origin
  {
    NW_MATCH = 0,
    NW_DELETE = 1,
    NW_INSERT = 2
  };

  struct Mentry
  {
    double dist;
    Origin origin;
  };

  vector<vector<Mentry>> matrix;
  matrix.resize(lr+1);
  for (unsigned i = 0; i < lr+1; i++)
    matrix[i].resize(lt+1);

  matrix[0][0].dist = 0.;
  for (unsigned i = 1; i <= MAX_EARLY_MISSES; i++)
    matrix[0][i].dist = EARLY_MISS_PENALTY;
  for (unsigned i = MAX_EARLY_MISSES+1; i < lr+1; i++)
    matrix[0][i].dist = INDEL_PENALTY;

  for (unsigned j = 1; j < lt+1; j++)
    matrix[j][0].dist = INDEL_PENALTY;

  // Run the dynamic programming.
  for (unsigned i = 1; i < lr+1; i++)
  {
    for (unsigned j = 1; j < lt+1; j++)
    {
      const double d = refPeaks[i-1].pos - scaledPeaks[i-1].pos;
      const double match = matrix[i-1][j-1].dist + d * d;
      const double del = matrix[i-1][j].dist + INDEL_PENALTY;
      const double ins = matrix[i][j-1].dist + INDEL_PENALTY;

      if (match <= del)
      {
        if (match <= ins)
          matrix[i][j].origin = NW_MATCH;
        else
          matrix[i][j].origin = NW_INSERT;
      }
      else if (del <= ins)
        matrix[i][j].origin = NW_DELETE;
      else
        matrix[i][j].origin = NW_INSERT;

      const double m = min(match, del);
      matrix[i][j].dist = min(ins, m);
    }
  }

  // Walk back through the matrix.
  alignment.dist = matrix[lr][lt].dist;
  alignment.numAdd = 0; // Spare peaks in scaledPeaks
  alignment.numDelete = 0; // Unused peaks in refPeaks
  
  unsigned i = lr;
  unsigned j = lt;
  while (i > 0 || j > 0)
  {
    const Origin o = matrix[i][j].origin;
    if (i > 0 && j > 0 && o == NW_MATCH)
    {
      alignment.actualToRef[j-1] = i-1;
      i--;
      j--;
    }
    else if (i > 0 && o == NW_INSERT)
    {
      alignment.actualToRef[j-1] = -1;
      alignment.numAdd++;
      j--;
    }
    else
    {
      alignment.numDelete++;
      i--;
    }
  }
}


void Align::bestMatches(
  const vector<PeakTime>& times,
  const Database& db,
  const int trainNo,
  const vector<HistMatch>& matchesHist,
  const unsigned tops,
  vector<Alignment>& matches) const
{
  UNUSED(trainNo); // TODO Log good and bad values, metrics?

  vector<PeakPos> refPeaks; 
  Alignment a;

  for (auto& mh: matchesHist)
  {
    a.trainNo = mh.trainNo;
    db.getPerfectPeaks(a.trainNo, refPeaks);

    Align::NeedlemanWunsch(refPeaks, times, mh.scale, a);
    matches.push_back(a);
  }

  sort(matches.begin(), matches.end());

  if (tops < matches.size())
    matches.resize(tops);
}


void Align::bestMatchesOld(
  const vector<PeakTime>& times,
  const Database& db,
  const int trainNo,
  const unsigned maxFronts,
  const vector<HistMatch>& matchesHist,
  const unsigned tops,
  vector<Alignment>& matches) const
{

  vector<PeakPos> refPeaks, positions; 
  Alignment a;

  for (auto& mh: matchesHist)
  {
    int m = mh.trainNo;
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

