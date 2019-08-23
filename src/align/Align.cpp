// It would be possible to optimize this quite a bit, but it is already
// quite fast.  Specifically:
// 
// * In Needleman-Wunsch we could notice whether the alignment has
//   changed.  If not, there is little need to re-run the regression.
//
// * Following N-W, some regressions can probably be thrown out based on 
//   the N-W residuals.


#include <iostream>
#include <iomanip>
#include <sstream>
#include <limits>
#include <algorithm>
#include <cassert>

#include "../database/Control.h"
#include "../database/TrainDB.h"

#include "PolynomialRegression.h"
#include "Align.h"

#include "../PeakGeneral.h"
#include "../const.h"

#include "../util/misc.h"
#include "../util/io.h"


Align::Align()
{
}


Align::~Align()
{
}


bool Align::trainMightFitGeometrically(
  const PeaksInfo& peaksInfo,
  const Alignment& match) const
{
  if (peaksInfo.numCars > match.numCars + MAX_CAR_DIFFERENCE_OK ||
      peaksInfo.numCars + MAX_CAR_DIFFERENCE_OK < match.numCars)
    return false;
  else if (peaksInfo.numPeaks > match.numAxles + MAX_AXLE_DIFFERENCE_OK ||
      peaksInfo.numPeaks + MAX_AXLE_DIFFERENCE_OK < match.numAxles)
    return false;
  else
    return true;
}



bool Align::trainMightFit(
  const PeaksInfo& peaksInfo,
  const string& sensorCountry,
  const TrainDB& trainDB,
  const Alignment& match) const
{
  if (! Align::trainMightFitGeometrically(peaksInfo, match))
    return false;
  else
    return trainDB.isInCountry(match.trainNo, sensorCountry);
}


bool Align::alignFronts(
  const PeaksInfo& refInfo,
  const PeaksInfo& peaksInfo,
  Alignment& match,
  int& offsetRef) const
{
  // numAdd counts the spurious peaks in times/scaledPeaks.
  // numDelete counts the unused reference peaks.

  if (peaksInfo.numCars == refInfo.numCars)
  {
    // Assume good alignment.
    match.numAdd = 0;
    match.numDelete = peaksInfo.peakNumbers[match.numAdd];
    offsetRef = 0;
  }
  else if (peaksInfo.numCars > refInfo.numCars)
  {
    const unsigned numSpurious = peaksInfo.numCars - refInfo.numCars;
    if (numSpurious > MAX_CAR_DIFFERENCE_OK)
      return false;

    // Front cars are assumed spurious.
    match.numAdd = 0;
    while (match.numAdd < peaksInfo.peakNumbers.size() &&
        peaksInfo.carNumbers[match.numAdd] < numSpurious)
      match.numAdd++;

    // The offset will be negative.
    offsetRef = static_cast<int>(peaksInfo.peakNumbersInCar[match.numAdd]) -
      static_cast<int>(peaksInfo.peakNumbers[match.numAdd]);

    match.numDelete = peaksInfo.peakNumbersInCar[match.numAdd];
  }
  else
  {
    const unsigned numMissing = refInfo.numCars - peaksInfo.numCars;
    if (numMissing > MAX_CAR_DIFFERENCE_OK)
      return false;

    // Front cars are assumed missing from the observed trace.
    match.numAdd = 0;

    offsetRef = 0;
    while (static_cast<unsigned>(offsetRef) < refInfo.carNumbers.size() &&
        refInfo.carNumbers[offsetRef] < numMissing)
      offsetRef++;

    match.numDelete = peaksInfo.peakNumbers[match.numAdd] + offsetRef;
  }
  return true;
}


void Align::storeResiduals(
  const vector<float>& x,
  const vector<float>& y,
  const unsigned lt,
  Alignment& match) const
{
  match.distMatch = 0.f;
  for (unsigned i = 0, p = 0; i < lt; i++)
  {
    if (match.actualToRef[i] >= 0)
    {
      const float pos = match.time2pos(x[p]);
      const float res = pos - y[p];

      const unsigned refIndex = static_cast<unsigned>(match.actualToRef[i]);
      match.residuals[p].index = refIndex;
      match.residuals[p].value = res;
      match.residuals[p].valueSq = res * res;

      match.distMatch += match.residuals[p].valueSq;

      p++;
    }
  }

  match.dist = match.distMatch + match.distOther;
}


