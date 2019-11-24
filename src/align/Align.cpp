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


// TODO Could go in Alignment
bool Align::promisingPartial(
  const vector<int>& actualToRef,
  const bool allowAddFlag) const
{
  unsigned numAdd = 0;
  unsigned numEvenParityErrors = 0;
  unsigned numOddParityErrors = 0;

  for (unsigned i = 0; i < actualToRef.size(); i++)
  {
    if (actualToRef[i] == -1)
    {
      numAdd++;
      continue;
    }

    if (i % 2 == 0)
    {
      if (actualToRef[i] % 2)
        numEvenParityErrors++;
    }
    else if (actualToRef[i] % 2 == 0)
      numOddParityErrors++;
  }

  if (numAdd || numEvenParityErrors || numOddParityErrors)
    cout << "align check " << numAdd << ", " <<
      numEvenParityErrors << ", " << numOddParityErrors << "\n";

  return ((allowAddFlag || numAdd == 0) && 
      numEvenParityErrors == 0 && 
      numOddParityErrors == 0);
}


bool Align::plausibleCounts(const Alignment& match) const
{
  // There should not be many unmatched bogies in the reference signal.
  if (match.numAdd > 2)
  {
    cout << match.numDelete << " deletions, " <<
      match.numAdd << " additions\n";

cout << "align:\n";
for (unsigned i = 0; i < match.actualToRef.size(); i++)
  if (match.actualToRef[i] != -1)
    cout << i << ": " << match.actualToRef[i] << "\n";

    return false;
  }

  unsigned firstRefMatch = numeric_limits<unsigned>::max();
  for (unsigned i = 0; i < match.actualToRef.size(); i++)
  {
    if (match.actualToRef[i] != -1)
    {
      firstRefMatch = match.actualToRef[i];
      break;
    }
  }

  // There should not be many leading reference peaks that are missing.
  if (firstRefMatch > 8)
  {
    cout << firstRefMatch << " is first match\n";

cout << "align:\n";
for (unsigned i = 0; i < match.actualToRef.size(); i++)
  if (match.actualToRef[i] != -1)
    cout << i << ": " << match.actualToRef[i] << "\n";

    return false;
  }

  return true;
}


unsigned Align::getGoodCount(const vector<int>& actualToRef) const
{
  for (unsigned i = 0; i < actualToRef.size(); i++)
  {
    const int j = actualToRef.size() - i - 1;
    if (actualToRef[j] == -1 ||
        (actualToRef[j]) % 2 != (j % 2))
    {
      return i;
    }
  }
  return actualToRef.size();
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
    const int numSpurious = 
      static_cast<int>(peaksInfo.numCars - refInfo.numCars);
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
    const int numMissing = 
      static_cast<int>(refInfo.numCars - peaksInfo.numCars);
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

  if (peaksInfo.peakNumbers.back() + offsetRef >= 
      static_cast<int>(refInfo.positions.size()))
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
      match.residuals[p].refIndex = refIndex;
      match.residuals[p].actualIndex = i;
      match.residuals[p].value = res;
      match.residuals[p].valueSq = res * res;

      match.distMatch += match.residuals[p].valueSq;

      p++;
    }
  }

  match.dist = match.distMatch + match.distOther;
}


void Align::regressPosLinear(
  const vector<float>& x,
  const vector<float>& y,
  vector<float>& estimate) const
{
  // Don't use estimate directly, as size would be 2.
  vector<float> est;
  PolynomialRegression pol;
  pol.fitIt(x, y, 1, est);
  estimate[0] = est[0];
  estimate[1] = est[1];
}


void Align::regressPosQuadratic(
  const vector<float>& x,
  const vector<float>& y,
  const unsigned lcommon,
  vector<float>& estimate) const
{
  PolynomialRegression pol;
  pol.fitIt(x, y, 2, estimate);

  // If the acceleration is higher than physically expected, clamp it
  // and go to a linear regression.  Note that coeff2 = accel /2.
  const float coeff2 = estimate[2];
  if (2.f * abs(coeff2) >= ACCEL_MAX)
  {
    vector<float> z(lcommon);
    const float clamp = (coeff2 > 0.f ? ACCEL_MAX : -ACCEL_MAX) / 2.f;
    estimate[2] = clamp;

    for (unsigned p = 0; p < lcommon; p++)
      z[p] = y[p] - clamp * x[p] * x[p];

    // Don't use match.motion.estimate directly, as size would be 2.
    Align::regressPosLinear(x, z, estimate);
  }
}


