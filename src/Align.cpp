#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <functional>
#include <algorithm>
#include <limits>

#include "Align.h"
#include "Database.h"
#include "Timers.h"
#include "print.h"

// Can adjust these.

#define INSERT_PENALTY 1000.
#define DELETE_PENALTY 1000.
#define EARLY_MISS_PENALTY 10.
#define MAX_EARLY_MISSES 3

#define MAX_AXLE_DIFFERENCE_OK 4

// In meters, see below.

#define PROXIMITY_PARAMETER 1.5

extern Timers timers;


struct OverallShift
{
  unsigned firstRefNo;
  unsigned firstTimeNo;
};

const vector<OverallShift> overallShifts =
{
  {0, 0},
  {0, 1},
  {1, 0},
  {0, 2},
  {2, 0},
  {0, 3},
  {3, 0},

  {2, 1},
  {3, 1},
};


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


void Align::NeedlemanWunschNew(
  const vector<PeakPos>& refPeaks,
  const vector<PeakPos>& scaledPeaks,
  const double peakScale,
  const Shift& shift,
  Alignment& alignment) const
{
  // https://en.wikipedia.org/wiki/Needleman%E2%80%93Wunsch_algorithm
  // This is an adaptation of the sequence-matching algorithm.
  //
  // 1. The "letters" are peak positions in meters, so the metric
  //    between letters is the square of the physical distance.
  //
  // 2. There is a custom penalty function for early deletions (from
  //    the synthetic trace, scaledPeaks, i.e. an early insertion in
  //    refPeaks) and for early insertions (in scalePeaks, i.e. an
  //    early deletion in refPeaks).
  //
  // The penalty for additions and deletions could actually become
  // more integrated with the nature of the peaks:  We could measure
  // the quantity of position (or of acceleration) that it would take
  // to shape or remove a peak in a certain location.
  //
  // The first dimension is refPeaks, the second is the synthetic one.
  
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
  for (unsigned i = 1; i <= shift.firstRefNo; i++)
  {
    // No penalty here for missing peaks that we expect to miss.
    matrix[i][0].dist = 0;
    matrix[i][0].origin = NW_DELETE;
  }
  for (unsigned i = shift.firstRefNo+1; i < lr+1; i++)
  {
    matrix[i][0].dist = (i - shift.firstRefNo) * DELETE_PENALTY;
    matrix[i][0].origin = NW_DELETE;
  }

  for (unsigned j = 1; j <= shift.firstTimeNo; j++)
  {
    matrix[0][j].dist = 0;
    matrix[0][j].origin = NW_INSERT;
  }

  for (unsigned j = shift.firstTimeNo+1; j < lt+1; j++)
  {
    matrix[0][j].dist = (j - shift.firstTimeNo) * INSERT_PENALTY;
    matrix[0][j].origin = NW_INSERT;
  }

  // Run the dynamic programming.
  for (unsigned i = 1; i < lr+1; i++)
  {
    for (unsigned j = 1; j < lt+1; j++)
    {
      const double d = refPeaks[i-1].pos - scaledPeaks[j-1].pos;
      const double match = matrix[i-1][j-1].dist + peakScale * d * d;
      const double del = matrix[i-1][j].dist + 
        (i <= shift.firstRefNo ? 0 : DELETE_PENALTY);
      const double ins = matrix[i][j-1].dist + 
        (j <= shift.firstTimeNo ? 0 : INSERT_PENALTY);

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

  // Add fixed penalties for early issues.
  // TODO Good penalty for this?  More dynamic?
  // How much dist does it lose to do (2, 0) rather than (3, 1)?
  // If it's more than "average", go with (3, 1), otherwise (2, 0).
  alignment.dist += shift.firstRefNo * 1 +
    shift.firstTimeNo * 1;
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
  Shift& shift) const
{
  /*
     The basic idea is to estimate speed and acceleration.  This has
     become somewhat more elaborate as it gained in importance.

     len = v0 * t_end + 0.5 * a * t_end^2
     len/2 = v_0 * t_mid + 0.5 * a * t_mid^2

     len * t_mid = v0 * t_end * tmid + 0.5 * a * t_end^2 * t_mid
     len * t_end/2 = v0 * t_mid * t_end + 0.5 * a * t_mid^2 * t_end

     len * (t_mid - t_end/2) = 0.5 * a * t_mid * t_end * (t_end - t_mid)

     a = len * (2 * t_mid - t_end) / [t_mid * t_end * (t_end - t_mid)]
  */

  const unsigned lp = refPeaks.size();

  const double len = refPeaks[lp-1].pos - refPeaks[shift.firstRefNo].pos;
  const double posMid = refPeaks[shift.firstRefNo].pos + len / 2.;

  // Look for posMid in refPeaks.
  unsigned posLeft = 0;
  while (posLeft+1 < lp && refPeaks[posLeft+1].pos <= posMid)
    posLeft++;

// if (flag)
// cout << "posLeft " << posLeft << endl;
  if (posLeft == 0 || posLeft == lp-1 || refPeaks[posLeft+1].pos <= posMid)
  {
    cout << "Should not happen:";
    cout << "posLeft " << posLeft << ", lp " << lp <<
      ", posMid " << posMid << endl;
  }
  const double propRight = (posMid - refPeaks[posLeft].pos) /
    (refPeaks[posLeft+1].pos - refPeaks[posLeft].pos);

// if (flag)
// cout << "posMid " << posMid << " propRight " << propRight << endl;

  // Down to here nothing depends on the vagaries of measured times.
  // The tricky part is to find tMid, the time at which the actual
  // train reaches posMid.  This happens at posLeft plus a fraction
  // (propRight) of the rightmost part of the next refPeaks interval.
  // 
  // Assuming the peaks line up from firstRefNo and firstTimeNo onwards, 
  // it's an interpolation.  But there could also be insertions and 
  // deletions either in the first or second half of the times list.
  //
  // If there is 1 extra time peak in the first half of times,
  // then we should pretend that lt is 1 lower when we calculate
  // the interpolation fraction in tpos0 and tpos1 (the times 
  // corresponding to posLeft and posLeft+1), and then add 1.
  //
  // If the extra time peak is in the second half, we still pretend
  // that lt is 1 lower, but we don't add 1.

  const unsigned lt = times.size();
  const double tOffset = times[shift.firstTimeNo].time;
  double tEnd = times.back().time - tOffset;

  const double lpeff = lp - shift.firstRefNo;
  const double lteff = lt - shift.firstTimeNo -
    shift.firstHalfNetInsert - shift.secondHalfNetInsert;

  const double tpos0 = 
    shift.firstTimeNo + 
    shift.firstHalfNetInsert +
    static_cast<double>(posLeft - shift.firstRefNo) * 
      (lteff - 1.) / (lpeff - 1.);

  const double tpos1 = 
    shift.firstTimeNo + 
    shift.firstHalfNetInsert +
    static_cast<double>(posLeft + 1 - shift.firstRefNo) * 
      (lteff - 1.) / (lpeff - 1.);

  const double tMid0 = Align::interpolateTime(times, tpos0);
  const double tMid1 = Align::interpolateTime(times, tpos1);

  const double tMid = (1.-propRight) * tMid0 + propRight * tMid1 -
    tOffset;
  
  // From here on it is just the calculation of accelation (2),
  // speed (1) and offset (0).

  shift.motion[2] = len * (2.*tMid - tEnd) /
    (tMid * tEnd * (tEnd - tMid));

  shift.motion[1] = (len - 0.5 * shift.motion[2] * tEnd * tEnd) / tEnd;
// if (flag)
// cout << "len " << len << " tMid " << tMid << " tEnd " << tEnd <<
  // " accel " << motion[2] << " speed " << motion[1] << endl;

  shift.motion[0] = refPeaks[shift.firstRefNo].pos -
    (shift.motion[1] * times[shift.firstTimeNo].time +
      0.5 * shift.motion[2] * times[shift.firstTimeNo].time * 
        times[shift.firstTimeNo].time);
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


void Align::makeShiftCandidates(
  vector<Shift>& candidates,
  const unsigned lt,
  const unsigned lp) const
{
  candidates.clear();

  for (auto& o: overallShifts)
  {

    // Let's say firstRefNo = 2, firstTimeNo = 1, lp = 56, lt = 54.
    // So we'll be aligning 54 ref peaks with 53 times.  That means
    // we missed one of the time peaks (or maybe we missed 2 and added
    // one spurious peak, but we won't look too hard for that).
    // We could have missed it either in the first half or in the 
    // second half of the times list.
    
    const unsigned effRef = lp - o.firstRefNo;
    const unsigned effTimes = lt - o.firstTimeNo;

    if (effRef == effTimes + 1)
    {
      candidates.push_back(Shift());
      Shift& shift1 = candidates.back();
      shift1.firstRefNo = o.firstRefNo;
      shift1.firstTimeNo = o.firstTimeNo;
      shift1.firstHalfNetInsert = 1;
      shift1.secondHalfNetInsert = 0;

      candidates.push_back(Shift());
      Shift& shift2 = candidates.back();
      shift2.firstRefNo = o.firstRefNo;
      shift2.firstTimeNo = o.firstTimeNo;
      shift2.firstHalfNetInsert = 0;
      shift2.secondHalfNetInsert = 1;
    }
    else if (effRef + 1 == effTimes)
    {
      candidates.push_back(Shift());
      Shift& shift1 = candidates.back();
      shift1.firstRefNo = o.firstRefNo;
      shift1.firstTimeNo = o.firstTimeNo;
      shift1.firstHalfNetInsert = -1;
      shift1.secondHalfNetInsert = 0;

      candidates.push_back(Shift());
      Shift& shift2 = candidates.back();
      shift2.firstRefNo = o.firstRefNo;
      shift2.firstTimeNo = o.firstTimeNo;
      shift2.firstHalfNetInsert = 0;
      shift2.secondHalfNetInsert = -1;
    }
    else
    {
      // This will only work out well if effRef == effTimes.
      candidates.push_back(Shift());
      Shift& shift = candidates.back();
      shift.firstRefNo = o.firstRefNo;
      shift.firstTimeNo = o.firstTimeNo;
      shift.firstHalfNetInsert = 0;
      shift.secondHalfNetInsert = 0;
    }
  }
}


bool Align::betterSimpleScore(
  const double score, 
  const unsigned index,
  const double bestScore,
  const unsigned bestIndex,
  const vector<Shift>& candidates,
  const unsigned lt) const
{
  if (bestScore < 0.)
    return true;

  // We take a bit of care when comparing e.g. (3, 1) to (2, 0).
  // (3, 1) leaves off two more peaks, and this may improve its simple
  // score even though the two are quite close and (2, 0) "should" win.
  // It would probably be better to let both of them through to the
  // Needleman-Wunsch algorithm, but assuming we only let one winner per
  // train type through, let us compare them more closely.

  const Shift& cNew = candidates[index];
  const Shift& cOld = candidates[bestIndex];

  const int dNew = (cNew.firstRefNo >= cNew.firstTimeNo ?
    cNew.firstRefNo - cNew.firstTimeNo :
    -static_cast<int>(cNew.firstTimeNo - cNew.firstRefNo));
  const int dOld = (cOld.firstRefNo >= cOld.firstTimeNo ?
    cOld.firstRefNo - cOld.firstTimeNo :
    -static_cast<int>(cOld.firstTimeNo - cOld.firstRefNo));

  if (dNew != dOld)
    return (score > bestScore);

  // So now we have a commensurate pair.
  bool newIsGood = (score >= 0.9 * (lt - cNew.firstTimeNo));
  bool oldIsGood = (bestScore >= 0.9 * (lt - cOld.firstTimeNo));

  if (! newIsGood || ! oldIsGood)
  {
    // It shouldn't really be possible for one with a low score
    // to be good etc., but we don't test for all this.
    return (score > bestScore);
  }
  else if (cNew.firstRefNo < cOld.firstRefNo && score >= bestScore)
  {
    // Wins on both counts.
    return true;
  }
  else if (cNew.firstRefNo > cOld.firstRefNo && score <= bestScore)
  {
    // Loses on both counts.
    return false;
  }

  // So now we have two great candidates.  One has more deletions, 
  // but also a better score.  We'll go with the one with fewer
  // deletions anyway.

  if (cNew.firstRefNo < cOld.firstRefNo)
    return true;
  else
    return false;
}


void Align::scalePeaks(
  const vector<PeakPos>& refPeaks,
  const vector<PeakTime>& times,
  Shift& shift,
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

  // TODO Make this more general:  Also net adds in 1st/2nd halves.
  // Impacts estimateMotion

  const unsigned lt = times.size();
  const unsigned lp = refPeaks.size();

  vector<Shift> candidates;
  Align::makeShiftCandidates(candidates, lt, lp);

  // We calculate a simple score for this shift.  In fact
  // we could also run Needleman-Wunsch, so this is a bit of an
  // optimization.

  vector<PeakPos> candPeaks(lt);

  unsigned bestIndex = 0;
  double bestScore = -1.;

  // scaledPeaks.resize(lt);

  for (unsigned i = 0; i < candidates.size(); i++)
  {
    Shift& cand = candidates[i];
    cand.motion.resize(3);

    Align::estimateMotion(refPeaks, times, cand);

    for (unsigned j = 0; j < lt; j++)
    {
      candPeaks[j].pos = cand.motion[0] +
        cand.motion[1] * times[j].time +
        0.5 * cand.motion[2] * times[j].time * times[j].time;
    }

    double score = Align::simpleScore(refPeaks, candPeaks);
// cout << "i: " << i << ", score " << score << "\n";
// if (i == 8)
// {
  // Align::printAlignPeaks("test", times, refPeaks, candPeaks);
// }
    if (Align::betterSimpleScore(score, i, bestScore, bestIndex, 
        candidates, lt))
    // if (score > bestScore)
    {
      bestIndex = i;
      bestScore = score;
      scaledPeaks = candPeaks;
    }
  }

// cout << "bestIndex: " << bestIndex << endl;
  shift = candidates[bestIndex];
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
  const Control& control,
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

    const double trainLength = refPeaks.back().pos - refPeaks.front().pos;
    Shift shift;
    Align::scalePeaks(refPeaks, times, shift, scaledPeaks);


    if (control.verboseAlignPeaks)
    {
      Align::printAlignPeaks(refTrain, times, refPeaks, scaledPeaks);
      // TODO Print shift.  
    }

    // Normalize the distance score to a 200m long train.
    const double peakScale = 200. * 200. / (trainLength * trainLength);

    matches.push_back(Alignment());
    matches.back().trainNo = refTrainNo;
    // Align::NeedlemanWunsch(refPeaks, scaledPeaks, peakScale, 
      // matches.back());
    Align::NeedlemanWunschNew(refPeaks, scaledPeaks, peakScale, 
      shift, matches.back());
// for (unsigned i = 0; i < matches.back().actualToRef.size(); i++)
// {
  // cout << "i " << i << " " << matches.back().actualToRef[i] << endl;
// }
  }

  sort(matches.begin(), matches.end());

  if (tops < matches.size())
    matches.resize(tops);

  timers.stop(TIMER_ALIGN);

  if (control.verboseAlignMatches)
    printMatches(db, matches);
}


void Align::printAlignPeaks(
  const string& refTrain,
  const vector<PeakTime>& times,
  const vector<PeakPos>& refPeaks,
  const vector<PeakPos>& scaledPeaks) const
{
  cout << "refTrain " << refTrain << "\n\n";

  cout << "times\n";
  printPeakTimeCSV(times, 0);

  cout << "refPeaks\n";
  printPeakPosCSV(refPeaks, 1);

  cout << "scaledPeaks " << "\n";
  printPeakPosCSV(scaledPeaks, 2);
}