void Align::regressTrain(
  const vector<float>& times,
  const vector<float>& refPeaks,
  const bool storeFlag,
  Alignment& match) const
{
  const unsigned lt = times.size();
  const unsigned lr = refPeaks.size();

  const unsigned lcommon = lt - match.numAdd;
  vector<float> x(lcommon), y(lcommon);

  // The vectors are as compact as possible and are matched.
  for (unsigned i = 0, p = 0; i < lt; i++)
  {
    if (match.actualToRef[i] >= 0)
    {
      y[p] = refPeaks[static_cast<unsigned>(match.actualToRef[i])];
      x[p] = times[i];
      p++;
    }
  }

  // Run the regression.
  PolynomialRegression pol;
  pol.fitIt(x, y, 2, match.motion.estimate);

  if (storeFlag)
  {
    // Store the residuals.
    match.residuals.resize(lcommon);
    Align::storeResiduals(x, y, lt, match);
  }
}


bool Align::alignPeaks(
  const PeaksInfo& refInfo,
  const PeaksInfo& peaksInfo,
  Alignment& match) const
{
  // We estimate the motion parameters of fitting peaksInfo.times
  // to refPeaks, and then we calculate the corresponding times.

  int offsetRef;
  if (! Align::alignFronts(refInfo, peaksInfo, match, offsetRef))
    return false;

  if (peaksInfo.peakNumbers.back() + offsetRef >= refInfo.positions.size())
    return false; // Car number would overflow

  // Set up the alignment for a regression.
  match.dist = 0.;
  match.distMatch = 0.;

  // times is empty when we use this method to correlate theoretical
  // trains according to their positions (there are no times).
  const unsigned lt = (peaksInfo.times.empty() ?
    peaksInfo.positions.size() : peaksInfo.times.size());

  match.actualToRef.resize(lt);
  for (unsigned k = 0; k < match.numAdd; k++)
    match.actualToRef[k] = -1;
  for (unsigned k = match.numAdd; k < lt; k++)
    match.actualToRef[k] = peaksInfo.peakNumbers[k] + offsetRef;
  
  return true;
}


bool Align::scalePeaks(
  const PeaksInfo& refInfo,
  const PeaksInfo& peaksInfo,
  Alignment& match,
  vector<float>& scaledPeaks) const
{
  Align::alignPeaks(refInfo, peaksInfo, match);

  // Run a regression.
  Align::regressTrain(peaksInfo.times, refInfo.positions, false, match);

  // Use the motion parameters.
  const unsigned lt = peaksInfo.times.size();
  scaledPeaks.resize(lt);
  for (unsigned j = 0; j < lt; j++)
    scaledPeaks[j] = match.motion.time2pos(peaksInfo.times[j]);

  return true;
}


void Align::initNeedlemanWunsch(
  const unsigned lreff,
  const unsigned lteff,
  vector<vector<Mentry>>& matrix) const
{
  matrix.resize(lreff+1);
  for (unsigned i = 0; i < lreff+1; i++)
    matrix[i].resize(lteff+1);

  matrix[0][0].dist = 0.;
  for (unsigned i = 1; i < lreff+1; i++)
  {
    matrix[i][0].dist = i * DELETE_PENALTY;
    matrix[i][0].origin = NW_DELETE;
  }

  for (unsigned j = 1; j < lteff+1; j++)
  {
    matrix[0][j].dist = j * INSERT_PENALTY;
    matrix[0][j].origin = NW_INSERT;
  }
}


