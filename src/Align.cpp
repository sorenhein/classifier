#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <functional>
#include <algorithm>
#include <limits>

#include "PeakGeneral.h"
#include "Align.h"

#include "database/TrainDB.h"
#include "database/Control.h"

#include "align/Alignment.h"

#include "regress/PolynomialRegression.h"

#include "util/misc.h"

#include "const.h"

// Can adjust these.

#define INSERT_PENALTY 100.f
#define DELETE_PENALTY 100.f
#define EARLY_MISS_PENALTY 0.f
#define EARLY_DELETE_PENALTY 0.f
#define MAX_EARLY_MISSES 7

#define EARLY_SHIFTS_PENALTY 0.25f

#define MAX_AXLE_DIFFERENCE_OK 7
#define MAX_CAR_DIFFERENCE_OK 1

// In meters, see below.

#define PROXIMITY_PARAMETER 1.5f

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


Align::Align()
{
}


Align::~Align()
{
}


bool Align::trainMightFit(
  const PeaksInfo& peaksInfo,
  const string& sensorCountry,
  const TrainDB& trainDB,
  const Alignment& match) const
{
  if (peaksInfo.numCars > 0)
  {
    // Match is good enough to go by number of cars.
    if (peaksInfo.numCars > match.numCars + MAX_CAR_DIFFERENCE_OK ||
        peaksInfo.numCars + MAX_CAR_DIFFERENCE_OK < match.numCars)
      return false;
  }
  else
  {
    // Fall back on nmber of peaks detected.
    if (peaksInfo.numPeaks > match.numAxles + MAX_AXLE_DIFFERENCE_OK ||
        peaksInfo.numPeaks + MAX_AXLE_DIFFERENCE_OK < match.numAxles)
      return false;
  }

  return trainDB.isInCountry(match.trainNo, sensorCountry);
}


