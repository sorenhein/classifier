// https://gist.github.com/chrisengelsma
// Simplified by Soren Hein to compile with Visual C++.

#include <vector>
#include <iostream>
#include <stdlib.h>
#include <math.h>

#include "PolynomialRegression.h"


PolynomialRegression::PolynomialRegression()
{
}


PolynomialRegression::~PolynomialRegression()
{
}


bool PolynomialRegression::fitIt( 
  const vector<double> & x,
  const vector<double> & y,
  const int &order,
  std::vector<double> &coeffs)
{
  // The size of xValues and yValues should be same
  if (x.size() != y.size()) 
  {
    cout << "The size of x & y arrays are different" << endl;
    return false;
  }

  // The size of xValues and yValues cannot be 0, should not happen
  if (x.size() == 0 || y.size() == 0) 
  {
    cout << "The size of x or y arrays is 0" << endl;
    return false;
  }
  
  size_t N = x.size();
  unsigned n = static_cast<unsigned>(order);
  unsigned np1 = n + 1;
  unsigned np2 = n + 2;
  unsigned tnp1 = 2 * n + 1;
  double tmp;

  // X = vector that stores values of sigma(xi^2n)
  vector<double> X(tnp1);
  for (unsigned i = 0; i < tnp1; ++i) 
  {
    X[i] = 0;
    for (unsigned j = 0; j < N; ++j)
      X[i] += static_cast<double>(pow(x[j], i));
  }

  // a = vector to store final coefficients.
  std::vector<double> a(np1);

  // B = normal augmented matrix that stores the equations.
  vector<vector<double>> B(np1, vector<double> (np2, 0));

  for (unsigned i = 0; i <= n; ++i) 
    for (unsigned j = 0; j <= n; ++j) 
      B[i][j] = X[i + j];

  // Y = vector to store values of sigma(xi^n * yi)
  vector<double> Y(np1);
  for (unsigned i = 0; i < np1; ++i) 
  {
    Y[i] = 0.;
    for (unsigned j = 0; j < N; ++j) 
      Y[i] += static_cast<double>(pow(x[j], i) * y[j]);
  }

  // Load values of Y as last column of B
  for (unsigned i = 0; i <= n; ++i) 
    B[i][np1] = Y[i];

  n += 1;
  const unsigned nm1 = n-1;

  // Pivotisation of the B matrix.
  for (unsigned i = 0; i < n; ++i) 
    for (unsigned k = i+1; k < n; ++k) 
      if (B[i][i] < B[k][i]) 
        for (unsigned j = 0; j <= n; ++j) 
        {
          tmp = B[i][j];
          B[i][j] = B[k][j];
          B[k][j] = tmp;
        }

  // Performs the Gaussian elimination.
  // (1) Make all elements below the pivot equals to zero
  //     or eliminate the variable.
  for (unsigned i = 0; i < nm1; ++i)
    for (unsigned k = i+1; k < n; ++k) 
    {
      double t = B[k][i] / B[i][i];
      for (unsigned j = 0; j <= n; ++j)
        B[k][j] -= t*B[i][j];         // (1)
    }

  // Back substitution.
  // (1) Set the variable as the rhs of last equation
  // (2) Subtract all lhs values except the target coefficient.
  // (3) Divide rhs by coefficient of variable being calculated.
  for (int i = static_cast<int>(nm1); i >= 0; --i) 
  {
    const unsigned iu = static_cast<unsigned>(i);
    a[iu] = static_cast<double>(B[iu][n]);                   // (1)
    for (unsigned j = 0; j < n; ++j)
      if (static_cast<int>(j) != i)
        a[iu] -= static_cast<double>(B[iu][j]) * a[j];       // (2)
    a[iu] /= static_cast<double>(B[iu][iu]); // (3)
  }

  coeffs.resize(a.size());
  for (size_t i = 0; i < a.size(); ++i) 
    coeffs[i] = a[i];

  return true;
}

