#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <limits>

#include "Align.h"
#include "Database.h"
#include "Timers.h"
#include "print.h"

// Can adjust these.

#define INSERT_PENALTY 5000.
#define DELETE_PENALTY 10000.
#define EARLY_MISS_PENALTY 10.
#define MAX_EARLY_MISSES 2

#define MAX_AXLE_DIFFERENCE_OK 4

// In meters, see below.

#define PROXIMITY_PARAMETER 1.5

extern Timers timers;


Align::Align()
{
}


Align::~Align()
{
}


void Align::NeedlemanWunsch(
  const vector<PeakPos>& refPeaks,
  const vector<PeakPos>& scaledPeaks,
  const double peakScale,
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
    matrix[i][0].dist = early  + (i - MAX_EARLY_MISSES) * DELETE_PENALTY;
    matrix[i][0].origin = NW_DELETE;
  }

  for (unsigned j = 1; j < lt+1; j++)
  {
    matrix[0][j].dist = j * INSERT_PENALTY;
    matrix[0][j].origin = NW_INSERT;
  }

  // Run the dynamic programming.
  for (unsigned i = 1; i < lr+1; i++)
  {
    for (unsigned j = 1; j < lt+1; j++)
    {
      const double d = refPeaks[i-1].pos - scaledPeaks[j-1].pos;
      const double match = matrix[i-1][j-1].dist + peakScale * d * d;
      const double del = matrix[i-1][j].dist + DELETE_PENALTY;
      const double ins = matrix[i][j-1].dist + INSERT_PENALTY;

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


double Align::interpolateTime(
  const vector<PeakTime>& times,
  const double index) const
{
  const unsigned lt = times.size();

// cout << "interpolate: index " << index << endl;
  const unsigned iLeft = static_cast<unsigned>(index);
  if (iLeft+1 >= lt)
  {
    // Shouldn't really happen.
    return times.back().time;
  }

  const unsigned iRight = iLeft + 1;
  const double frac = index - iLeft;
// cout << "iLeft " << iLeft << " iRight " << iRight << " frac " <<
  // frac << endl;

  return (1.-frac) * times[iLeft].time + frac * times[iRight].time;
}


void Align::estimateMotion(
  const vector<PeakPos>& refPeaks,
  const vector<PeakTime>& times,
  const unsigned firstRefNo,
  const unsigned firstTimeNo,
  double& speed,
  double& accel) const
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

  const unsigned lp = refPeaks.size();
  const unsigned lt = times.size();

  const double len = refPeaks[lp-1].pos - refPeaks[firstRefNo].pos;
  const double posMid = refPeaks[firstRefNo].pos + len / 2.;

  // Look for posMid in refPeaks.
  unsigned posLeft = 0;
  while (posLeft+1 < lp && refPeaks[posLeft+1].pos <= posMid)
    posLeft++;
// cout << "posLeft " << posLeft << endl;
  if (posLeft == 0 || posLeft == lp-1 || refPeaks[posLeft+1].pos <= posMid)
    cout << "Should not happen\n";
  const double propRight = (posMid - refPeaks[posLeft].pos) /
    (refPeaks[posLeft+1].pos - refPeaks[posLeft].pos);

// cout << "posMid " << posMid << " propRight " << propRight << endl;

  // Look for the posRight-1 and posRight values in times.
  // If we have the same number of peaks on both sides, this is not hard.
  // In general we interpolate, which does not deal so well with
  // spurious peaks.

  const double tOffset = times[firstTimeNo].time;

  const double tMid0 = Align::interpolateTime(times, 
    firstTimeNo + static_cast<double>(posLeft - firstRefNo) * 
      (lt - firstTimeNo - 1.) / (lp - firstRefNo - 1.));
  const double tMid1 = Align::interpolateTime(times, 
    firstTimeNo + static_cast<double>(posLeft + 1 - firstRefNo) * 
      (lt - firstTimeNo - 1.) / (lp - firstRefNo - 1.));

  const double tMid = (1.-propRight) * tMid0 + propRight * tMid1 -
    tOffset;
  
  double tEnd = times.back().time - tOffset;

  accel = len * (2.*tMid - tEnd) /
    (tMid * tEnd * (tEnd - tMid));
// cout << "len " << len << " tMid " << tMid << " tEnd " << tEnd <<
  // " accel " << accel << endl;

  speed = (len - 0.5 * accel * tEnd * tEnd) / tEnd;
}


double Align::simpleScore(
  const vector<PeakPos>& refPeaks,
  const vector<PeakPos>& shiftedPeaks) const
{
  const unsigned lr = refPeaks.size();
  const unsigned ls = shiftedPeaks.size();

  // If peaks match extremely well, we may get a distance of ~ 1 [m^2]
  // for ~ 50 peaks.  That's 0.02 [m^2] on average, or about 0.15 m of
  // deviation.  If the train goes, say, 50 m/s, that takes 0.003 s.
  // At 2000 samples per second it is 6 samples.
  // We count the distance between each shifted peak and its closest
  // reference peak.  We do not take into account that the same reference
  // peak may match more than one shifted peak (Needleman-Wunsch does
  // this).
  // A distance of d counts as max(0, (1.5 m - d) / 1.5 m).

  unsigned ir = 0;
  double score = 0.;
  double d = PROXIMITY_PARAMETER;

  for (unsigned is = 0; is < ls; is++)
  {
    double dleft = shiftedPeaks[is].pos - refPeaks[ir].pos;

    if (dleft < 0.)
    {
      if (ir == 0)
        d = -dleft;
      else
        cout << "Should not happen" << endl;
    }
    else
    {
      double dright = numeric_limits<double>::max();
      while (ir+1 < lr)
      {
        dright = refPeaks[ir+1].pos - shiftedPeaks[is].pos;
        if (dright >= 0.)
          break;
        ir++;
      }

      dleft = shiftedPeaks[is].pos - refPeaks[ir].pos;
      d = min(dleft, dright);
    }
    
    if (d <= PROXIMITY_PARAMETER)
      score += (PROXIMITY_PARAMETER - d) / PROXIMITY_PARAMETER;
  }

  return score;
}


void Align::scalePeaks(
  const vector<PeakPos>& refPeaks,
  const vector<PeakTime>& times,
  vector<PeakPos>& scaledPeaks) const
{
  // The shift is "subtracted" from scaledPeaks, so if the shift is
  // positive, we align the n'th scaled peak with the 0'th
  // reference peak.  Actually we keep the last peaks aligned,
  // so the shift melts away over the shifted peaks.
  // If the shift is negative, we align the 0'th scaled peak with
  // the n'th reference peak (n = -shift).  
  // In either case we assume that we're matching the last peaks,
  // whether or not the peak numbers make sense.

  struct Shift
  {
    unsigned firstRefNo;
    unsigned firstTimeNo;
    double shift; 
    double score;
    // Sorry...
    bool operator <(const Shift& s2) {return score > s2.score; }
  };

  vector<Shift> candidates(7);
  for (int i = -3; i <= 3; i++)
  {
    // candidates[i+3].no = i;
    if (i >= 0)
    {
      // Align the i'th scaled peak with the 0'th reference peak.
      // We assume i spurious detected peaks.
      candidates[i+3].firstRefNo = 0;
      candidates[i+3].firstTimeNo = i;
    }
    else
    {
      // Align the 0'th scaled peak with the i'th reference peak.
      // We assume that we've missed i reference samples waking up.
      candidates[i+3].shift = -refPeaks[-i].pos;
      candidates[i+3].firstRefNo = static_cast<unsigned>(-i);
      candidates[i+3].firstTimeNo = 0;
    }
  }

  // We calculate a very simple score for this shift.  In fact
  // we could also run Needleman-Wunsch, so this is a bit of an
  // optimization.

// cout << "times\n";
// printPeakTimeCSV(times, 0);

// cout << "refPeaks\n";
// printPeakPosCSV(refPeaks, 1);

unsigned ii = 2;

  const unsigned lt = times.size();
  vector<PeakPos> shiftedPeaks(lt);
  scaledPeaks.resize(lt);

  for (auto& cand: candidates)
  {
    double speed, accel;
    Align::estimateMotion(refPeaks, times, cand.firstRefNo, 
      cand.firstTimeNo, speed, accel);
    // cout << "estimated speed " << speed << ", accel " << accel << endl;

    scaledPeaks.resize(lt);
    for (unsigned j = 0; j < lt; j++)
    {
      scaledPeaks[j].pos = speed * times[j].time +
        0.5 * accel * times[j].time * times[j].time;
    }

    // Couldn't set this sooner.
    if (cand.firstRefNo == 0)
      cand.shift = scaledPeaks[cand.firstTimeNo].pos;

    for (unsigned j = 0; j < lt; j++)
      shiftedPeaks[j].pos = scaledPeaks[j].pos - cand.shift;

// cout << "shiftedPeaks " << cand.shift << "\n";
// printPeakPosCSV(shiftedPeaks, ii);
ii++;

    cand.score = Align::simpleScore(refPeaks, shiftedPeaks);
  }

  sort(candidates.begin(), candidates.end());

/*
  cout << "Shift scores\n";
  for (auto& cand: candidates)
    cout << cand.shift << ": " << cand.score << "\n";
  cout << "\n";
*/

  // Redo for the winner (remember the results...)
    double speed, accel;
    Align::estimateMotion(refPeaks, times, candidates[0].firstRefNo, 
      candidates[0].firstTimeNo, speed, accel);
// cout << "Best candidate " << candidates[0].firstRefNo << ", " <<
  // candidates[0].score << endl;
// cout << "final estimated speed " << speed << ", accel " << accel << endl;

    scaledPeaks.resize(lt);
    for (unsigned j = 0; j < lt; j++)
    {
      scaledPeaks[j].pos = speed * times[j].time +
        0.5 * accel * times[j].time * times[j].time - candidates[0].shift;
    }
}


bool Align::countTooDifferent(
  const vector<PeakTime>& times,
  const unsigned refCount) const
{
  const unsigned lt = times.size();
  return (refCount > lt + MAX_AXLE_DIFFERENCE_OK || 
      lt > refCount + MAX_AXLE_DIFFERENCE_OK);
}


void Align::bestMatches(
  const vector<PeakTime>& times,
  const Database& db,
  const string& country,
  const unsigned tops,
  const bool writeFlag,
  vector<Alignment>& matches) const
{
  timers.start(TIMER_ALIGN);

  vector<PeakPos> refPeaks, scaledPeaks;
  matches.clear();

  for (auto& refTrain: db)
  {
    const int refTrainNo = db.lookupTrainNumber(refTrain);

    if (Align::countTooDifferent(times, db.axleCount(refTrainNo)))
      continue;

    if (! db.trainIsInCountry(refTrainNo, country))
      continue;

    db.getPerfectPeaks(refTrainNo, refPeaks);

cout << "refTrain " << refTrain << endl;
    const double trainLength = refPeaks.back().pos - refPeaks.front().pos;
    Align::scalePeaks(refPeaks, times, scaledPeaks);

    // Normalize the distance score to a 200m long train.
    const double peakScale = 200. * 200. / (trainLength * trainLength);

    matches.push_back(Alignment());
    matches.back().trainNo = refTrainNo;
    Align::NeedlemanWunsch(refPeaks, scaledPeaks, peakScale, 
      matches.back());
  }

  sort(matches.begin(), matches.end());

  if (tops < matches.size())
    matches.resize(tops);

  timers.stop(TIMER_ALIGN);

  if (writeFlag)
    printMatches(db, matches);
}

