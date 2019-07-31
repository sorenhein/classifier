#include <iostream>
#include <sstream>
#include <algorithm>
#include <limits>

#include "Regress.h"
#include "Except.h"

#include "align/Alignment.h"

#include "database/Control.h"
#include "database/TrainDB.h"

#include "regress/PolynomialRegression.h"

#include "util/Motion.h"


Regress::Regress()
{
}


Regress::~Regress()
{
}


float Regress::time2pos(
  const float& time,
  const vector<float>& coeffs) const
{
  float res = 0.;
  float pow = 1.;
  for (unsigned c = 0; c < coeffs.size(); c++)
  {
    res += coeffs[c] * pow;
    pow *= time;
  }
  return res;
}


void Regress::specificMatch(
  const vector<float>& times,
  const vector<float>& refPeaks,
  Alignment& match) const
{
  PolynomialRegression pol;
  vector<float> x, y;
  const unsigned lt = times.size();

  const unsigned lr = refPeaks.size();
  const float trainLength = refPeaks.back() - refPeaks.front();

  if (lr + match.numAdd != lt + match.numDelete)
    THROW(ERR_REGRESS, "Number of regression elements don't add up");

  const unsigned lcommon = lt - match.numAdd;
  x.resize(lcommon);
  y.resize(lcommon);

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

  pol.fitIt(x, y, 2, match.motion.estimate);

  // Normalize the distance score to a 200m long train.
  const float peakScale = 200.f * 200.f / (trainLength * trainLength);

  // Store the residuals.
  match.distMatch = 0.f;
  match.residuals.resize(lcommon);
  for (unsigned i = 0, p = 0; i < lt; i++)
  {
    if (match.actualToRef[i] >= 0)
    {
      const float pos = Regress::time2pos(x[p], match.motion.estimate);
      const float res = pos - y[p];

      match.residuals[p].index = static_cast<unsigned>(match.actualToRef[i]);
      match.residuals[p].value = res;
      match.residuals[p].valueSq = peakScale * res * res;

      match.distMatch += match.residuals[p].valueSq;

      p++;
    }
  }

  match.dist = match.distMatch + match.distOther;
}


void Regress::bestMatch(
  const TrainDB& trainDB,
  const vector<float>& times,
  vector<Alignment>& matches) const
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


string Regress::str(
  const Control& control,
  const vector<Alignment>& matches) const
{
  stringstream ss;

  const auto& bestAlign = matches.front();

  if (control.verboseRegressMatch())
  {
    ss << "Matching alignment\n";
    for (auto& match: matches)
      ss << match.str();
    ss << "\n";

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

