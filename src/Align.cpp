#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "Align.h"
#include "Database.h"

// Can adjust these.

#define INDEL_PENALTY 1000.
#define EARLY_MISS_PENALTY 300.
#define MAX_EARLY_MISSES 2


Align::Align()
{
}


Align::~Align()
{
}


void Align::NeedlemanWunsch(
  const vector<PeakPos>& refPeaks,
  const vector<PeakPos>& scaledPeaks,
  Alignment& alignment) const
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
  const unsigned lt = scaledPeaks.size();

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
  {
    matrix[i][0].dist = i * EARLY_MISS_PENALTY;
    matrix[i][0].origin = NW_DELETE;
  }
  const double early = MAX_EARLY_MISSES * EARLY_MISS_PENALTY;
  for (unsigned i = MAX_EARLY_MISSES+1; i < lr+1; i++)
  {
    matrix[i][0].dist = early  + (i - MAX_EARLY_MISSES) * INDEL_PENALTY;
    matrix[i][0].origin = NW_DELETE;
  }

  for (unsigned j = 1; j < lt+1; j++)
  {
    matrix[0][j].dist = j * INDEL_PENALTY;
    matrix[0][j].origin = NW_INSERT;
  }

  // Run the dynamic programming.
  for (unsigned i = 1; i < lr+1; i++)
  {
    for (unsigned j = 1; j < lt+1; j++)
    {
      const double d = refPeaks[i-1].pos - scaledPeaks[j-1].pos;
      const double match = matrix[i-1][j-1].dist + d * d;
      const double del = matrix[i-1][j].dist + INDEL_PENALTY;
      const double ins = matrix[i][j-1].dist + INDEL_PENALTY;

      if (match <= del)
      {
        if (match <= ins)
        {
          matrix[i][j].origin = NW_MATCH;
          matrix[i][j].dist = match;
        }
        else
        {
          matrix[i][j].origin = NW_INSERT;
          matrix[i][j].dist = ins;
        }
      }
      else if (del <= ins)
      {
        matrix[i][j].origin = NW_DELETE;
        matrix[i][j].dist = del;
      }
      else
      {
        matrix[i][j].origin = NW_INSERT;
        matrix[i][j].dist = ins;
      }
    }
  }

  // Walk back through the matrix.
  alignment.dist = matrix[lr][lt].dist;
  alignment.distMatch = 0.;
  alignment.numAdd = 0; // Spare peaks in scaledPeaks
  alignment.numDelete = 0; // Unused peaks in refPeaks
  alignment.actualToRef.resize(lt);
  
  unsigned i = lr;
  unsigned j = lt;
  while (i > 0 || j > 0)
  {
    const Origin o = matrix[i][j].origin;
    if (i > 0 && j > 0 && o == NW_MATCH)
    {
      alignment.actualToRef[j-1] = static_cast<int>(i-1);
      alignment.distMatch += matrix[i][j].dist - matrix[i-1][j-1].dist;
      i--;
      j--;
    }
    else if (j > 0 && o == NW_INSERT)
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


void Align::scalePeaks(
  const vector<PeakTime>& times,
  const double len, // In m
  vector<PeakPos>& scaledPeaks) const
{
  /*
     This is quite approximate, especially in that t_mid is not
     determined very well.  But it seems to be good enough to unwarp
     most of the acceleration.

     len = v0 * t_end + 0.5 * a * t_end^2
     len/2 = v_0 * t_mid + 0.5 * a * t_mid^2

     len * t_mid = v0 * t_end * tmid + 0.5 * a * t_end^2 * t_mid
     len * t_end/2 = v0 * t_mid * t_end + 0.5 * a * t_mid^2 * t_end

     len * (t_mid - t_end/2) = 0.5 * a * t_mid * t_end * (t_end - t_mid)

     a = len * (2 * t_mid - t_end) / [t_mid * t_end * (t_end - t_mid)]
  */

  const unsigned lt = times.size();
  scaledPeaks.resize(lt);

  double tEnd = times.back().time;
  
  double tMid;
  if (lt % 2 == 0)
    tMid = times[lt/2].time;
  else
  {
    const unsigned m = (lt-1) / 2;
    tMid = 0.5 * (times[m].time + times[m+1].time);
  }

  const double accel = len * (2.*tMid - tEnd) /
    (tMid * tEnd * (tEnd - tMid));

  const double speed = (len - 0.5 * accel * tEnd * tEnd) / tEnd;

  for (unsigned j = 0; j < lt; j++)
  {
    scaledPeaks[j].pos = speed * times[j].time +
      0.5 * accel * times[j].time * times[j].time;
  }
}


void Align::bestMatches(
  const vector<PeakTime>& times,
  const Database& db,
  const vector<HistMatch>& matchesHist,
  const unsigned tops,
  vector<Alignment>& matches) const
{
  vector<PeakPos> refPeaks, scaledPeaks;
  Alignment a;

  for (auto& mh: matchesHist)
  {
    a.trainNo = mh.trainNo;
    db.getPerfectPeaks(a.trainNo, refPeaks);

    Align::scalePeaks(times, refPeaks.back().pos - refPeaks.front().pos,
      scaledPeaks);

    Align::NeedlemanWunsch(refPeaks, scaledPeaks, a);
    matches.push_back(a);
  }

  sort(matches.begin(), matches.end());

  if (tops < matches.size())
    matches.resize(tops);
}