void Align::NeedlemanWunsch(
  const vector<float>& refPeaks,
  const vector<float>& scaledPeaks,
  // const float peakScale,
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
  
  const unsigned lr = refPeaks.size() - shift.firstRefNo;
  const unsigned lt = scaledPeaks.size() - shift.firstTimeNo;

  // Normalize the distance score to a 200m long train.
  const float trainLength = refPeaks.back() - refPeaks.front();
  const float peakScale = TRAIN_REF_LENGTH * TRAIN_REF_LENGTH / 
    (trainLength * trainLength);

  // Set up the matrix.
  enum Origin
  {
    NW_MATCH = 0,
    NW_DELETE = 1,
    NW_INSERT = 2
  };

  struct Mentry
  {
    float dist;
    Origin origin;
  };

  vector<vector<Mentry>> matrix;
  matrix.resize(lr+1);
  for (unsigned i = 0; i < lr+1; i++)
    matrix[i].resize(lt+1);

  matrix[0][0].dist = 0.;
  for (unsigned i = 1; i < lr+1; i++)
  {
    matrix[i][0].dist = i * DELETE_PENALTY;
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
      const float d = refPeaks[i + shift.firstRefNo - 1] - 
        scaledPeaks[j + shift.firstTimeNo - 1];
      const float match = matrix[i-1][j-1].dist + peakScale * d * d;

      // During the first few peaks we don't penalize a missed real peak
      // as heavily, as it could be due to transients etc.
      // If all real peaks are there (firstRefNo == 0), we are lenient on
      // [3, 2] (miss the third real peak),
      // [3, 1] (miss the third and then surely also the second),
      // [2, 1] (miss the second real peak).
      // If firstRefNo == 1, we are only lenient on
      // [2, 1] (miss the second real peak by number here, which is
      // really the third peak).

      const float del = matrix[i-1][j].dist + 
        (shift.firstRefNo <= 1 && 
         i > 1 && i <= 3 - shift.firstRefNo &&
         j < i ? EARLY_DELETE_PENALTY : DELETE_PENALTY); // Ref

      const float ins = matrix[i][j-1].dist + INSERT_PENALTY; // Time

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
  alignment.numAdd = shift.firstTimeNo; // Spare peaks in scaledPeaks
  alignment.numDelete = shift.firstRefNo; // Unused peaks in refPeaks
  alignment.actualToRef.resize(lt + shift.firstTimeNo);
  for (unsigned k = 0; k < shift.firstTimeNo; k++)
   alignment.actualToRef[k] = -1;
  
  unsigned i = lr;
  unsigned j = lt;
  while (i > 0 || j > 0)
  {
    const Origin o = matrix[i][j].origin;
    if (i > 0 && j > 0 && o == NW_MATCH)
    {
      alignment.actualToRef[j + shift.firstTimeNo - 1] = 
        static_cast<int>(i + shift.firstRefNo - 1);
      alignment.distMatch += matrix[i][j].dist - matrix[i-1][j-1].dist;
      i--;
      j--;
    }
    else if (j > 0 && o == NW_INSERT)
    {
      alignment.actualToRef[j + shift.firstTimeNo - 1] = -1;
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
  alignment.dist += shift.firstRefNo * EARLY_SHIFTS_PENALTY +
    shift.firstTimeNo * EARLY_SHIFTS_PENALTY;

  alignment.distOther = alignment.dist - alignment.distMatch;
}


float Align::interpolateTime(
  const vector<float>& times,
  const float index) const
{
  const unsigned lt = times.size();

  const unsigned iLeft = static_cast<unsigned>(index);
  if (iLeft+1 >= lt)
  {
    // Shouldn't really happen.
    return times.back();
  }

  const unsigned iRight = iLeft + 1;
  const float frac = index - iLeft;

  return (1.f - frac) * times[iLeft] + frac * times[iRight];
}


void Align::estimateAlignedMotion(
  const vector<float>& refPeaks,
  const vector<float>& times,
  const vector<unsigned>& actualToRef,
  const int offsetRef,
  Shift& shift) const
{
  PolynomialRegression pol;
  vector<float> x, y;

  const unsigned lt = times.size();

  // If offsetRef is negative, skip over presumed spurious first car.
  unsigned i = 0;
  while (i < lt && static_cast<int>(actualToRef[i]) + offsetRef < 0)
    i++;

  const unsigned lr = refPeaks.size();
  const unsigned rest = lt - i;
  x.resize(rest);
  y.resize(rest);

  for (unsigned p = 0; i < lt; i++, p++)
  {
    const unsigned ri = static_cast<unsigned>(actualToRef[i] + offsetRef);
    if (ri >= lr)
      break;

    y[p] = refPeaks[ri];
    x[p] = static_cast<float>(times[i]);
  }

  pol.fitIt(x, y, 2, shift.motion.estimate);
}


void Align::estimateMotion(
  const vector<float>& refPeaks,
  const vector<float>& times,
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

  const float len = refPeaks[lp-1] - refPeaks[shift.firstRefNo];
  const float posMid = refPeaks[shift.firstRefNo] + len / 2.f;

  // Look for posMid in refPeaks.
  unsigned posLeft = 0;
  while (posLeft+1 < lp && refPeaks[posLeft+1] <= posMid)
    posLeft++;

// if (flag)
// cout << "posLeft " << posLeft << endl;
  if (posLeft == 0 || posLeft == lp-1 || refPeaks[posLeft+1] <= posMid)
  {
    cout << "Should not happen:";
    cout << "posLeft " << posLeft << ", lp " << lp <<
      ", posMid " << posMid << endl;
  }
  const float propRight = (posMid - refPeaks[posLeft]) /
    (refPeaks[posLeft+1] - refPeaks[posLeft]);

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
  const float tOffset = times[shift.firstTimeNo];
  float tEnd = times.back() - tOffset;

  const float lpeff = static_cast<float>(lp) - 
    static_cast<float>(shift.firstRefNo);
  const float lteff = static_cast<float>(lt) - 
    static_cast<float>(shift.firstTimeNo) -
    shift.firstHalfNetInsert - shift.secondHalfNetInsert;

  const float tpos0 = 
    static_cast<int>(shift.firstTimeNo) + 
    shift.firstHalfNetInsert +
    static_cast<float>(posLeft - shift.firstRefNo) * 
      (lteff - 1.f) / (lpeff - 1.f);

  const float tpos1 = 
    static_cast<int>(shift.firstTimeNo) + 
    shift.firstHalfNetInsert +
    static_cast<float>(posLeft + 1 - shift.firstRefNo) * 
      (lteff - 1.f) / (lpeff - 1.f);

  const float tMid0 = Align::interpolateTime(times, tpos0);
  const float tMid1 = Align::interpolateTime(times, tpos1);

  const float tMid = (1.f-propRight) * tMid0 + propRight * tMid1 -
    tOffset;
  
  // From here on it is just the calculation of accelation (2),
  // speed (1) and offset (0).

  const float accel = len * (2.f*tMid - tEnd) /
    (tMid * tEnd * (tEnd - tMid));

  shift.motion.estimate[1] = (len - 0.5f * accel * tEnd * tEnd) / tEnd;
// if (flag)
// cout << "len " << len << " tMid " << tMid << " tEnd " << tEnd <<
  // " accel " << motion[2] << " speed " << motion[1] << endl;

  shift.motion.estimate[0] = refPeaks[shift.firstRefNo] -
    (shift.motion.estimate[1] * times[shift.firstTimeNo] +
      0.5f * accel * times[shift.firstTimeNo] * 
        times[shift.firstTimeNo]);
  
  shift.motion.estimate[2] = 0.5f * accel;

  // TODO Does this work as intended?  Later we use time2pos to
  // expand (no 0.5 factor).
}


float Align::simpleScore(
  const vector<float>& refPeaks,
  const vector<float>& shiftedPeaks) const
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
  float score = 0.;
  float d = PROXIMITY_PARAMETER;

  for (unsigned is = 0; is < ls; is++)
  {
    float dleft = shiftedPeaks[is] - refPeaks[ir];

    if (dleft < 0.)
    {
      if (ir == 0)
        d = -dleft;
      else
        cout << "Should not happen" << endl;
    }
    else
    {
      float dright = numeric_limits<float>::max();
      while (ir+1 < lr)
      {
        dright = refPeaks[ir+1] - shiftedPeaks[is];
        if (dright >= 0.)
          break;
        ir++;
      }

      dleft = shiftedPeaks[is] - refPeaks[ir];
      d = min(dleft, dright);
    }
    
    if (d <= PROXIMITY_PARAMETER)
      score += (PROXIMITY_PARAMETER - d) / PROXIMITY_PARAMETER;
  }

  return score;
}


void Align::makeShiftCandidates(
  vector<Shift>& candidates,
  const vector<OverallShift>& shifts,
  const unsigned lt,
  const unsigned lp,
  const vector<unsigned>& actualToRef,
  const int offsetRef,
  const bool fullTrainFlag) const
{
  candidates.clear();

  if (fullTrainFlag)
  {
    for (auto& o: shifts)
    {
      // const unsigned m = actualToRef[
        // static_cast<int>(o.firstTimeNo) + offsetRef];

      const unsigned m = static_cast<unsigned>(
        actualToRef[o.firstTimeNo] + offsetRef);
      if (m <= o.firstRefNo + 2 && m + 2 >= o.firstRefNo)
      {
        candidates.push_back(Shift());
        Shift& shift = candidates.back();
        shift.firstRefNo = o.firstRefNo;
        shift.firstTimeNo = o.firstTimeNo;
      }
    }
  }
  else
  {
    // TODO indent
  for (auto& o: shifts)
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

      // For trains that are presumably full, we don't need to
      // double up.
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
}


bool Align::betterSimpleScore(
  const float score, 
  const unsigned index,
  const float bestScore,
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

  const int frn = static_cast<int>(cNew.firstRefNo);
  const int ftn = static_cast<int>(cNew.firstTimeNo);
  const int dNew = frn - ftn;

  const int ofrn = static_cast<int>(cOld.firstRefNo);
  const int oftn = static_cast<int>(cOld.firstTimeNo);
  const int dOld = ofrn - oftn;

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


bool Align::scalePeaks(
  const vector<float>& refPeaks,
  const PeaksInfo& peaksInfo,
  Shift& shift,
  vector<float>& scaledPeaks) const
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

  const vector<vector<Align::OverallShift>> likelyShifts =
  {
    { {0, 0} },
    { {3, 0}, {7, 0}, {2, 0}, {6, 0}, {4, 1},         {0, 1} },
    { {2, 0}, {6, 0}, {1, 0}, {5, 0}, {3, 1}, {2, 1}, {0, 2} },
    { {1, 0}, {5, 0}, {0, 0}, {4, 0}, {2, 1}, {1, 1} },
    { {0, 0}, {4, 0} }
  };

  const unsigned lt = peaksInfo.times.size();
  const unsigned lp = refPeaks.size();

  vector<Shift> candidates;

  // Guess whether we're missing the whole first car.
  int offsetRef;

  if (peaksInfo.numCars == 0)
  {
    offsetRef = 0;
  }
  else
  {
    const unsigned aback = static_cast<unsigned>(peaksInfo.peakNumbers.back());
    if (aback + 4 <= lp)
      offsetRef = 4;
    else if (aback >= lp && aback <= lp + 4)
      offsetRef = -4;
    else
      offsetRef = 0;
  }

  if (peaksInfo.numFrontWheels <= 4)
    Align::makeShiftCandidates(candidates, likelyShifts[peaksInfo.numFrontWheels], 
      lt, lp, peaksInfo.peakNumbers, offsetRef, peaksInfo.numCars > 0);

  if (candidates.empty())
    return false;

  // We calculate a simple score for this shift.  In fact
  // we could also run Needleman-Wunsch, so this is a bit of an
  // optimization.

  vector<float> candPeaks(lt);

  unsigned bestIndex = 0;
  float bestScore = -1.;

  // scaledPeaks.resize(lt);

  for (unsigned i = 0; i < candidates.size(); i++)
  {
    Shift& cand = candidates[i];

    if (peaksInfo.numCars > 0)
      Align::estimateAlignedMotion(refPeaks, peaksInfo.times, peaksInfo.peakNumbers, 
        offsetRef, cand);
    else
      Align::estimateMotion(refPeaks, peaksInfo.times, cand);

/*
cout << "i " << i << ": " << cand.firstRefNo << ", " << cand.firstTimeNo <<
  endl;
cout << "motion " << cand.motion[0] << ", " <<
  cand.motion[1] << ", " << cand.motion[2] << endl;
  */

    for (unsigned j = 0; j < lt; j++)
      candPeaks[j] = cand.motion.time2pos(peaksInfo.times[j]);

    float score = Align::simpleScore(refPeaks, candPeaks);
// cout << "i: " << i << ", score " << score << "\n";
// if (i == 8)
// {
  // Align::printAlignPeaks("test", times, refPeaks, candPeaks);
// }
    if (Align::betterSimpleScore(score, i, bestScore, bestIndex, 
        candidates, lt))
    {
      bestIndex = i;
      bestScore = score;
      scaledPeaks = candPeaks;
    }
  }

// cout << "bestIndex: " << bestIndex << endl;
  shift = candidates[bestIndex];
  return true;
}


void Align::bestMatches(
  const Control& control,
  const TrainDB& trainDB,
  const string& sensorCountry,
  const PeaksInfo& peaksInfo,
  vector<Alignment>& matches) const
{
  vector<float> scaledPeaks;
  Alignment match;
  Shift shift;
  matches.clear();

  for (auto& refTrain: trainDB)
  {
    match.trainName = refTrain;
    match.trainNo =  static_cast<unsigned>(trainDB.lookupNumber(refTrain));
    match.numCars = trainDB.numCars(match.trainNo);
    match.numAxles = trainDB.numAxles(match.trainNo);

    if (! Align::trainMightFit(peaksInfo, sensorCountry, trainDB, match))
      continue;

    const vector<float>& refPeaks = trainDB.getPeakPositions(match.trainNo);
    if (! Align::scalePeaks(refPeaks, peaksInfo, shift, scaledPeaks))
      continue;

    // TODO Print shift.  
    if (control.verboseAlignPeaks())
      Align::printAlignPeaks(refTrain, peaksInfo.times, refPeaks, scaledPeaks);

    Align::NeedlemanWunsch(refPeaks, scaledPeaks, shift, match);
    matches.push_back(match);
  }

  sort(matches.begin(), matches.end());
}


void Align::printAlignPeaks(
  const string& refTrain,
  const vector<float>& times,
  const vector<float>& refPeaks,
  const vector<float>& scaledPeaks) const
{
  cout << "refTrain " << refTrain << "\n\n";

  printVectorCSV("times", times, 0);
  printVectorCSV("refPeaks", refPeaks, 1);
  printVectorCSV("scaledPeaks", scaledPeaks, 2);
}