void Align::regressTrain(
  const vector<float>& times,
  const vector<float>& refPeaks,
  const bool accelFlag,
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
  if (accelFlag)
    Align::regressPosQuadratic(x, y, lcommon, match.motion.estimate);
  else
    Align::regressPosLinear(x, y, match.motion.estimate);

  if (storeFlag)
  {
    // Store the residuals.
    match.residuals.resize(lcommon);
    Align::storeResiduals(x, y, lt, match);
  }
}


bool Align::scalePeaks(
  const PeaksInfo& refInfo,
  const PeaksInfo& peaksInfo,
  Alignment& match,
  vector<float>& scaledPeaks) const
{
  Align::alignPeaks(refInfo, peaksInfo, match);

  // Run a regression.
  Align::regressTrain(peaksInfo.times, refInfo.positions, true, false, match);

  // Use the motion parameters.
  const unsigned lt = peaksInfo.times.size();
  scaledPeaks.resize(lt);
  for (unsigned j = 0; j < lt; j++)
    scaledPeaks[j] = match.motion.time2pos(peaksInfo.times[j]);

  return true;
}


void Align::distributeBogies(
  const list<BogieTimes>& bogieTimes,
  const list<BogieTimes>::const_iterator& bitStart,
  const float posOffset,
  const float speed,
  vector<float>& scaledPeaks) const
{
  scaledPeaks.clear();
  for (auto bit = bitStart; bit != bogieTimes.end(); bit++)
  {
    scaledPeaks.push_back(posOffset + speed * bit->first);
    scaledPeaks.push_back(posOffset + speed * bit->second);
  }
}


#define LAST_DIST_LIMIT 0.5

bool Align::bootstrapMotion(
  const PeaksInfo& refInfo,
  const list<BogieTimes>& bogieTimes,
  Alignment& match) const
{
  // Just the last bogie.
  if (bogieTimes.empty())
    return false;

  const unsigned lr = refInfo.positions.size();
  const BogieTimes& lastBogie = bogieTimes.back();

  // Align the last two wheels.
  match.motion.estimate[1] = 
    2000.f * (refInfo.positions[lr-1] - refInfo.positions[lr-2]) /
    (lastBogie.second - lastBogie.first);

  match.motion.estimate[0] = 
    refInfo.positions[lr-1] - 
      match.motion.estimate[1] * lastBogie.second / 2000.f;

cout << "bootstrap motion " << setw(15) << "" <<
  match.motion.estimate[0] << " m, " <<
  match.motion.estimate[1] << " m/s" << endl;

  return true;
}

bool Align::scaleLastBogies(
  const PeaksInfo& refInfo,
  const list<BogieTimes>& bogieTimes,
  const unsigned numBogies,
  vector<float>& scaledPeaks) const
{
  if (bogieTimes.size() < numBogies)
    return false;

  if (refInfo.positions.size() < 2 * numBogies)
    return false;

  const unsigned lr = refInfo.positions.size();
  const BogieTimes& lastBogie = bogieTimes.back();

  // Align the last two wheels.
  float lastSpeed = (refInfo.positions[lr-1] - refInfo.positions[lr-2]) /
    (lastBogie.second - lastBogie.first);
  float sOffset = refInfo.positions[lr-1] - lastSpeed * lastBogie.second;

  // Only do the last several bogies.
  auto bt = bogieTimes.end();
  for (unsigned i = 0; i < numBogies; i++)
    bt = prev(bt);

// cout << "Partial motion parameters:\n";
// cout << sOffset << ", " << lastSpeed << "\n\n";

  Align::distributeBogies(bogieTimes, bt, sOffset, lastSpeed, scaledPeaks);

/*
  for (auto f: scaledPeaks)
    cout << "Bogie detections: " << f << endl;
  cout << "\n";

  for (auto f: refInfo.positions)
    cout << "References: " << f << endl;
  cout << "\n";
*/

  return true;
}


bool Align::realign(
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

    // Run Needleman-Wunsch matching.
    dynprog.run(refInfo.positions, peaksInfo.penaltyFactor, scaledPeaks, 
      fullPenalties, false, match);
    matches.push_back(match);
  }

  sort(matches.begin(), matches.end());

  return (! matches.empty());
}


