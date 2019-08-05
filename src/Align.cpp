#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

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
  // Match is good enough to go by number of cars.
  if (peaksInfo.numCars > match.numCars + MAX_CAR_DIFFERENCE_OK ||
      peaksInfo.numCars + MAX_CAR_DIFFERENCE_OK < match.numCars)
    return false;
  else if (peaksInfo.numPeaks > match.numAxles + MAX_AXLE_DIFFERENCE_OK ||
      peaksInfo.numPeaks + MAX_AXLE_DIFFERENCE_OK < match.numAxles)
    return false;
  else
    return trainDB.isInCountry(match.trainNo, sensorCountry);
}


void Align::NeedlemanWunsch(
  const vector<float>& refPeaks,
  const vector<float>& scaledPeaks,
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
  
  const unsigned lr = refPeaks.size() - alignment.numDelete;
  const unsigned lt = scaledPeaks.size() - alignment.numAdd;

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
      const float d = refPeaks[i + alignment.numDelete - 1] - 
        scaledPeaks[j + alignment.numAdd - 1];
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
        (alignment.numDelete <= 1 && 
         i > 1 && i <= 3 - alignment.numDelete &&
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
  
  // Add fixed penalties for early issues.
  // TODO Good penalty for this?  More dynamic?
  // How much dist does it lose to do (2, 0) rather than (3, 1)?
  // If it's more than "average", go with (3, 1), otherwise (2, 0).
  alignment.dist += alignment.numDelete * EARLY_SHIFTS_PENALTY +
    alignment.numAdd * EARLY_SHIFTS_PENALTY;

  unsigned i = lr;
  unsigned j = lt;
  const unsigned numAdd = alignment.numAdd;
  const unsigned numDelete = alignment.numDelete;

  while (i > 0 || j > 0)
  {
    const Origin o = matrix[i][j].origin;
    if (i > 0 && j > 0 && o == NW_MATCH)
    {
      alignment.actualToRef[j + numAdd - 1] = 
        static_cast<int>(i + numDelete - 1);
      alignment.distMatch += matrix[i][j].dist - matrix[i-1][j-1].dist;
      i--;
      j--;
    }
    else if (j > 0 && o == NW_INSERT)
    {
      alignment.actualToRef[j + numAdd - 1] = -1;
      alignment.numAdd++;
      j--;
    }
    else
    {
      alignment.numDelete++;
      i--;
    }
  }

  alignment.distOther = alignment.dist - alignment.distMatch;
}


void Align::estimateAlignedMotion(
  const vector<float>& refPeaks,
  const vector<float>& times,
  const vector<unsigned>& actualToRef,
  const int offsetRef,
  Shift& shift) const
{
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

  PolynomialRegression pol;
  pol.fitIt(x, y, 2, shift.motion.estimate);
}


bool Align::scalePeaks(
  const vector<float>& refPeaks,
  const unsigned numRefCars,
  const PeaksInfo& peaksInfo,
  Shift& shift,
  Alignment& match,
  vector<float>& scaledPeaks) const
{
  int offsetRef;
  if (peaksInfo.numCars == numRefCars)
  {
    // Assume good alignment.
    shift.firstTimeNo = 0;
    shift.firstRefNo = peaksInfo.peakNumbers[shift.firstTimeNo];
    offsetRef = 0;
  }
  else if (peaksInfo.numCars == numRefCars + 1)
  {
    // Front car is assumed spurious.
    shift.firstTimeNo = 0;
    while (shift.firstTimeNo < peaksInfo.peakNumbers.size() &&
        peaksInfo.carNumbers[shift.firstTimeNo] == 0)
      shift.firstTimeNo++;

    shift.firstRefNo = peaksInfo.peakNumbers[shift.firstTimeNo];
    offsetRef = -4;
  }
  else if (peaksInfo.numCars + 1 == numRefCars)
  {
    // Assumed missing a front car.
    shift.firstTimeNo = 0;
    shift.firstRefNo = peaksInfo.peakNumbers[shift.firstTimeNo] + 4;
    offsetRef = 4;
  }
  else
  {
    cout << "ALIGNERROR\n";
    return false;
  }

  Align::estimateAlignedMotion(refPeaks, peaksInfo.times, 
    peaksInfo.peakNumbers, offsetRef, shift);

  scaledPeaks.resize(peaksInfo.times.size());
  for (unsigned j = 0; j < peaksInfo.times.size(); j++)
    scaledPeaks[j] = shift.motion.time2pos(peaksInfo.times[j]);

  match.dist = 0.;
  match.distMatch = 0.;
  match.numAdd = shift.firstTimeNo; // Spare peaks in scaledPeaks
  match.numDelete = shift.firstRefNo; // Unused peaks in refPeaks
  match.actualToRef.resize(
    peaksInfo.times.size() + shift.firstTimeNo);
  for (unsigned k = 0; k < shift.firstTimeNo; k++)
   match.actualToRef[k] = -1;

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
    if (! Align::scalePeaks(refPeaks, match.numCars, peaksInfo, 
        shift, match, scaledPeaks))
      continue;

    // TODO Print shift.  
    if (control.verboseAlignPeaks())
      Align::printAlignPeaks(refTrain, peaksInfo.times, refPeaks, scaledPeaks);

    Align::NeedlemanWunsch(refPeaks, scaledPeaks, match);
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

