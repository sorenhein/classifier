#include <iostream>
#include <iomanip>
#include <sstream>

#include "PeakPieces.h"
#include "Peak.h"
#include "Except.h"

#define SLIDING_LOWER 0.9f
#define SLIDING_UPPER 1.1f
#define SLIDING_UPPER_SQ (SLIDING_UPPER * SLIDING_UPPER)

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


PeakPieces::PeakPieces()
{
  PeakPieces::reset();
}


PeakPieces::~PeakPieces()
{
}


void PeakPieces::reset()
{
  steps.clear();
  pieces.clear();
}


void PeakPieces::makeSteps(const vector<unsigned>& distances)
{
  // We consider a sliding window with range +/- 10% relative to its
  // center.  We want to find the first maximum (i.e. the value at which
  // the count of distances in dists within the range reaches a local
  // maximum).  This is a somewhat rough way to find the lowest
  // "cluster" of values.

  // If an entry in dists is d, then it creates two DistEntry values.
  // One is at 0.9 * d and is a +1, and one is a -1 at 1.1 * d.

  steps.clear();
  for (auto d: distances)
  {
    steps.emplace_back(DistEntry());
    DistEntry& de1 = steps.back();
    de1.index = static_cast<unsigned>(d / SLIDING_UPPER);
    de1.direction = 1;
    de1.origin = d;

    steps.emplace_back(DistEntry());
    DistEntry& de2 = steps.back();
    de2.index = static_cast<unsigned>(d * SLIDING_UPPER);
    de2.direction = -1;
    de2.origin = d;
  }
}


void PeakPieces::collapseSteps()
{
  if (steps.empty())
    return;

  steps.sort();

  // Collapse those steps that have the same index.
  int cumul = 0;
  for (auto sit = steps.begin(); sit != steps.end(); )
  {
    sit->count = 0;
    auto nsit = sit;
    do
    {
      sit->count += nsit->direction;
      nsit++;
    }
    while (nsit != steps.end() && nsit->index == sit->index);

    cumul += sit->count;
    sit->cumul = cumul;

    // Skip the occasional zero count as well (canceling out).
    if (sit->count)
      sit++;

    if (sit != nsit) 
      sit = steps.erase(sit, nsit);
  }
}


void PeakPieces::makePieces()
{
  // Segment the steps into pieces where the cumulative value reaches
  // all the way down to zero.  Split each piece into extrema.
  
  pieces.clear();
  summary.cumul = 0;

  for (auto sit = steps.begin(); sit != steps.end(); )
  {
    auto nsit = sit;
    do
    {
      nsit++;
    }
    while (nsit != steps.end() && nsit->cumul != 0);

    // We've got a piece at [sit, nsit).
    pieces.emplace_back(PeakPiece());
    PeakPiece& pe = pieces.back();
    pe.reset();

    for (auto it = sit; it != nsit; it++)
    {
      const bool leftUp = 
        (it == sit ? true : (it->cumul > prev(it)->cumul));
      const bool rightUp = 
        (next(it) == sit ? false : (it->cumul < next(it)->cumul));

      if (leftUp == rightUp)
        continue;

      pe.logExtremum(it->index, it->cumul, (leftUp ? 1 : -1));
    }

    pe.summarize();

    if (pe > summary.cumul)
    {
      summary.index = pe.summary().index;
      summary.cumul = pe.summary().cumul;
    }

    if (nsit == steps.end())
      break;
    else
      sit = next(nsit);
  }
}


void PeakPieces::eraseSmallPieces()
{
  const int limit = static_cast<int>(0.25f * summary.cumul);

  for (auto pit = pieces.begin(); pit != pieces.end(); )
  {
    if (* pit <= limit)
      pit = pieces.erase(pit);
    else
      pit++;
  }
}


void PeakPieces::eraseSmallMaxima()
{
  const int limit = static_cast<int>(0.25f * summary.cumul);

  for (auto pit = pieces.begin(); pit != pieces.end(); pit++)
    pit->eraseSmallMaxima(limit);
}


void PeakPieces::split()
{
  unsigned indexLeft;
  unsigned indexRight;
  for (auto pit = pieces.begin(); pit != pieces.end(); )
  {
    if (pit->modality() == 1)
    {
      pit++;
      continue;
    }

    if (pit->splittableOnDip(indexLeft, indexRight))
    {
      // Copy the entry to begin with.  newpit precedes pit now.
      auto newpit = pieces.emplace(pit, * pit);
      newpit->splitOnIndices(indexLeft, indexRight, * pit);
      continue;
    }

    if (pit->splittableOnGap(indexLeft, indexRight))
    {
      auto newpit = pieces.emplace(pit, * pit);
      newpit->splitOnIndices(indexLeft, indexRight, * pit);
      continue;
    }

    pit++;
  }
}


void PeakPieces::unjitter()
{
  for (auto& piece: pieces)
    piece.unjitter();
}


void PeakPieces::make(const vector<unsigned>& distances)
{
  PeakPieces::makeSteps(distances);

  PeakPieces::collapseSteps();

  PeakPieces::makePieces();

  PeakPieces::eraseSmallPieces();

  PeakPieces::eraseSmallMaxima();

  PeakPieces::split();

  PeakPieces::unjitter();
}