void Align::getPartialMatch(
  const vector<float>& times,
  const vector<float>& scaledPeaks,
  const unsigned scaledBackCount,
  const vector<float>& refPositions,
  Alignment& match) const
{
  // Takes the last scaledBackCount peaks and regresses them against the
  // positions with which they are aligned.  match.actualToRef is adjusted
  // (-1 on early entries) if needed.
  
  if (scaledBackCount < scaledPeaks.size())
  {
    for (unsigned i = 0; i < scaledPeaks.size() - scaledBackCount; i++)
      match.actualToRef[i] = -1;
  }

  vector<float> backTimes;
  for (unsigned i = 0; i < scaledPeaks.size(); i++)
    backTimes.push_back(times[times.size() - scaledPeaks.size() + i]);

  Align::regressTrain(backTimes, refPositions, false, true, match);

cout << "motion parameters for " << scaledBackCount << " peaks: " <<
  match.motion.estimate[0] << " m, " << match.motion.estimate[1] << 
  " m/s\n\n";
}


#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))

void Align::setupDynRun(
  const unsigned refSize,
  const unsigned seenSize,
  vector<float>& penaltyFactor,
  Alignment& match) const
{
  UNUSED(refSize);

  penaltyFactor.resize(seenSize, 1.);
  match.numAdd = 0;
  match.numDelete = 0;

  match.actualToRef.resize(seenSize);

  match.dist = 0.f;
  match.distOther = 0.f;
  match.distMatch = 0.f;
}


void Align::printMatch(
  const Alignment& match,
  const unsigned len,
  const string& text) const
{
  cout << text << "\n";
  cout << match.str() << "\n";
  for (unsigned i = 0; i < len; i++)
    cout << "i " << i << ": " << match.actualToRef[i] << endl;
}


void Align::printVector(
  const vector<float>& v,
  const string& text) const
{
  cout << text << "\n";
  for (unsigned i = 0; i < v.size(); i++)
    cout << i << " " << v[i] << "\n";
  cout << "\n";
}


void Align::stopAtLargeJump(Alignment& match)
{
  for (unsigned i = 1; i < match.actualToRef.size(); i++)
  {
    unsigned j = match.actualToRef.size() - 1 - i;
    // TODO Should be j
    if (match.actualToRef[j] == -1)
      break;

    if (abs(match.residuals[j].value - match.residuals[j+1].value) > 3.f &&
        match.residuals[j].value * match.residuals[j+1].value < 0.f)
    {
      // Opposite signs, large jump.
cout << "Found large jump at " << j << endl;
      for (unsigned k = 0; k <= j; k++)
        match.actualToRef[k] = -1;
      break;
    }
  }
}


unsigned Align::cutoff(Alignment& match)
{
  // Returns the number of plausible matches from the back.
  bool cutFlag = false;
  unsigned cutPos = numeric_limits<unsigned>::max();
  for (unsigned i = 1; i < match.actualToRef.size(); i++)
  {
    unsigned j = match.actualToRef.size() - 1 - i;
    if (match.actualToRef[j] == -1)
    {
      cutFlag = true;
      cutPos = j;
      break;
    }

    if ((j & 1) && match.actualToRef[j-1]+1 != match.actualToRef[j])
    {
      // Dynamic programming aligns across bogies.
cout << "Found in-bogie jump at " << j << endl;
      cutFlag = true;
      cutPos = j;
      break;
    }

    const float rleft = match.residualsActual[j].value;
    const float rright = match.residualsActual[j+1].value;

    if (abs(rleft - rright) >5.f ||
        (abs(rleft - rright) > 3.f && rleft * rright < 0.f))
    {
      // Opposite signs, large jump.
cout << "Found large cutoff at " << j << endl;
      cutFlag = true;
      cutPos = j;
      break;
    }
  }

  if (cutFlag)
  {
    for (unsigned i = 0; i <= cutPos; i++)
      match.actualToRef[i] = -1;
  }

  if (cutPos < match.actualToRef.size())
    return match.actualToRef.size() - 1 - cutPos;
  else
    return match.actualToRef.size();
}


