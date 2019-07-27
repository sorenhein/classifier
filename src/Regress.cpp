#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <limits>

#include "Regress.h"
#include "Except.h"
#include "print.h"

#include "database/TrainDB.h"
#include "database/Control.h"

#include "align/Alignment.h"

#include "regress/PolynomialRegression.h"

#include "stats/Timers.h"

#define GOOD_RESIDUAL_LIMIT 50.0f

extern Timers timers;


Regress::Regress()
{
}


Regress::~Regress()
{
}


double Regress::time2pos(
  const double& time,
  const vector<double>& coeffs) const
{
  double res = 0.;
  double pow = 1.;
  for (unsigned c = 0; c <= coeffs.size(); c++)
  {
    res += coeffs[c] * pow;
    pow *= time;
  }
  return res;
}


double Regress::residuals(
  const vector<double>& x,
  const vector<double>& y,
  const vector<double>& coeffs) const
{
  double res = 0.;
  for (unsigned i = 0; i < x.size(); i++)
  {
    double pos = Regress::time2pos(x[i], coeffs);
    res += (y[i] - pos) * (y[i] - pos);
  }
  return res;
}


void Regress::specificMatch(
  const vector<PeakTime>& times,
  const TrainDB& trainDB,
  const Alignment& match,
  vector<double>& coeffs,
  double& residuals) const
{
  PolynomialRegression pol;
  vector<double> refPeaks;
  vector<double> x, y;
  const unsigned lt = times.size();

  trainDB.getPeakPositions(match.trainNo, refPeaks);
  const unsigned lr = refPeaks.size();
  const double trainLength = refPeaks.back() - refPeaks.front();

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
      x[p] = times[i].time;
      p++;
    }
  }

  pol.fitIt(x, y, 2, coeffs);

  // Normalize the distance score to a 200m long train.
  const double peakScale = 200. * 200. / (trainLength * trainLength);
  residuals = peakScale * 
    Regress::residuals(x, y, coeffs);
}


void Regress::summarizeResiduals(
  const vector<PeakTime>& times,
  const TrainDB& trainDB,
  const vector<double>& coeffs,
  Alignment& match) const
{
  vector<double> refPeaks;
  trainDB.getPeakPositions(match.trainNo, refPeaks);
  const unsigned lr = refPeaks.size();

  vector<RegrEntry> x;
  x.resize(lr);

  const unsigned lt = times.size();
  double sum = 0.;
  for (unsigned i = 0; i < lt; i++)
  {
    if (match.actualToRef[i] != -1)
    {
      const unsigned refIndex = static_cast<unsigned>(match.actualToRef[i]);
      const double position = Regress::time2pos(times[i].time, coeffs);

// cout << refIndex << ";" << refPeaks[refIndex] << ";" << position << "\n";

      x[refIndex].index = refIndex;
      x[refIndex].value = position - refPeaks[refIndex];
      x[refIndex].valueSq = x[refIndex].value * x[refIndex].value;
      sum += x[refIndex].valueSq;
    }
  }

  sort(x.rbegin(), x.rend());

  const double average = sum / lt;

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
      x[i].frac = x[i].valueSq / sum;sum;
      match.topResiduals.push_back(x[i]);
    }
  }
}


void Regress::bestMatch(
  const vector<PeakTime>& times,
  const TrainDB& trainDB,
  const unsigned order,
  const Control& control,
  vector<Alignment>& matches,
  Alignment& bestAlign,
  vector<double>& motionEstimate) const
{
  timers.start(TIMER_REGRESS);

  PolynomialRegression pol;

  vector<double> refPeaks;

  bestAlign.dist = numeric_limits<double>::max();
  vector<double> x, y, coeffs(order+1);
  double residuals;

  const unsigned lt = times.size();

  for (auto& ma: matches)
  {
    if (ma.dist - ma.distMatch > bestAlign.dist)
    {
      // Can never beat bestAlign.
      continue;
    }

// cout << db.lookupTrainName(ma.trainNo) << "\n";

    Regress::specificMatch(times, trainDB, ma, coeffs, residuals);
    Regress::summarizeResiduals(times, trainDB, coeffs, ma);

    if (ma.dist - ma.distMatch + residuals < bestAlign.dist)
    {
      bestAlign = ma;
      bestAlign.dist = ma.dist - ma.distMatch + residuals;
      bestAlign.distMatch = residuals;

      // TODO bestAlign should be a ref/pointer, not duplicated.
      ma = bestAlign;

      motionEstimate[0] = coeffs[0];
      motionEstimate[1] = coeffs[1];
      motionEstimate[2] = 2. * coeffs[2];
      // As the regression estimates 0.5 * a in the physics formula.
    }
    else
    {
      // TODO This is not necessary if we just want the winner
      ma.dist += residuals - ma.distMatch;
      ma.distMatch = residuals;
    }
  }

  timers.stop(TIMER_REGRESS);

  sort(matches.begin(), matches.end());
  bestAlign = matches.front();

printMatches(trainDB, matches);

  if (control.verboseRegressMatch())
  {
    cout << "Regression alignment\n";
    printAlignment(bestAlign, trainDB.lookupName(bestAlign.trainNo));
    cout << endl;
  }

  if (control.verboseRegressMotion())
  {
    cout << "Regression motion\n";
    printMotion(motionEstimate);
  }

printTopResiduals(bestAlign);

}

