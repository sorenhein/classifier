#include <iostream>
#include <iomanip>
#include <sstream>

#include "PeakPiece.h"
#include "Except.h"

#define SLIDING_LOWER 0.9f
#define SLIDING_UPPER 1.1f
#define SLIDING_UPPER_SQ (SLIDING_UPPER * SLIDING_UPPER)



PeakPiece::PeakPiece()
{
  PeakPiece::reset();
}


PeakPiece::~PeakPiece()
{
}


void PeakPiece::reset()
{
  _extrema.clear();
}


void PeakPiece::logExtremum(
  const unsigned index,
  const int cumul,
  const int direction)
{
  _extrema.emplace_back(DistEntry());
  DistEntry& de = _extrema.back();
  de.index = index;
  de.indexHi = 0;
  de.cumul = cumul;
  de.direction = direction;
}


bool PeakPiece::operator < (const unsigned limit) const
{
  return (_summary.cumul < static_cast<int>(limit));
}


bool PeakPiece::operator <= (const unsigned limit) const
{
  return (_summary.cumul <= static_cast<int>(limit));
}


bool PeakPiece::operator > (const unsigned limit) const
{
  return (_summary.cumul > static_cast<int>(limit));
}


bool PeakPiece::apart(
  const PeakPiece& piece2,
  const float factor) const
{
  return (piece2._summary.index >= 
    static_cast<unsigned>(factor * _summary.index));
}


bool PeakPiece::oftener(
  const PeakPiece& piece2,
  const float factor) const
{
  return (_summary.cumul >= 
    static_cast<int>(factor * piece2._summary.cumul));
}


bool PeakPiece::oftener(
  const Gap& gap,
  const float factor) const
{
  return (_summary.cumul >= static_cast<int>(factor * gap.count));
}


void PeakPiece::copySummaryFrom(const PeakPiece& piece2)
{
  _summary = piece2._summary;
}


void PeakPiece::eraseSmallMaxima(const unsigned limit)
{
  if (_modality == 1)
    return;

  const int limitI = static_cast<int>(limit);
  for (auto eit = _extrema.begin(); eit != _extrema.end(); )
  {
    if (eit->direction == -1)
      eit++;
    else if (eit->cumul <= limitI)
    {
      // Keep the lowest minimum if there is a choice.
      if (next(eit) == _extrema.end() ||
          (eit != _extrema.begin() &&
          prev(eit)->cumul > next(eit)->cumul))
        eit = prev(eit);

      eit = _extrema.erase(eit);
      eit = _extrema.erase(eit);
      _modality--;
    }
    else
      eit++;
  }
}


void PeakPiece::summarize()
{
  _modality = 0;
  _summary.cumul = 0;
  for (auto& de: _extrema)
  {
    if (de.direction == 1)
    {
      _modality++;
      if (de.cumul > _summary.cumul)
      {
        _summary.index = de.index;
        _summary.indexHi = de.indexHi;
        _summary.cumul = de.cumul;
      }
    }
  }
}


void PeakPiece::splitOnIndices(
  const unsigned indexLeft,
  const unsigned indexRight,
  PeakPiece& piece2)
{
  // Stop current piece at indexLeft.
  for (auto eit = _extrema.begin(); eit != _extrema.end(); eit++)
  {
    if (eit->index == indexLeft)
    {
      _extrema.erase(next(eit), _extrema.end());
      break;
    }
  }

  // Begin piece2 at indexRight.
  for (auto eit = piece2._extrema.begin();
      eit != piece2._extrema.end(); eit++)
  {
    if (eit->index == indexRight)
    {
      piece2._extrema.erase(piece2._extrema.begin(), eit);
      break;
    }
  }

  PeakPiece::summarize();
  piece2.summarize();
}


bool PeakPiece::splittableOnDip(
  unsigned& indexLeft,
  unsigned& indexRight) const
{
  // Split if two consecutive maxima are not within +/- 10%.
  // Also split if there's a fairly sharp dip:  Either side must be
  // >= 4 and the bottom must be less than half of the smallest
  // neighbor.

  for (auto eit = _extrema.begin(); eit != _extrema.end(); )
  {
    if (next(eit) == _extrema.end())
      return false;

    if (eit->direction == -1)
    {
      eit++;
      continue;
    }

    auto nneit = next(next(eit));
    if (static_cast<unsigned>(
        SLIDING_UPPER_SQ * eit->index) >= nneit->index)
    {
      // Next one is not too far away.
      const int a = eit->cumul;
      const int b = next(eit)->cumul;
      const int c = nneit->cumul;
      if (a <= 3 || c <= 3 || 2*b >= a || 2*b >= c)
      {
        // The cumul values do not make for a convincing split.

        eit++;
        continue;
      }
    }

    indexLeft = eit->index;
    indexRight = nneit->index;
    return true;
  }
  // Doesn't happen.
  return false;
}


bool PeakPiece::splittableOnGap(
  unsigned& indexLeft,
  unsigned& indexRight) const
{
  // Split if the overall piece is too long, even though there
  // was no obvious dip on which to split.  Choose the position
  // with the largest relative gap.

  const unsigned pitStart = _extrema.front().index;
  const unsigned pitStop = static_cast<unsigned>(
    SLIDING_UPPER_SQ * pitStart);

  if (_extrema.back().index <= pitStop)
    return false;

  float factorMax = 0.f;
  indexLeft = 0;
  indexRight = 0;
  unsigned indexPrev = pitStart;

  for (auto eit = next(_extrema.begin()); eit != _extrema.end(); eit++)
  {
    if (eit->direction == -1)
      continue;

    float factor = eit->index / static_cast<float>(indexPrev);
    if (factor > factorMax)
    {
      indexLeft = indexPrev;
      indexRight = eit->index;
      factorMax = factor;
    }

    indexPrev = eit->index;
  }

  return true;
}


