#ifndef _POLYNOMIAL_REGRESSION_H
#define _POLYNOMIAL_REGRESSION_H

// https://gist.github.com/chrisengelsma
// Simplified by Soren Hein to compile with Visual C++.

/**
 * PURPOSE:
 *
 *  Polynomial Regression aims to fit a non-linear relationship to a set of
 *  points. It approximates this by solving a series of linear equations using 
 *  a least-squares approach.
 *
 *  We can model the expected value y as an nth degree polynomial, yielding
 *  the general polynomial regression model:
 *
 *  y = a0 + a1 * x + a2 * x^2 + ... + an * x^n
 * @author Chris Engelsma
 */

#include <vector>

using namespace std;


class PolynomialRegression 
{

  public:

    PolynomialRegression();

    ~PolynomialRegression();

    bool fitIt(
      const vector<float>& x,
      const vector<float>& y,
      const int order,
      vector<float>& coeffs);
};

#endif
