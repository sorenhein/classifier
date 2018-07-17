#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <limits>

#include "Regress.h"
#include "Database.h"
#include "regress/PolynomialRegression.h"


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
  const int order,
  vector<Alignment>& matches,
  Alignment& bestAlign,
  vector<double>& motionEstimate) const
{
  PolynomialRegression pol;

  vector<PeakPos> refPeaks;

  bestAlign.dist = numeric_limits<double>::max();
  vector<double> x, y, coeffs(order+1);

  const unsigned lt = times.size();

  for (auto& ma: matches)
  {
    db.getPerfectPeaks(ma.trainNo, refPeaks);
    const unsigned lr = refPeaks.size();

    if (lr + ma.numAdd != lt + ma.numDelete)
    {
      cout << "Lengths don't add up\n";
      return;
    }

    const unsigned lcommon = lt - ma.numAdd;
    x.resize(lcommon);
    y.resize(lcommon);

    for (unsigned i = 0, p = 0; i < lt; i++)
    {
      if (ma.actualToRef[i] >= 0)
      {
        y[p] = refPeaks[ma.actualToRef[i]].pos;
        x[p] = times[i].time;
        p++;
      }
    }

    pol.fitIt(x, y, 2, coeffs);

    // TODO Must get actual residuals;
    const double residuals = Regress::residuals(x, y, coeffs);

    if (ma.dist - ma.distMatch + residuals < bestAlign.dist)
    {
      bestAlign = ma;
      bestAlign.dist = ma.dist - ma.distMatch + residuals;

      motionEstimate[0] = coeffs[0];
      motionEstimate[1] = coeffs[1];
      motionEstimate[2] = 2. * coeffs[2];
      // As the regression estimates 0.5 * a in the physics formula.
    }
  }
}

