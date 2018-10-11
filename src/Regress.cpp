#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <limits>

#include "Regress.h"
#include "Database.h"
#include "Timers.h"
#include "Except.h"
#include "print.h"
#include "regress/PolynomialRegression.h"

extern Timers timers;


Regress::Regress()
{
}


Regress::~Regress()
{
}


double Regress::residuals(
  const vector<double>& x,
  const vector<double>& y,
  const vector<double>& coeffs) const
{
  double res = 0.;
  for (unsigned i = 0; i < x.size(); i++)
  {
    double rhs = 0.;
    double pow = 1.;
    for (unsigned c = 0; c <= coeffs.size(); c++)
    {
      rhs += coeffs[c] * pow;
      pow *= x[i];
    }

    res += (y[i] - rhs) * (y[i] - rhs);
  }
  return res;
}


void Regress::bestMatch(
  const vector<PeakTime>& times,
  const Database& db,
  const unsigned order,
  const Control& control,
  vector<Alignment>& matches,
  Alignment& bestAlign,
  vector<double>& motionEstimate) const
{
  timers.start(TIMER_REGRESS);

  PolynomialRegression pol;

  vector<PeakPos> refPeaks;

  bestAlign.dist = numeric_limits<double>::max();
  vector<double> x, y, coeffs(order+1);

  const unsigned lt = times.size();

  for (auto& ma: matches)
  {
    if (ma.dist - ma.distMatch > bestAlign.dist)
    {
      // Can never best bestAlign.
      continue;
    }

    db.getPerfectPeaks(ma.trainNo, refPeaks);
    const unsigned lr = refPeaks.size();
    const double trainLength = refPeaks.back().pos - refPeaks.front().pos;

    if (lr + ma.numAdd != lt + ma.numDelete)
      THROW(ERR_REGRESS, "Number of regression elements don't add up");

    const unsigned lcommon = lt - ma.numAdd;
    x.resize(lcommon);
    y.resize(lcommon);

    for (unsigned i = 0, p = 0; i < lt; i++)
    {
      if (ma.actualToRef[i] >= 0)
      {
        y[p] = refPeaks[static_cast<unsigned>(ma.actualToRef[i])].pos;
        x[p] = times[i].time;
        p++;
      }
    }

    pol.fitIt(x, y, 2, coeffs);

    // Normalize the distance score to a 200m long train.
    const double peakScale = 200. * 200. / (trainLength * trainLength);
    const double residuals = peakScale * 
      Regress::residuals(x, y, coeffs);

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
printMatches(db, matches);

  if (control.verboseRegressMatch)
  {
    cout << "Regression alignment\n";
    printAlignment(bestAlign, db.lookupTrainName(bestAlign.trainNo));
    cout << endl;
  }

  if (control.verboseRegressMotion)
  {
    cout << "Regression motion\n";
    printMotion(motionEstimate);
  }
}

