#include <cassert>
#include <algorithm>

#include "Alignment.h"
#include "DynProg.h"

#include "../Peak.h"
#include "../PeakGeneral.h"

#include "../const.h"


// These are not essential parameters.  The sizes are used as a 
// starting point for the matrix, but it will grow if needed.

#define DYN_DEFAULT_REF 80
#define DYN_DEFAULT_SEEN 80

// This is an optimization in which not the whole matrix is filled
// out, only a band around the diagonal.

#define DYN_RANGE 8


DynProg::DynProg()
{
  lenPenaltyRef = DYN_DEFAULT_REF;
  lenPenaltySeen = DYN_DEFAULT_SEEN;
  lenMatrixRef = DYN_DEFAULT_REF;
  lenMatrixSeen = DYN_DEFAULT_SEEN;

  penaltyRef.resize(lenPenaltyRef);
  penaltySeen.resize(lenPenaltySeen);

  matrix.resize(lenMatrixRef);
  for (unsigned i = 0; i < lenMatrixRef; i++)
    matrix[i].resize(lenMatrixSeen);
}


DynProg::~DynProg()
{
}


void DynProg::initNeedlemanWunsch(
  const vector<float>& seenPenaltyFactor,
  const unsigned refSize,
  const unsigned seenSize,
  const Alignment& match)
{
  // This the penalty marginal for deleting reference peaks.
  if (refSize > lenPenaltyRef)
  {
    penaltyRef.resize(refSize);
    lenPenaltyRef = refSize;
  }

  for (unsigned i = 0; i < match.numDelete; i++)
    penaltyRef[i] = EARLY_SHIFTS_PENALTY;
  for (unsigned i = match.numDelete; i < refSize; i++)
    penaltyRef[i] = DELETE_PENALTY;

  // This the penalty marginal for adding (spurious?) seen peaks.
  if (seenSize > lenPenaltySeen)
  {
    penaltySeen.resize(seenSize);
    lenPenaltySeen = seenSize;
  }

  for (unsigned j = 0; j < seenSize; j++)
    penaltySeen[j] = INSERT_PENALTY * seenPenaltyFactor[j];

  // The first matrix dimension is refPeaks, the second is the seen one.
  // We can imagine the first index as the row index.
  //
  //      j =  0   1   2   3   4   5   ...
  //       +---------------------------------
  // i = 0 |   0 ins ins ins ins ins
  //     1 | del
  //     2 | del
  //     3 | del
  //     4 | del
  //     5 | del

  lenMatrixRefUsed = refSize - match.numDelete + 1;
  lenMatrixSeenUsed = seenSize - match.numAdd + 1;

  if (lenMatrixRefUsed > lenMatrixRef)
  {
    matrix.resize(lenMatrixRefUsed);
    lenMatrixRef = lenMatrixRefUsed;
  }

  if (lenMatrixSeenUsed > lenMatrixSeen)
  {
    for (unsigned i = 0; i < lenMatrixRefUsed; i++)
      matrix[i].resize(lenMatrixSeenUsed);
    lenMatrixSeen = lenMatrixSeenUsed;
  }

  matrix[0][0].dist = 0.;
  for (unsigned i = 1; i < lenMatrixRefUsed; i++)
  {
    matrix[i][0].dist = matrix[i-1][0].dist +
      penaltyRef[i + match.numDelete - 1];
    matrix[i][0].origin = NW_DELETE;
  }
  for (unsigned j = 1; j < lenMatrixSeenUsed; j++)
  {
    matrix[0][j].dist = matrix[0][j-1].dist + 
      penaltySeen[j + match.numAdd - 1];
    matrix[0][j].origin = NW_INSERT;
  }
}


