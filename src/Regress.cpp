#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <limits>

#include "Regress.h"
#include "Except.h"

#include "database/TrainDB.h"
#include "database/Control.h"

#include "align/Alignment.h"

#include "util/Motion.h"

#include "regress/PolynomialRegression.h"


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
  const vector<float>& times,
  const TrainDB& trainDB,
  const Control& control,
  vector<Alignment>& matches) const
{
  // PolynomialRegression pol;

  float bestDist = numeric_limits<float>::max();

  for (auto& ma: matches)
  {
    // Can we still beat bestAlign?
    if (ma.distOther > bestDist)
      continue;

    const vector<float>& refPeaks =
      trainDB.getPeakPositions(ma.trainNo);

// cout << db.lookupTrainName(ma.trainNo) << "\n";

    Regress::specificMatch(times, refPeaks, ma);

    ma.setTopResiduals();

    if (ma.dist < bestDist)
      bestDist = ma.dist;
  }

  sort(matches.begin(), matches.end());
  auto& bestAlign = matches.front();

  cout << "Matching alignment\n";
  for (auto& match: matches)
    cout << match.str();
  cout << "\n";

  if (control.verboseRegressMatch())
  {
    cout << "Regression alignment\n";
    cout << bestAlign.str();
    cout << endl;
  }

  if (control.verboseRegressMotion())
    cout << bestAlign.motion.strEstimate("Regression motion");

  cout << bestAlign.strTopResiduals();
}