bool PeakPieces::empty() const
{
  return pieces.empty();
}


void PeakPieces::guessBogieGap(Gap& wheelGap) const
{
  if (pieces.size() == 1)
  {
    pieces.front().getGap(wheelGap);
    return;
  }

  const PeakPiece& piece1 = pieces.front();
  const PeakPiece& piece2 = * next(pieces.begin());

  if (piece2.summary().cumul >= 3 * piece1.summary().cumul / 2 &&
      piece2.summary().index <= 2 * piece2.summary().index)
  {
    // Assume that the first piece is spurious.
    piece2.getGap(wheelGap);
  }
  else
    piece1.getGap(wheelGap);
}


bool PeakPieces::extendBogieGap(Gap& wheelGap) const
{
  if (pieces.empty())
    THROW(ERR_NO_PEAKS, "Piece list for bogie extension is empty");

  const unsigned wlo = static_cast<unsigned>(wheelGap.lower / 1.1f);
  const unsigned whi = static_cast<unsigned>
    (1.5f * (wheelGap.lower + wheelGap.upper) / 2.);

  unsigned i = 0;
  bool ret = false;
  for (auto pit = pieces.begin(); pit != pieces.end(); pit++, i++)
  {
    if (pit->summary().index < wlo)
    {
      // Most likely a small piece that was rejected relative to the
      // largest piece (bogie gaps), but now this large piece is gone.
      cout << "markShortGaps: Skipping an initial low piece\n";
      cout << "wheelGap count " << wheelGap.count << endl;
      cout << "skip count: " << pit->summary().cumul << endl;
    }
    else if (i+3 <= pieces.size() && pit->extend(wheelGap))
    {
      // Only regrade if there are pieces left for short and long gaps.
      cout << "Extending wheelGap\n";
      ret = true;
    }
    else if (i+3 <= pieces.size() &&
        pit->summary().index < whi &&
        pit->summary().cumul <= static_cast<int>(wheelGap.count / 4))
    {
      // Most likely a small piece with valid bogie differences for
      // short cars.
      // TODO Exploit this as well.
      cout << "markShortGaps: Skipping an initial high piece\n";
      cout << "wheelGap count " << wheelGap.count << endl;
      cout << "skip count: " << pit->summary().cumul << endl;
    }
  }
  return ret;
}


void PeakPieces::guessShortGap(
  Gap& wheelGap,
  Gap& shortGap,
  bool& wheelGapNewFlag) const
{
  PeakPiece const * pptr = nullptr;
  Gap actualGap;
  const unsigned wlo = static_cast<unsigned>(wheelGap.lower / 1.1f);
  const unsigned whi = static_cast<unsigned>
    (1.5f * (wheelGap.lower + wheelGap.upper) / 2.);

  if (pieces.empty())
    THROW(ERR_NO_PEAKS, "Piece list for short gaps is empty");

  wheelGapNewFlag = false;
  unsigned i = 0;
  for (auto pit = pieces.begin(); pit != pieces.end(); pit++, i++)
  {
    if (pit->summary().index < wlo)
    {
      // Most likely a small piece that was rejected relative to the
      // largest piece (bogie gaps), but now this large piece is gone.
      cout << "markShortGaps: Skipping an initial low piece\n";
      cout << "wheelGap count " << wheelGap.count << endl;
      cout << "skip count: " << pit->summary().cumul << endl;
    }
    else if (i+3 <= pieces.size() && pit->extend(wheelGap))
    {
      // Only regrade if there are pieces left for short and long gaps.
      cout << "Extending wheelGap\n";
      wheelGapNewFlag = true;
    }
    else if (i+3 <= pieces.size() &&
        pit->summary().index < whi &&
        pit->summary().cumul <= static_cast<int>(wheelGap.count / 4))
    {
      // Most likely a small piece with valid bogie differences for
      // short cars.
      // TODO Exploit this as well.
      cout << "markShortGaps: Skipping an initial high piece\n";
      cout << "wheelGap count " << wheelGap.count << endl;
      cout << "skip count: " << pit->summary().cumul << endl;
    }
    else
    {
      pptr = &* pit;
      break;
    }
  }

  if (pptr == nullptr)
    THROW(ERR_NO_PEAKS, "Piece list has no short gaps");

  cout << PeakPieces::str("For short gaps");

  pptr->getGap(shortGap);
}


void PeakPieces::guessLongGap(
  Gap& wheelGap,
  Gap& shortGap,
  Gap& longGap) const
{
  UNUSED(wheelGap);
  UNUSED(shortGap);
  UNUSED(longGap);
}


string PeakPieces::str(const string& text) const
{
  stringstream ss;
  ss << text << "\n";
  for (auto& p: pieces)
    cout << p.str();
  cout << endl;
  return ss.str();
}


string PeakPieces::strModality(const string& text) const
{
  stringstream ss;
  ss << text << " ";
  for (auto& p: pieces)
    cout << p.modality();
  cout << endl;
  return ss.str();
}