void DynProg::fillNeedlemanWunsch(
  const vector<float>& refPeaks,
  const vector<float>& scaledPeaks,
  Alignment& match)
{
  // Run the dynamic programming.
  for (unsigned i = 1; i < lenMatrixRefUsed; i++)
  {
    const unsigned jmin = (i < DYN_RANGE+1 ? 1 : i - DYN_RANGE);
    const unsigned jmax = min(i+DYN_RANGE, lenMatrixSeenUsed-1);

    // We could go 1 .. lenMatrixSeenUsed, but this optimization 
    // is sufficient.
    for (unsigned j = jmin; j <= jmax; j++)
    {
      // Calculate the cell value if we come diagonally from the
      // upper left, with neither deletion nor insertion.
       const float d = refPeaks[i + match.numDelete - 1] - 
         scaledPeaks[j + match.numAdd - 1];
       const float matchVal = matrix[i-1][j-1].dist + d * d;

      // Calculate the cell value if we come from the left through
      // the deletion of a reference peak, i.e. a missing seen peak.
      float del;
      if (j == jmax)
        del = numeric_limits<float>::max();
      else
        del = matrix[i-1][j].dist + penaltyRef[i + match.numDelete - 1];

      // Calculate the cell value if we come from above through the
      // insertion of a reference peak, i.e. a spurious peak in our
      // detected, scaled peaks.
      float ins;
      if (j == jmin)
        ins = numeric_limits<float>::max();
      else
        ins = matrix[i][j-1].dist + penaltySeen[j + match.numAdd- 1];

      if (matchVal <= del)
      {
        if (matchVal <= ins)
        {
          // The diagonal match wins.
          matrix[i][j].origin = NW_MATCH;
          matrix[i][j].dist = matchVal;
        }
        else
        {
          // The insertion of a spurious peak wins.
          matrix[i][j].origin = NW_INSERT;
          matrix[i][j].dist = ins;
        }
      }
      else if (del <= ins)
      {
        // The deletion of a reference peak wins.
        matrix[i][j].origin = NW_DELETE;
        matrix[i][j].dist = del;
      }
      else
      {
        // The insertion of a spurious peak wins.
        matrix[i][j].origin = NW_INSERT;
        matrix[i][j].dist = ins;
      }
    }
  }

  match.dist = matrix[lenMatrixRefUsed-1][lenMatrixSeenUsed-1].dist;
  
  // Add fixed penalties for early issues outside of Needleman-Wunsch.
  for (unsigned i = 0; i < match.numDelete; i++)
    match.dist += penaltyRef[i];
  for (unsigned j = 0; j < match.numAdd; j++)
    match.dist += penaltySeen[j];
}


void DynProg::backtrackNeedlemanWunsch(Alignment& match)
{
  unsigned i = lenMatrixRefUsed-1;
  unsigned j = lenMatrixSeenUsed-1;
  const unsigned numAdd = match.numAdd;
  const unsigned numDelete = match.numDelete;
  match.dynChangeFlag = false;

  while (i > 0 || j > 0)
  {
    const Origin o = matrix[i][j].origin;
    if (i > 0 && j > 0 && o == NW_MATCH)
    {
      const int a2rNew = static_cast<int>(i + numDelete - 1);
      if (match.actualToRef[j + numAdd - 1] != a2rNew)
        match.dynChangeFlag = true;

      match.actualToRef[j + numAdd - 1] = a2rNew;
      match.distMatch += matrix[i][j].dist - matrix[i-1][j-1].dist;
      i--;
      j--;
    }
    else if (j > 0 && o == NW_INSERT)
    {
      if (match.actualToRef[j + numAdd - 1] != -1)
        match.dynChangeFlag = true;

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
  const PeaksInfo& peaksInfo,
  const vector<float>& scaledPeaks,
  Alignment& match)
{
  // https://en.wikipedia.org/wiki/Needleman%E2%80%93Wunsch_algorithm
  // This is an adaptation of the sequence-matching algorithm.
  //
  // 1. The "letters" are peak positions in meters, so the metric
  //    between letters is the square of the physical distance.
  //
  // 2. There is a custom penalty function for early deletions 
  //    and for early insertions.
  //
  // An insertion can occur when there are 29 detected peaks in scaledPeaks
  // and only 28 reference peaks.  It could be that the first detected
  // peak is spurious and was inserted by our algorithm.
  //
  // A deletion can occur in the same example when there are 32
  // reference peaks.  Then there might have been three deletions
  // at the front which were missed.
  
  // Set up the matrix and the penalty marginals.
  DynProg::initNeedlemanWunsch(peaksInfo.penaltyFactor,
    refPeaks.size(), peaksInfo.peaks.size(), match);

  // Fill the matrix with distances and origins.
  DynProg::fillNeedlemanWunsch(refPeaks, scaledPeaks, match);

  // Walk back through the matrix.
  DynProg::backtrackNeedlemanWunsch(match);

  assert(refPeaks.size() + match.numAdd == 
    scaledPeaks.size() + match.numDelete);
}