void Align::alignIteration(
  const vector<float>& refPositions,
  const DynamicPenalties& penalties,
  const vector<float>& timesSeen,
  const list<BogieTimes>& bogieTimes,
  const unsigned scaledCount,
  const string& text,
  vector<float>& penaltyFactor,
  vector<float>& scaledPeaks,
  Alignment& match)
{
  // Regress linearly on the scaledPeaks.
  Align::getPartialMatch(timesSeen, scaledPeaks, scaledCount,
    refPositions, match);

  // Distribute all the bogies with these motion parameters.
  // Kludge: Make up my mind, probably scale bogieTimes already.
  // Don't scale back and forth.
  Align::distributeBogies(bogieTimes, bogieTimes.cbegin(),
    match.motion.estimate[0], match.motion.estimate[1] / 2000.f, scaledPeaks);

  // Run the regular Needleman-Wunsch matching.
  Align::setupDynRun(refPositions.size(), scaledPeaks.size(),
    penaltyFactor, match);

  dynprog.run(refPositions, penaltyFactor, scaledPeaks, penalties, 
    true, match); 

  Align::stopAtLargeJump(match);

  Align::printVector(scaledPeaks, text);
  cout << text << ":\n" << match.str() << "\n";
}


bool Align::iterate(
  const vector<float>& refPositions,
  const DynamicPenalties& penalties,
  const vector<float>& timesSeen,
  const list<BogieTimes>& bogieTimes,
  Alignment& match,
  unsigned& numPlausible)
{
  // The starting point is a new set of motion parameters.
  // Returns true if there is no change in alignment.

  // Distribute all the bogies with these motion parameters.
  // Kludge: Make up my mind, probably scale bogieTimes already.
  // Don't scale back and forth.
  vector<float> scaledPeaks;
  Align::distributeBogies(bogieTimes, bogieTimes.cbegin(),
    match.motion.estimate[0], match.motion.estimate[1] / 2000.f, scaledPeaks);

  // Run the regular Needleman-Wunsch matching.
  vector<float> penaltyFactor;
  Align::setupDynRun(refPositions.size(), scaledPeaks.size(),
    penaltyFactor, match);

  dynprog.run(refPositions, penaltyFactor, scaledPeaks, penalties, 
    true, match); 

  if (! match.dynChangeFlag)
    return true;

  // Regress linearly on the scaledPeaks.
  // TODO Figure out count, maybe set add/delete in match
  match.motion.estimate[2] = 0.f;
  Align::getPartialMatch(timesSeen, scaledPeaks, scaledPeaks.size(),
    refPositions, match);

  // Uncompactify the residuals for easier reference.
  match.residualsActual.resize(scaledPeaks.size());
  unsigned p = 0;
  for (unsigned i = 0; i < scaledPeaks.size(); i++)
  {
    if (match.actualToRef[i] != -1)
    {
      match.residualsActual[i] = match.residuals[p];
      p++;
    }
  }

cout << setw(6) << "" << setw(8) << "ref" <<
  setw(8) << "resid" << 
  setw(8) << "time" << 
  setw(8) << "scaled" <<
  setw(6) << "a2r" << "\n";
for (unsigned i = 0; i < refPositions.size(); i++)
{
  cout << setw(6) << left << i <<
    setw(8) << right << fixed << setprecision(2) << refPositions[i];
  if (i >= scaledPeaks.size())
    cout << "\n";
  else
  {
    if (match.actualToRef[i] == -1 &&
        abs(match.residualsActual[i].value) > 10000.)
      cout << setw(8) << "-";
    else
      cout << setw(8) << match.residualsActual[i].value;

    cout <<
      setw(8) << timesSeen[i] << 
      setw(8) << scaledPeaks[i] << 
      setw(6) << match.actualToRef[i] << endl;
  }
}
cout << "\n";

  // Cut off when we hit -1 or a large jump suggesting a skipped bogie.
  numPlausible = Align::cutoff(match);
cout << "numPlausible: " << numPlausible << endl;
  if (numPlausible <= 2)
    return true;
  
  // Rerun regression.
  // TODO Figure out count, maybe set add/delete in match
  match.numAdd = scaledPeaks.size() - numPlausible;

  Align::getPartialMatch(timesSeen, scaledPeaks, numPlausible,
    refPositions, match);
  return false;
}