bool PeakPiece::extend(Gap& wheelGap) const
{
  const unsigned p = _summary.index;
  const unsigned plo = static_cast<unsigned>(p / 1.1f);
  const unsigned phi = static_cast<unsigned>(p * 1.1f);
    
  const unsigned wlo = static_cast<unsigned>(wheelGap.lower / 1.1f);
  const unsigned whi = static_cast<unsigned>(wheelGap.upper * 1.1f);

cout << "trying index " << p << ", " << plo << "-" << phi << endl;
cout << "wheel gap " << wheelGap.lower << "-" << wheelGap.upper << endl;
cout << "wheel tol " << wlo << "-" << whi << endl;

  if (plo <= wheelGap.upper && wheelGap.lower <= phi)
  {
    if (plo < wheelGap.lower)
    {
      wheelGap.lower = plo;
cout << "REGRADE branch 0: overlap lo\n";
    }
    if (phi > wheelGap.upper)
    {
      wheelGap.upper = phi;
cout << "REGRADE branch 0: overlap hi\n";
    }
  }
  else if (phi >= wlo && phi <= wheelGap.lower)
  {
    // Extend on the low end.
    wheelGap.lower = phi - 1;
cout << "REGRADE branch 1\n";
  }
  else if (plo >= wheelGap.upper && plo <= whi)
  {
    // Extend on the high end.
    wheelGap.upper = plo + 1;
cout << "REGRADE branch 2\n";
  }
  else if (plo >= wlo && plo <= wheelGap.lower)
  {
    // Extend on the low end.
    wheelGap.lower = plo - 1;
cout << "REGRADE branch 3\n";
  }
  else if (phi >= wheelGap.upper && phi <= whi)
  {
    // Extend on the high end.
    wheelGap.upper = phi + 1;
cout << "REGRADE branch 4\n";
  }
  else
    return false;

  wheelGap.count += _summary.cumul;
  return true;
}


void PeakPiece::unjitter()
{
  if (_modality == 1)
    return;

  for (auto eit = _extrema.begin(); eit != _extrema.end(); )
  {
    if (eit->direction == 1)
    {
      eit++;
      continue;
    }

    auto pit = prev(eit);
    auto nit = next(eit);
    const int a = pit->cumul; // Max
    const int b = eit->cumul; // Min
    const int c = nit->cumul; // Max
    const bool leftHighFlag = (a >= c);
    const int m = (leftHighFlag ? c : a);
    if (m < 4 && m - b > 2)
    {
      eit++;
      continue;
    }

    // So now we've got a dip of 1-2 below the lowest maximum
    // which is at least 4.

    const int M = (leftHighFlag ? a : c);
    int limit = static_cast<int>(0.1f * M);
    if (limit == 0)
      limit = 1;

    if (leftHighFlag)
    {
      // Keep a memory of the right maximum in this maximum.
      if (M-m <= limit)
        pit->indexHi = nit->index;
    }
    else
    {
      eit = pit;
      if (M-m <= limit)
      {
        // Keep a memory of the left maximum in this maximum.
        nit->indexHi = nit->index;
        nit->index = pit->index;
      }
    }

    eit = _extrema.erase(eit);
    eit = _extrema.erase(eit);
    _modality--;
  }
}


unsigned PeakPiece::modality() const
{
  return _modality;
}


const DistEntry& PeakPiece::summary() const
{
  return _summary;
}


bool PeakPiece::getGap(Gap& gap) const
{
  const unsigned p = (_summary.indexHi == 0 ?
    _summary.index : (_summary.index + _summary.indexHi) / 2);

  // It's not a given that this is the right centering.
  // But we generally get the stragglers later.
  gap.lower = static_cast<unsigned>(p / SLIDING_UPPER) - 1;
  gap.upper = static_cast<unsigned>(p * SLIDING_UPPER) + 1;
  gap.count = static_cast<unsigned>(_summary.cumul);
  return true;
}


void PeakPiece::getCombinedGap(
  const PeakPiece& piece2,
  Gap& gap) const
{
  const unsigned p1 = (_summary.indexHi == 0 ?
    _summary.index : (_summary.index + _summary.indexHi) / 2);
  const unsigned p2 = (piece2._summary.indexHi == 0 ?
    piece2._summary.index : 
    (piece2._summary.index + piece2._summary.indexHi) / 2);

  const unsigned p = (p1 + p2) / 2;

  gap.lower = static_cast<unsigned>(p / SLIDING_UPPER) - 1;
  gap.upper = static_cast<unsigned>(p * SLIDING_UPPER) + 1;
  gap.count = static_cast<unsigned>(_summary.cumul);
}


string PeakPiece::strHeader() const
{
  stringstream ss;
  ss <<
    setw(6) << left << "extr" <<
    setw(6) << right << "index" <<
    setw(6) << "indHi" <<
    setw(6) << "pol" <<
    setw(6) << "cumul" << endl;
  return ss.str();
}


string PeakPiece::str() const
{
  stringstream ss;
  unsigned i = 0;
  for (auto& e: _extrema)
  {
    ss << setw(6) << left << i << 
      setw(6) << right << e.index <<
      setw(6) << (e.indexHi == 0 ? "-" : to_string(e.indexHi)) <<
      setw(6) << (e.direction == 1 ? "+" : "-") <<
      setw(6) << e.cumul << "\n";
    i++;
  }
  return ss.str();
}


