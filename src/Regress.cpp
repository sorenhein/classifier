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
  // for (unsigned c = 0; c <= coeffs.size(); c++)
  for (unsigned c = 0; c < coeffs.size(); c++)
  {
    res += coeffs[c] * pow;
    pow *= time;
  }
  return res;
}


float Regress::residuals(
  const vector<float>& x,
  const vector<float>& y,
  const vector<float>& coeffs) const
{
  float res = 0.;
  for (unsigned i = 0; i < x.size(); i++)
  {
    float pos = Regress::time2pos(x[i], coeffs);
    res += (y[i] - pos) * (y[i] - pos);
  }
  return res;
}


void Regress::specificMatch(
  const vector<float>& times,
  const TrainDB& trainDB,
  const Alignment& match,
  vector<float>& coeffs,
  float& residuals) const
{
  PolynomialRegression pol;
  vector<float> refPeaks;
  vector<float> x, y;
  const unsigned lt = times.size();

  trainDB.getPeakPositions(match.trainNo, refPeaks);
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
      x[p] = static_cast<float>(times[i]);
      p++;
    }
  }

  pol.fitIt(x, y, 2, coeffs);

  // Normalize the distance score to a 200m long train.
  const float peakScale = 200.f * 200.f / (trainLength * trainLength);
  residuals = peakScale * 
    Regress::residuals(x, y, coeffs);
}


void Regress::summarizeResiduals(
  const vector<float>& times,
  const TrainDB& trainDB,
  const vector<float>& coeffs,
  Alignment& match) const
{
  vector<float> refPeaks;
  trainDB.getPeakPositions(match.trainNo, refPeaks);
  const unsigned lr = refPeaks.size();

  vector<RegrEntry> x;
  x.resize(lr);

  const unsigned lt = times.size();
  float sum = 0.;
  for (unsigned i = 0; i < lt; i++)
  {
    if (match.actualToRef[i] != -1)
    {
      const unsigned refIndex = static_cast<unsigned>(match.actualToRef[i]);
      const float position = Regress::time2pos(times[i], coeffs);

// cout << refIndex << ";" << refPeaks[refIndex] << ";" << position << "\n";

      x[refIndex].index = refIndex;
      x[refIndex].value = position - refPeaks[refIndex];
      x[refIndex].valueSq = x[refIndex].value * x[refIndex].value;
      sum += x[refIndex].valueSq;
    }
  }

  sort(x.rbegin(), x.rend());

  const float average = sum / lt;

  unsigned i = 0;
  unsigned last = numeric_limits<unsigned>::max();
  while (i+1 < lt && x[i].valueSq > 2. * average)
  {
    if (x[i].valueSq > x[i+1].valueSq + 0.5 * average)
      last = i;
    i++;
  }

  if (last != numeric_limits<unsigned>::max())
  {
    for (i = 0; i <= last; i++)
    {
      x[i].frac = x[i].valueSq / sum;
      match.topResiduals.push_back(x[i]);
    }
  }
}


void Regress::bestMatch(
  const vector<float>& times,
  const TrainDB& trainDB,
  const Control& control,
  vector<Alignment>& matches,
  Alignment& bestAlign,
  Motion& motion) const
{
  PolynomialRegression pol;

  vector<float> coeffs(motion.order+1);
  float residuals;

  float bestDist = numeric_limits<float>::max();

  for (auto& ma: matches)
  {
    if (ma.dist - ma.distMatch > bestDist)
    {
      // Can never beat bestAlign.
      continue;
    }

// cout << db.lookupTrainName(ma.trainNo) << "\n";

    Regress::specificMatch(times, trainDB, ma, coeffs, residuals);
    Regress::summarizeResiduals(times, trainDB, coeffs, ma);

    ma.dist += residuals - ma.distMatch;
    ma.distMatch = residuals;

    if (ma.dist < bestDist)
    {
      bestDist = ma.dist;
      motion.setEstimate(coeffs);
    }
  }

  sort(matches.begin(), matches.end());
  bestAlign = matches.front();

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
    cout << motion.strEstimate("Regression motion");

  cout << bestAlign.strTopResiduals();

}