bool Align::realign(
  const TrainDB& trainDB,
  const string& sensorCountry,
  const list<BogieTimes>& bogieTimes)
{
  PeaksInfo peaksInfo;
  peaksInfo.numCars = (bogieTimes.size() / 2) + 1;
  peaksInfo.numPeaks = 2 * bogieTimes.size();

  vector<float> times;
  for (auto& b: bogieTimes)
  {
    // TODO Use sampleRate
    times.push_back(b.first / 2000.f);
    times.push_back(b.second / 2000.f);
  }

cout << "Have " << times.size() << " bogie times\n";
if (times.empty())
  return false;

  vector<float> scaledPeaks;
  vector<float> penaltyFactor;
  Alignment match;
  matches.clear();

  for (auto& refTrain: trainDB)
  {
    match.trainName = refTrain;
    match.trainNo =  static_cast<unsigned>(trainDB.lookupNumber(refTrain));
    match.numCars = trainDB.numCars(match.trainNo);
    match.numAxles = trainDB.numAxles(match.trainNo);

    // trainMightFit is too much, but probably shouldn't have a lot more
    // bogie peaks than reference peaks...

    if (! trainDB.isInCountry(match.trainNo, sensorCountry))
      continue;

    // Shouldn't be many spurious bogies.
    if (match.numAxles + 2 < peaksInfo.numPeaks)
    {
      cout << "FLAGGING " << match.numAxles << ", " << peaksInfo.numPeaks << endl;
      // continue;
    }

    const PeaksInfo& refInfo = trainDB.getRefInfo(match.trainNo);

cout << "Trying train " << refTrain << endl;
if (refTrain == "ICET_DEU_56_N")
  cout << "HERE\n";
    
/* */
    if (! Align::bootstrapMotion(refInfo, bogieTimes, match))
      continue;

    unsigned iter = 0;
    bool doneFlag = false;
    unsigned numPlausible = numeric_limits<unsigned>::max();
    do
    {
      if (Align::iterate(refInfo.positions, partialPenalties,
          times, bogieTimes, match, numPlausible))
      {
        cout << "done " << iter << ":\n" << match.str() << "\n";
        doneFlag = true;
        break;
      }
      else
      {
        cout << "iter " << iter << ":\n" << match.str() << "\n";
        iter++;
      }
    }
    while (iter < 3);

    if (! doneFlag || numPlausible <= 2) 
    {
cout << "Match failed\n\n";
      continue;
    }
/* */



/*
    if (! Align::scaleLastBogies(refInfo, bogieTimes, 3, scaledPeaks))
      continue;

    // Run partial Needleman-Wunsch matching.
    Align::setupDynRun(refInfo.positions.size(), scaledPeaks.size(),
      penaltyFactor, match);

    dynprog.run(refInfo.positions, penaltyFactor, scaledPeaks, 
      partialPenalties, true, match);

    if (! Align::promisingPartial(match.actualToRef, false))
      continue;

Align::printVector(refInfo.positions, "Reference");

Align::printMatch(match, scaledPeaks.size(), "Partial match");
    
    Align::alignIteration(refInfo.positions, partialPenalties,
      times, bogieTimes, scaledPeaks.size(), "Full linear match", penaltyFactor,
      scaledPeaks, match);

    if (! Align::promisingPartial(match.actualToRef, true))
    {
      cout << "Failed the plausible match test\n";

Align::printMatch(match, scaledPeaks.size(), "Supposedly full match");

      // Regress linearly on the as many scaledPeaks as reasonable.
      const unsigned goodScales = Align::getGoodCount(match.actualToRef);

      Align::alignIteration(refInfo.positions, partialPenalties,
        times, bogieTimes, goodScales, "Final linear match", penaltyFactor,
        scaledPeaks, match);

Align::printMatch(match, scaledPeaks.size(), "Final linear match");

      if (! Align::promisingPartial(match.actualToRef, true))
      {
        cout << "Failed the plausible match test\n";
        continue;
      }
    }

    if (! Align::plausibleCounts(match))
    {
      cout << "Implausible match counts\n";
      continue;
    }
*/

    // Regress quadratically on all bogie peaks.
    Align::regressTrain(times, refInfo.positions, true, true, match);

cout << "Full quadratic match:\n" << match.str() << "\n";

    matches.push_back(match);
  }

  sort(matches.begin(), matches.end());

if (matches.empty())
  cout << "Have 0 matches\n";

  return (! matches.empty());
}