void Align::fillNeedlemanWunsch(
  const vector<float>& refPeaks,
  const vector<float>& scaledPeaks,
  const Alignment& match,
  const unsigned lreff,
  const unsigned lteff,
  vector<vector<Mentry>>& matrix) const
{
  // Run the dynamic programming.
  for (unsigned i = 1; i < lreff+1; i++)
  {
    for (unsigned j = 1; j < lteff+1; j++)
    {
      const float d = refPeaks[i + match.numDelete - 1] - 
        scaledPeaks[j + match.numAdd - 1];
      const float matchVal = matrix[i-1][j-1].dist + d * d;

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
        (match.numDelete <= 1 && 
         i > 1 && i <= 3 - match.numDelete &&
         j < i ? 0 : DELETE_PENALTY); // Ref

      const float ins = matrix[i][j-1].dist + INSERT_PENALTY; // Time

      if (matchVal <= del)
      {
        if (matchVal <= ins)
        {
          matrix[i][j].origin = NW_MATCH;
          matrix[i][j].dist = matchVal;
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
}


void Align::backtrackNeedlemanWunsch(
  const unsigned lreff,
  const unsigned lteff,
  const vector<vector<Mentry>>& matrix,
  Alignment& match) const
{
  match.dist = matrix[lreff][lteff].dist;
  
  // Add fixed penalties for early issues.
  match.dist += match.numDelete * EARLY_SHIFTS_PENALTY +
    match.numAdd * EARLY_SHIFTS_PENALTY;

  unsigned i = lreff;
  unsigned j = lteff;
  const unsigned numAdd = match.numAdd;
  const unsigned numDelete = match.numDelete;

  while (i > 0 || j > 0)
  {
    const Origin o = matrix[i][j].origin;
    if (i > 0 && j > 0 && o == NW_MATCH)
    {
      match.actualToRef[j + numAdd - 1] = 
        static_cast<int>(i + numDelete - 1);
      match.distMatch += matrix[i][j].dist - matrix[i-1][j-1].dist;
      i--;
      j--;
    }
    else if (j > 0 && o == NW_INSERT)
    {
      match.actualToRef[j + numAdd - 1] = -1;
      match.numAdd++;
      j--;
    }
    else
    {
      match.numDelete++;
      i--;
    }
  }

  match.distOther = match.dist - match.distMatch;
}


void Align::NeedlemanWunsch(
  const vector<float>& refPeaks,
  const vector<float>& scaledPeaks,
  Alignment& match) const
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
  // The first dimension is refPeaks, the second is the synthetic one.
  
  const unsigned lr = refPeaks.size() - match.numDelete;
  const unsigned lt = scaledPeaks.size() - match.numAdd;

  // Set up the matrix.
  vector<vector<Mentry>> matrix;
  Align::initNeedlemanWunsch(lr, lt, matrix);

  // Fill the matrix with distances and origins.
  Align::fillNeedlemanWunsch(refPeaks, scaledPeaks, match,
    lr, lt, matrix);

  // Walk back through the matrix.
  Align::backtrackNeedlemanWunsch(lr, lt, matrix, match);

  assert(refPeaks.size() + match.numAdd == 
    scaledPeaks.size() + match.numDelete);
}


bool Align::realign(
  const Control& control,
  const TrainDB& trainDB,
  const string& sensorCountry,
  const PeaksInfo& peaksInfo)
{
  vector<float> scaledPeaks;
  Alignment match;
  matches.clear();

  for (auto& refTrain: trainDB)
  {
    match.trainName = refTrain;
    match.trainNo =  static_cast<unsigned>(trainDB.lookupNumber(refTrain));
    match.numCars = trainDB.numCars(match.trainNo);
    match.numAxles = trainDB.numAxles(match.trainNo);

    if (! Align::trainMightFit(peaksInfo, sensorCountry, trainDB, match))
      continue;

    const PeaksInfo& refInfo = trainDB.getRefInfo(match.trainNo);

    if (! Align::scalePeaks(refInfo, peaksInfo, match, scaledPeaks))
      continue;

    // TODO Print shift.  
    if (control.verboseAlignPeaks())
      Align::printAlignPeaks(refTrain, peaksInfo.times, 
        refInfo.positions, scaledPeaks);

    Align::NeedlemanWunsch(refInfo.positions, scaledPeaks, match);
    matches.push_back(match);
  }

  sort(matches.begin(), matches.end());
  return (! matches.empty());
}


void Align::regress(
  const TrainDB& trainDB,
  const vector<float>& times)
{
  float bestDist = numeric_limits<float>::max();

  for (auto& ma: matches)
  {
    // Can we still beat bestAlign?
    if (ma.distOther > bestDist)
      continue;

    Align::regressTrain(times, 
      trainDB.getRefInfo(ma.trainNo).positions, true, ma);

    if (ma.dist < bestDist)
      bestDist = ma.dist;

    ma.setTopResiduals();
  }

  sort(matches.begin(), matches.end());
}


void Align::getBest(
  string& trainDetected,
  float& distDetected) const
{
  if (matches.empty())
    return;

  trainDetected = matches.front().trainName;
  distDetected = matches.front().distMatch;
}


unsigned Align::getMatchRank(const unsigned trainNoTrue) const
{
  for (unsigned i = 0; i < matches.size(); i++)
  {
    if (matches[i].trainNo == trainNoTrue)
      return i;
  }
  return matches.size();
}


void Align::updateStats() const
{
  matches.front().updateStats();
}


string Align::strMatches(const string& title) const
{
  stringstream ss;
  ss << title << "\n";
  for (auto& match: matches)
    ss << match.str();
  ss << "\n";
  return ss.str();
}


string Align::strRegress(const Control& control) const
{
  stringstream ss;

  const auto& bestAlign = matches.front();

  if (control.verboseRegressMatch())
  {
    ss << Align::strMatches("Matching alignment");

    ss << "Regression alignment\n";
    ss << bestAlign.str();
    ss << endl;
  }

  if (control.verboseRegressMotion())
    ss << bestAlign.motion.strEstimate("Regression motion");

  if (control.verboseRegressTopResiduals())
    ss << bestAlign.strTopResiduals();
  
  return ss.str();
}


string Align::strMatchingResiduals(
  const string& trainTrue,
  const string& pickAny,
  const string& heading) const
{
  stringstream ss;

  unsigned mno = 0;
  for (auto& ma: matches)
  {
    mno++;
    if (ma.trainName.find(pickAny) == string::npos)
      continue;
    if (ma.distMatch > REGRESS_GREAT_SCORE)
      continue;

    ss << "SPECTRAIN" << trainTrue << " " << ma.trainName << endl;
    ss << heading << "/" <<
      fixed << setprecision(2) << ma.distMatch << "/#" << mno << "\n";

    // The match data may have been sorted by residual size.
    // This works regardless.
    vector<float> pos(ma.numAxles);
    for (auto& re: ma.residuals)
      pos[re.index] = re.value;

    for (unsigned i = 0; i < pos.size(); i++)
    {
      if (pos[i] == 0.)
        ss << i << ";" << endl;
      else
        ss << i << ";" << fixed << setprecision(4) << pos[i] << endl;
    }
    ss << "ENDSPEC\n";
  }

  return ss.str();
}


string Align::strDeviation() const
{
  return matches.front().strDeviation();
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


Motion const * Align::getMatchingMotion(const string& trainName) const
{
  for (auto& ma: matches)
  {
    if (ma.trainName == trainName)
      return &ma.motion;
  }
  return nullptr;
}


void Align::getBoxTraces(
  const TrainDB& trainDB,
  const unsigned offset,
  const float sampleRate,
  const string& trainName,
  const vector<int>& actualToRef,
  vector<unsigned>& refTimes,
  vector<unsigned>& refPeakTypes) const
{
  Motion const * motion = Align::getMatchingMotion(trainName);
  if (motion == nullptr)
    return;

  const unsigned refNo = 
    static_cast<unsigned>(trainDB.lookupNumber(trainName));
  const auto& pi = trainDB.getRefInfo(refNo);
  const vector<float>& refPeaks = pi.points;
  const vector<int>& refCars = pi.carNumbersForPoints;
  const vector<int>& refNumbers = pi.peakNumbersForPoints;

  unsigned aNo = 0;
  unsigned n;
  int i = 0;

  while (i < static_cast<int>(refPeaks.size()))
  {
    if (! motion->pos2time(refPeaks[i], sampleRate, n))
    {
      i++;
      continue;
    }

    // Get a valid, unused actualToRef value.
    while (aNo < actualToRef.size())
    {
      if (actualToRef[aNo] == -1)
        aNo++;
      else
        break;
    }


    // Used peaks are 1-4 or 0 for boundary.
    // Missed peaks are max.
    if (refNumbers[i] == -1)
    {
      // Boundary.
      refTimes.push_back(n + offset);
      refPeakTypes.push_back(0);
      i++;
    }
    else if (aNo == actualToRef.size() ||
        actualToRef[aNo] > refNumbers[i])
    {
      // Reference peak not detected.
      refTimes.push_back(n + offset);
      refPeakTypes.push_back(numeric_limits<unsigned>::max());
      i++;
    }
    else if (actualToRef[aNo] == refNumbers[i])
    {
      // Use up the actual peak seen.
      refTimes.push_back(n + offset);
      refPeakTypes.push_back(static_cast<unsigned>(refCars[i]));
      i++;
      aNo++;
    }
    else
      aNo++;
  }
}


void Align::writeTrain(
  const TrainDB& trainDB,
  const string& filename,
  const vector<float>& posTrace,
  const unsigned offset,
  const float sampleRate,
  const string& trainName) const
{
  // Writes a synthetic time trace of the reference peaks, scaled by
  // the motion parameters of a specific match.  Useful for seeing
  // visually whether the match is systematically off.
  
  // TODO Probably superseded by writeTrainBox once that works
  Motion const * motion = Align::getMatchingMotion(trainName);
  if (motion == nullptr)
    return;

  const unsigned refNo = 
    static_cast<unsigned>(trainDB.lookupNumber(trainName));
  const vector<float>& refPeaks = trainDB.getRefInfo(refNo).positions;

  vector<float> refTimes;
  refTimes.resize(posTrace.size());
  unsigned n;

  for (unsigned i = 0; i < refPeaks.size(); i++)
  {
    if (motion->pos2time(refPeaks[i], sampleRate, n) &&
        n < posTrace.size())
      refTimes[n] = posTrace[n];
  }

  writeBinary(filename, offset, refTimes);
}


void Align::writeTrainBox(
  const Control& control,
  const TrainDB& trainDB,
  const Alignment& match,
  const string& dir,
  const string& filename,
  const unsigned offset,
  const float sampleRate) const
{
  // Write data that can be used to plot a "stick drawing" of the
  // reference train in the time domain.

  vector<unsigned> refTimes;
  vector<unsigned> refPeakTypes;
  Align::getBoxTraces(trainDB, offset, sampleRate, match.trainName,
    match.actualToRef, refTimes, refPeakTypes);

  writeBinaryUnsigned(dir + "/" + control.timesName() + "/" + filename, 
    refTimes);
  writeBinaryUnsigned(dir + "/" + control.carsName() + "/" + filename, 
    refPeakTypes);

  // Estimate duration of wheel in samples.
  // A typical wheel has a diameter of 0.9m.
  const float mid = trainDB.length(match.trainNo) / 2.f;
  unsigned w0, w1;
  match.motion.pos2time(mid, sampleRate, w0);
  match.motion.pos2time(mid + 0.9f, sampleRate, w1);
  

  stringstream ss;
  ss << 
    "TRAIN_MATCH " << match.trainName << "\n" <<
    "WHEEL_DIAMETER " << fixed << setprecision(2) << w1-w0 << "\n" <<
    "SPEED " << match.motion.strSpeed() << "\n" <<
    "ACCEL " << match.motion.strAccel()  << "\n"<<
    "DIST_MATCH " << match.distMatch << "\n";

  writeString(dir + "/" + control.infoName() + "/" + filename, ss.str());
}



void Align::writeTrainBoxes(
  const Control& control,
  const TrainDB& trainDB,
  const string& filename,
  const unsigned offset,
  const float sampleRate,
  const string& trueTrainName) const
{
  if (matches.empty())
    return;

  // Write the best match.
  const auto& matchBest = matches.front();
  Align::writeTrainBox(control, trainDB, matchBest,
    control.boxDir() + "/" + control.bestLeafDir(), filename,
    offset, sampleRate);

  if (matches.size() == 1 || matches[0].trainName == trueTrainName)
    return;

  for (unsigned i = 1; i < matches.size(); i++)
  {
    if (matches[i].trainName == trueTrainName)
    {
      // Write the "true" match.
      Align::writeTrainBox(control, trainDB, matches[i],
        control.boxDir() + "/" + control.truthLeafDir(), filename,
        offset, sampleRate);

      // Only write #2 if this was not already done.
      if (i == 1)
        return;
    }
  }

  // So now we write #2.
  Align::writeTrainBox(control, trainDB, matches[1],
    control.boxDir() + "/" + control.secondLeafDir(), filename,
    offset, sampleRate);
}

