#include <iostream>
#include <iomanip>
#include <sstream>

#include "PeakPieces.h"
#include "Peak.h"
#include "util/Gap.h"
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

    if (pe > static_cast<unsigned>(summary.cumul))
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


void PeakPieces::eraseSmallPieces(const unsigned smallPieceLimit)
{
  const unsigned basis = 
    (smallPieceLimit > 0 ? smallPieceLimit : 
    static_cast<unsigned>(summary.cumul));

  const unsigned limit = static_cast<unsigned>(0.25f * basis);

  for (auto pit = pieces.begin(); pit != pieces.end(); )
  {
    if (* pit <= limit)
      pit = pieces.erase(pit);
    else
      pit++;
  } }


void PeakPieces::eraseSmallMaxima()
{
  const unsigned limit = static_cast<unsigned>(0.25f * summary.cumul);

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


void PeakPieces::make(
  const vector<unsigned>& distances,
  const unsigned smallPieceLimit)
{
  PeakPieces::makeSteps(distances);

  PeakPieces::collapseSteps();

  PeakPieces::makePieces();

  PeakPieces::split();

  PeakPieces::eraseSmallMaxima();

  PeakPieces::eraseSmallPieces(smallPieceLimit);

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

  if (pieces.size() >= 4 &&
      piece2.summary().cumul >= 3 * piece1.summary().cumul / 2 &&
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

  const unsigned whi = static_cast<unsigned>(1.5f * wheelGap.center());

  unsigned i = 0;
  bool ret = false;

  for (auto pit = pieces.begin(); pit != pieces.end(); pit++, i++)
  {
    if (wheelGap.wellAbove(pit->summary().index))
    {
      // Most likely a small piece that was rejected relative to the
      // largest piece (bogie gaps), but now this large piece is gone.
      cout << "markShortGaps: Skipping an initial low piece\n";
      cout << "wheelGap count " << wheelGap.count() << endl;
      cout << "skip count: " << pit->summary().cumul << endl;
    }
    else if (i+3 <= pieces.size() && 
      wheelGap.extend(pit->summary().index))
    {
      // Only regrade if there are pieces left for short and long gaps.
      cout << "Extending wheelGap\n";
      ret = true;
    }
    else if (i+3 <= pieces.size() &&
        pit->summary().index < whi &&
        pit->summary().cumul <= static_cast<int>(wheelGap.count() / 4))
    {
      // Most likely a small piece with valid bogie differences for
      // short cars.
      // TODO Exploit this as well.
      cout << "markShortGaps: Skipping an initial high piece\n";
      cout << "wheelGap count " << wheelGap.count() << endl;
      cout << "skip count: " << pit->summary().cumul << endl;
    }
  }

  return ret;
}


bool PeakPieces::selectShortAmongTwo(
  const PeakPiece& piece1,
  const PeakPiece& piece2,
  const Gap& wheelGap,
  Gap& shortGap) const
{
  if (piece1.apart(piece2, 2.0f))
  {
    // piece2 is probably the long gap.
    piece1.getGap(shortGap);
  }
  else if (piece1.oftener(piece2, 2.0f))
  {
    // piece1 is much more frequent.
    piece1.getGap(shortGap);
  }
  else if (piece1.oftener(wheelGap, 0.4f))
  {
    // piece1 has enough count to be credible on its own.
    piece1.getGap(shortGap);
  }
  else if (piece2.oftener(piece1, 2.0f))
  {
    // piece2 is much more frequent.
    piece2.getGap(shortGap);
  }
  else if (! piece1.apart(piece2, 1.5f))
  {
    // Reasonably close, so use both.
    piece1.getCombinedGap(piece2, shortGap);
  }
  else if (piece1.oftener(piece2, 1.5f))
  {
    // piece1 is somewhat separated in both index and count.
    piece1.getGap(shortGap);
  }
  else if (piece2.oftener(piece1, 1.5f))
  {
    // piece2 is somewhat separated in both index and count.
    piece2.getGap(shortGap);
  }
  else
  {
    // Just pick the shorter one.
    piece1.getGap(shortGap);
  }
  return true;
}


bool PeakPieces::selectShortAmongThree(
  const PeakPiece& piece1,
  const PeakPiece& piece2,
  const PeakPiece& piece3,
  const Gap& wheelGap,
  Gap& shortGap) const
{
  if (piece2.apart(piece3, 2.0f))
  {
    // piece3 is probably the long piece.
    return PeakPieces::selectShortAmongTwo(piece1, piece2, 
     wheelGap, shortGap);
  }
  else if (piece1.oftener(piece2, 1.5f) &&
      piece1.apart(piece3, 2.0f))
  {
    // piece2 can probably be ignored for now, and
    // piece3 is the long piece after all.
    return PeakPieces::selectShortAmongTwo(piece1, piece3, 
      wheelGap, shortGap);
  }
  else if (piece2.oftener(piece3, 2.0f))
  {
    // price3 is probably not really the long piece.
    return PeakPieces::selectShortAmongTwo(piece1, piece2, 
      wheelGap, shortGap);
  }
  else
  {
    // Just pick the shorter one.
    piece1.getGap(shortGap);
  }
  return true;
}


void PeakPieces::guessShortGap(
  const Gap& wheelGap,
  Gap& shortGap) const
{
  const unsigned lp = pieces.size();

  // Skip over any early pieces that are way too short.
  auto pit = pieces.begin();
  unsigned c = 0;
  while (pit != pieces.end() && ! wheelGap.wellBelow(pit->summary().index))
  {
    pit++;
    c++;
  }

  if (c == lp)
    THROW(ERR_NO_PEAKS, "No long-enough short pieces");

  if (c+1 == lp)
  {
    // A single piece, so it will have to do.
    pieces.front().getGap(shortGap);
    return;
  }

  if (c+2 == lp)
  {
    if (PeakPieces::selectShortAmongTwo(* pit, * next(pit), 
        wheelGap, shortGap))
      return;
    else
    {
      cout << PeakPieces::str("ERROR2");
      cout << "wheel gap " << wheelGap.str() << "\n\n";
      THROW(ERR_NO_PEAKS, "Confusing short pieces among two");
    }
  }

  if (c+3 <= lp)
  {
    // Choose among the first three.
    const PeakPiece& piece1 = * pit;
    const PeakPiece& piece2 = * next(pit);
    const PeakPiece& piece3 = * next(next(pit));
    if (PeakPieces::selectShortAmongThree(piece1, piece2, piece3, 
        wheelGap, shortGap))
      return;
    else
    {
      cout << PeakPieces::str("ERROR");
      cout << "wheel gap " << wheelGap.str() << "\n\n";
      THROW(ERR_NO_PEAKS, "Confusing short pieces among three");
    }
  }
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


bool PeakPieces::guessGaps(
  const Gap& wheelGap,
  Gap& shortGap,
  Gap& longGap) const
{
  const unsigned lp = pieces.size();

  cout << "Wheel gap " << wheelGap.str() << "\n";

  cout << PeakPieces::str("Starting point");

  if (lp == 1)
  {
    const PeakPiece& piece = pieces.front();
    const float lenRatio = wheelGap.ratio(piece.summary().index);

    if (lenRatio >= 3. && lenRatio <= 7. &&
        2 * piece.summary().cumul >= static_cast<int>(wheelGap.count()))
    {
      shortGap.reset();
      piece.getGap(longGap);
      return true;
    }
    else
    {
      cout << "ODDNUM " << lp << "\n";
      cout << "NOT-THREE " << fixed << setprecision(2) << lenRatio << "\n";
      cout << piece.summary().cumul << " vs " << wheelGap.count() << endl;
    }

  }
  else if (lp == 2)
  {
    // Hope they are the two gaps.
    // TODO Check
    auto pit = pieces.begin();
    const PeakPiece& piece0 = * pit; pit++;
    const PeakPiece& piece1 = * pit;
    piece0.getGap(shortGap);
    piece1.getGap(longGap);
    return true;
  }
  else if (lp == 3)
  {
    auto pit = pieces.begin();
    const PeakPiece& piece0 = * pit; pit++;
    const PeakPiece& piece1 = * pit; pit++;
    const PeakPiece& piece2 = * pit;

    if (wheelGap.startsBelow(piece0.summary().index) && 
        piece0.summary().index + piece1.summary().index <
        piece2.summary().index)
    {
      // Hope the first two ones are the gaps.
      // TODO Check
      piece0.getGap(shortGap);
      piece1.getGap(longGap);
      return true;
    }
    else
      cout << "ODDNUM " << lp << "\n";
  }
  else if (lp == 4)
  {
    auto pit = pieces.begin();
    const PeakPiece& piece0 = * pit; pit++;
    const PeakPiece& piece1 = * pit; pit++;
    const PeakPiece& piece2 = * pit;

    if (wheelGap.startsBelow(piece0.summary().index) && 
        piece0.summary().index + piece1.summary().index <
        piece2.summary().index)
    {
      // Hope the first two ones are the gaps.
      // TODO Check
      piece0.getGap(shortGap);
      piece1.getGap(longGap);
      return true;
    }
    else
      cout << "ODDNUM " << lp << "\n";
  }
  else
    cout << "ODDNUM " << lp << "\n";

  return false;
}


string PeakPieces::str(const string& text) const
{
  stringstream ss;
  ss << text << "\n";
  if (pieces.empty())
    return ss.str();

  ss << pieces.front().strHeader();
  for (auto& p: pieces)
    ss << p.str();
  ss << endl;
  return ss.str();
}


string PeakPieces::strModality(const string& text) const
{
  stringstream ss;
  ss << text << " ";
  for (auto& p: pieces)
    ss << p.modality();
  ss << endl;
  return ss.str();
}