void Align::regress(
  const TrainDB& trainDB,
  const PeaksInfo& peaksInfo,
  const bool topFlag)
{
  float bestDist = numeric_limits<float>::max();
  bool changeFlag = false;

  for (auto& ma: matches)
  {
    // It can happen that there too few peaks to recurse on.
    if (ma.numAdd + 4 >= ma.numAxles)
      continue;

    // Only rerun regression if the alignment changed.
    if (ma.dynChangeFlag)
    {
      Align::regressTrain(peaksInfo.times, 
        trainDB.getRefInfo(ma.trainNo).positions, true, true, ma);
      changeFlag = true;
    }

    if (ma.dist < bestDist)
      bestDist = ma.dist;

    if (topFlag)
      ma.setTopResiduals();
  }

  if (changeFlag)
    sort(matches.begin(), matches.end());

  // Skip very unlikely matches.
  if (matches.size() >= 2)
  {
    bestDist = matches.front().dist;
    const float bestDistMatch = matches.front().distMatch;
    auto mit = next(matches.begin());
    while (mit != matches.end() && 
        (mit->dist <= ALIGN_ABS_LIMIT ||
        mit->dist <= ALIGN_RATIO_LIMIT * bestDist) &&
        (mit->distMatch <= ALIGN_ABS_LIMIT ||
        mit->distMatch <= ALIGN_RATIO_LIMIT * bestDistMatch))
      mit++;
    
    if (mit != matches.end())
      matches.erase(mit, matches.end());
  }
}


const Alignment& Align::best() const
{
  return matches.front();
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
  const Alignment& match,
  const unsigned offset,
  const float sampleRate,
  vector<unsigned>& refTimes,
  vector<unsigned>& refPeakTypes,
  vector<float>& refGrades,
  vector<float>& peakGrades) const
{
  Motion const * motion = Align::getMatchingMotion(match.trainName);
  if (motion == nullptr)
    return;

  const unsigned refNo = 
    static_cast<unsigned>(trainDB.lookupNumber(match.trainName));
  const auto& pi = trainDB.getRefInfo(refNo);
  const vector<float>& refPeaks = pi.points;
  const vector<int>& refCars = pi.carNumbersForPoints;
  const vector<int>& refNumbers = pi.peakNumbersForPoints;

  const auto& actualToRef = match.actualToRef;
  const auto& residuals = match.residuals;
  unsigned n;
  int i = 0; // Index in refPeaks
  unsigned aNo = 0; // Index in actualToRef
  unsigned r = 0; // Index in residuals

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
      {
        peakGrades.push_back(-1.f);
        aNo++;
      }
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
      refGrades.push_back(0.f);
      i++;
    }
    else if (aNo == actualToRef.size() ||
        actualToRef[aNo] > refNumbers[i])
    {
      // Reference peak not detected.
      refTimes.push_back(n + offset);
      refPeakTypes.push_back(static_cast<unsigned>(refCars[i]+1));
      refGrades.push_back(-1.f);
      i++;
    }
    else if (actualToRef[aNo] == refNumbers[i])
    {
      // Use up the actual peak seen.
      refTimes.push_back(n + offset);
      refPeakTypes.push_back(static_cast<unsigned>(refCars[i]+1));

      // Look for i/aNo in residuals.
      const unsigned ru = static_cast<unsigned>(refNumbers[i]);
      while (r < residuals.size() && ru > residuals[r].refIndex)
        r++;
      assert(r < residuals.size() && ru == residuals[r].refIndex);
      refGrades.push_back(residuals[r].valueSq);
      peakGrades.push_back(residuals[r].valueSq);

      i++;
      aNo++;
      r++;
    }
    else
    {
      peakGrades.push_back(-1.f);
      aNo++;
    }
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

  // Candidate train may not have been examined in detail as it could
  // not win.
  if (match.residuals.empty())
    return;

  vector<unsigned> refTimes;
  vector<unsigned> refPeakTypes;
  vector<float> refGrades;
  vector<float> peakGrades;
  Align::getBoxTraces(trainDB, match, offset, sampleRate,
    refTimes, refPeakTypes, refGrades, peakGrades);

  writeBinaryUnsigned(dir + "/" + control.timesName() + "/" + filename, 
    refTimes);
  writeBinaryUnsigned(dir + "/" + control.carsName() + "/" + filename, 
    refPeakTypes);
  writeBinaryFloat(dir + "/" + control.refGradeName() + "/" + filename, 
    refGrades);
  writeBinaryFloat(dir + "/" + control.peakGradeName() + "/" + filename, 
    peakGrades);

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
  if (matches.empty())
    return "No matches";

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
      pos[re.refIndex] = re.value;

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
