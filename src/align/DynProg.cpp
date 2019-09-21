// It would be possible to optimize this quite a bit, but it is already
// quite fast.  Specifically:
// 
// * In Needleman-Wunsch we could notice whether the alignment has
//   changed.  If not, there is little need to re-run the regression.
//

#include <cassert>

#include "Alignment.h"
#include "DynProg.h"

#include "../const.h"


DynProg::DynProg()
{
}


DynProg::~DynProg()
{
}


void DynProg::initNeedlemanWunsch(
  const unsigned lreff,
  const unsigned lteff,
  vector<vector<Mentry>>& matrix) const
{
  matrix.resize(lreff+1);
  for (unsigned i = 0; i < lreff+1; i++)
    matrix[i].resize(lteff+1);

  matrix[0][0].dist = 0.;
  for (unsigned i = 1; i < lreff+1; i++)
  {
    matrix[i][0].dist = i * DELETE_PENALTY;
    matrix[i][0].origin = NW_DELETE;
  }

  for (unsigned j = 1; j < lteff+1; j++)
  {
    matrix[0][j].dist = j * INSERT_PENALTY;
    matrix[0][j].origin = NW_INSERT;
  }
}


void DynProg::fillNeedlemanWunsch(
  const vector<float>& refPeaks,
  const vector<float>& scaledPeaks,
  const Alignment& match,
  const unsigned lreff,
  const unsigned lteff,
  vector<vector<Mentry>>& matrix) const
{
  // Run the dynamic programming.
  for (unsigned i = 1; i < lreff+1; i++)
  {
    for (unsigned j = 1; j < lteff+1; j++)
    {
      const float d = refPeaks[i + match.numDelete - 1] - 
        scaledPeaks[j + match.numAdd - 1];
      const float matchVal = matrix[i-1][j-1].dist + d * d;

      // During the first few peaks we don't penalize a missed real peak
      // as heavily, as it could be due to transients etc.
      // If all real peaks are there (firstRefNo == 0), we are lenient on
      // [3, 2] (miss the third real peak),
      // [3, 1] (miss the third and then surely also the second),
      // [2, 1] (miss the second real peak).
      // If firstRefNo == 1, we are only lenient on
      // [2, 1] (miss the second real peak by number here, which is
      // really the third peak).

      const float del = matrix[i-1][j].dist + 
        (match.numDelete <= 1 && 
         i > 1 && i <= 3 - match.numDelete &&
         j < i ? 0 : DELETE_PENALTY); // Ref

      const float ins = matrix[i][j-1].dist + INSERT_PENALTY; // Time

      if (matchVal <= del)
      {
        if (matchVal <= ins)
        {
          matrix[i][j].origin = NW_MATCH;
          matrix[i][j].dist = matchVal;
        }
        else
        {
          matrix[i][j].origin = NW_INSERT;
          matrix[i][j].dist = ins;
        }
      }
      else if (del <= ins)
      {
        matrix[i][j].origin = NW_DELETE;
        matrix[i][j].dist = del;
      }
      else
      {
        matrix[i][j].origin = NW_INSERT;
        matrix[i][j].dist = ins;
      }
    }
  }
}


void DynProg::backtrackNeedlemanWunsch(
  const unsigned lreff,
  const unsigned lteff,
  const vector<vector<Mentry>>& matrix,
  Alignment& match) const
{
  match.dist = matrix[lreff][lteff].dist;
  
  // Add fixed penalties for early issues.
  match.dist += match.numDelete * EARLY_SHIFTS_PENALTY +
    match.numAdd * EARLY_SHIFTS_PENALTY;

  unsigned i = lreff;
  unsigned j = lteff;
  const unsigned numAdd = match.numAdd;
  const unsigned numDelete = match.numDelete;

  while (i > 0 || j > 0)
  {
    const Origin o = matrix[i][j].origin;
    if (i > 0 && j > 0 && o == NW_MATCH)
    {
      match.actualToRef[j + numAdd - 1] = 
        static_cast<int>(i + numDelete - 1);
      match.distMatch += matrix[i][j].dist - matrix[i-1][j-1].dist;
      i--;
      j--;
    }
    else if (j > 0 && o == NW_INSERT)
    {
      match.actualToRef[j + numAdd - 1] = -1;
      match.numAdd++;
      j--;
    }
    else
    {
      match.numDelete++;
      i--;
    }
  }

  match.distOther = match.dist - match.distMatch;
}


void DynProg::run(
  const vector<float>& refPeaks,
  const vector<float>& scaledPeaks,
  Alignment& match) const
{
  // https://en.wikipedia.org/wiki/Needleman%E2%80%93Wunsch_algorithm
  // This is an adaptation of the sequence-matching algorithm.
  //
  // 1. The "letters" are peak positions in meters, so the metric
  //    between letters is the square of the physical distance.
  //
  // 2. There is a custom penalty function for early deletions (from
  //    the synthetic trace, scaledPeaks, i.e. an early insertion in
  //    refPeaks) and for early insertions (in scalePeaks, i.e. an
  //    early deletion in refPeaks).
  //
  // The first dimension is refPeaks, the second is the synthetic one.
  
  const unsigned lr = refPeaks.size() - match.numDelete;
  const unsigned lt = scaledPeaks.size() - match.numAdd;

  // Set up the matrix.
  vector<vector<Mentry>> matrix;
  DynProg::initNeedlemanWunsch(lr, lt, matrix);

  // Fill the matrix with distances and origins.
  DynProg::fillNeedlemanWunsch(refPeaks, scaledPeaks, match,
    lr, lt, matrix);

  // Walk back through the matrix.
  DynProg::backtrackNeedlemanWunsch(lr, lt, matrix, match);

  assert(refPeaks.size() + match.numAdd == 
    scaledPeaks.size() + match.numDelete);
}

