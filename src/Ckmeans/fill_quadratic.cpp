/* fill_quadratic.cpp --- an iterative algorithm to compute a
 *   row in the dynamic programming matrix in O(n^2) time.
 *
 * Copyright (C) 2016 Mingzhou Song
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
//
// Joe Song
// Created: August 5, 2016

#include <vector>
#include <algorithm>
#include <iostream>
#include <cassert>
#include <stack>
#include <cmath>

#include "Ckmeans.1d.dp.h"

void fill_row_q(int imin, int imax, int q,
                std::vector< std::vector<ldouble> > & S,
                std::vector< std::vector<size_t> > & J,
                const std::vector<ldouble> & sum_x,
                const std::vector<ldouble> & sum_x_sq,
                const std::vector<ldouble> & sum_w,
                const std::vector<ldouble> & sum_w_sq,
                const enum DISSIMILARITY criterion)
{
  const unsigned qu = static_cast<unsigned>(q);
  // Assumption: each cluster must have at least one point.
  for(unsigned i=static_cast<unsigned>(imin); 
      i<=static_cast<unsigned>(imax); ++i) {
    S[qu][i] = S[qu-1][i-1];
    J[qu][i] = i;
    int jmin = std::max(q, static_cast<int>(J[qu-1][i]));
    for(int j=static_cast<int>(i)-1; j>=jmin; --j) {
      const unsigned ju = static_cast<unsigned>(j);
      ldouble Sj(S[qu-1][ju-1] +
        dissimilarity(criterion, ju, i, sum_x, sum_x_sq, sum_w, sum_w_sq)
                   // ssq(j, i, sum_x, sum_x_sq, sum_w)
      );

      if(Sj < S[qu][i]) {
        S[qu][i] = Sj;
        J[qu][i] = ju;
      }
    }
  }
}
