#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <limits>
#include <cassert>

#include "../database/Control.h"
#include "../database/TrainDB.h"

#include "PolynomialRegression.h"
#include "Regress.h"

#include "../Except.h"
#include "../const.h"



Regress::Regress()
{
}


Regress::~Regress()
{
}


void Regress::storeResiduals(
  const vector<float>& x,
  const vector<float>& y,
  const unsigned lt,
  const float peakScale,
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
      match.residuals[p].valueSq = peakScale * res * res;

      match.distMatch += match.residuals[p].valueSq;

      p++;
    }
  }

  match.dist = match.distMatch + match.distOther;
}


void Regress::specificMatch(
  const vector<float>& times,
  const vector<float>& refPeaks,
  Alignment& match) const
{
  const unsigned lt = times.size();
  const unsigned lr = refPeaks.size();
  assert(lr + match.numAdd == lt + match.numDelete);

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

  // Normalize the distance score to a 200m long train.
  const float trainLength = refPeaks.back() - refPeaks.front();
  const float peakScale = TRAIN_REF_LENGTH * TRAIN_REF_LENGTH / 
    (trainLength * trainLength);

  // Store the residuals.
  match.residuals.resize(lcommon);
  Regress::storeResiduals(x, y, lt, peakScale, match);
}


void Regress::bestMatch(
  const TrainDB& trainDB,
  const vector<float>& times)
{
  float bestDist = numeric_limits<float>::max();

  for (auto& ma: matches)
  {
    // Can we still beat bestAlign?
    if (ma.distOther > bestDist)
      continue;

    Regress::specificMatch(times, trainDB.getPeakPositions(ma.trainNo), ma);

    if (ma.dist < bestDist)
      bestDist = ma.dist;

    ma.setTopResiduals();
  }

  sort(matches.begin(), matches.end());
}


vector<Alignment>& Regress::getMatches()
{
  return matches;
}


void Regress::getBest(
  const unsigned& trainNoTrue,
  string& trainDetected,
  float& distDetected,
  unsigned& rankDetected) const
{
  bool foundFlag = false;
  for (unsigned i = 0; i < matches.size(); i++)
  {
    if (matches[i].trainNo == trainNoTrue)
    {
      foundFlag = true;
      rankDetected = i;
      break;
    }
  }

  if (! foundFlag)
    rankDetected = matches.size();

  trainDetected = matches.front().trainName;
  distDetected = matches.front().distMatch;
}


string Regress::strMatches(const string& title) const
{
  stringstream ss;
  ss << title << "\n";
  for (auto& match: matches)
    ss << match.str();
  ss << "\n";
  return ss.str();
}


string Regress::str(const Control& control) const
{
  stringstream ss;

  const auto& bestAlign = matches.front();

  if (control.verboseRegressMatch())
  {
    ss << Regress::strMatches("Matching alignment");

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


string Regress::strMatchingResiduals(
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

