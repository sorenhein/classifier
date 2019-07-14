#include <list>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <limits>

#include "PeakPtrs.h"
#include "Except.h"


struct WheelSpec
{
  BogieType bogie;
  WheelType wheel;
};

static const vector<WheelSpec> wheelSpecs =
{
  {BOGIE_LEFT, WHEEL_LEFT},
  {BOGIE_LEFT, WHEEL_RIGHT},
  {BOGIE_RIGHT, WHEEL_LEFT},
  {BOGIE_RIGHT, WHEEL_RIGHT}
};


const list<PeakFncPtr> wheelFncs =
{
  &Peak::isLeftWheel, &Peak::isRightWheel, 
  &Peak::isLeftWheel, &Peak::isRightWheel
};

const list<PeakFncPtr> bogieFncs =
{
  &Peak::isLeftBogie, &Peak::isLeftBogie, 
  &Peak::isRightBogie, &Peak::isRightBogie
};


PeakPtrs::PeakPtrs()
{
  PeakPtrs::clear();
}


PeakPtrs::~PeakPtrs()
{
}


void PeakPtrs::clear()
{
  peaks.clear();
}


void PeakPtrs::push_front(Peak * peak)
{
  peaks.push_front(peak);
}


void PeakPtrs::push_back(Peak * peak)
{
  peaks.push_back(peak);
}


void PeakPtrs::assign(
  const unsigned num,
  Peak * const peak)
{
  peaks.assign(num, peak);
}


bool PeakPtrs::add(Peak * peak)
{
  // TODO Pretty inefficient if we are adding multiple candidates.
  // If we need to do that, better add them all at once.
  const unsigned pindex = peak->getIndex();
  for (auto it = peaks.begin(); it != peaks.end(); it++)
  {
    if ((* it)->getIndex() == pindex)
      return true;

    if ((* it)->getIndex() > pindex)
    {
      peaks.insert(it, peak);
      return true;
    }
  }

  if (peaks.empty() || pindex > peaks.back()->getIndex())
  {
    peaks.push_back(peak);
    return true;
  }

  return false;
}


bool PeakPtrs::remove(Peak const * peak)
{
  const unsigned pindex = peak->getIndex();
  for (auto it = peaks.begin(); it != peaks.end(); it++)
  {
    const unsigned index = (* it)->getIndex();
    if (index == pindex)
    {
      peaks.erase(it);
      return true;
    }
    else if (index > pindex)
      return false;
  }
  return false;
}


void PeakPtrs::insert(
  PPLiterator& it,
  Peak * peak)
{
  peaks.insert(it, peak);
}


void PeakPtrs::moveFrom(PeakPtrs& fromPtrs)
{
  for (auto& fr: fromPtrs)
    peaks.push_back(fr);

  fromPtrs.clear();
}


void PeakPtrs::moveOut(
  const vector<Peak const *>& peaksClose,
  PeakPtrs& peakPtrsUnused)
{
  // Move those peaks that are not in peakClose to peakPtrsUnused.
  auto pu = peaks.begin();
  for (auto& peak: peaksClose)
  {
    if (peak == nullptr)
      continue;

    const unsigned indexClose = peak->getIndex();
    while (pu != peaks.end() && (* pu)->getIndex() < indexClose)
    {
      peakPtrsUnused.add(* pu);
      pu = PeakPtrs::erase(pu);
    }

    if (pu == peaks.end())
      break;

    // Preserve those peaks that are also in peaksClose.
    if ((* pu)->getIndex() == indexClose)
      pu = PeakPtrs::next(pu);
  }

  // Erase trailing peaks.
  while (pu != peaks.end())
  {
    peakPtrsUnused.add(* pu);
    pu = PeakPtrs::erase(pu);
  }
}


void PeakPtrs::shift_down(Peak * peak)
{
  if (peaks.size() > 0)
    peaks.pop_front();
  peaks.push_back(peak);
}


PPLiterator PeakPtrs::erase(PPLiterator& it)
{
  return peaks.erase(it);
}


void PeakPtrs::erase_below(const unsigned limit)
{
  for (PPLiterator it = peaks.begin(); it != peaks.end(); )
  {
    if ((* it)->getIndex() < limit)
      it = peaks.erase(it);
    else
      break;
  }
}


void PeakPtrs::erase_above(const unsigned limit)
{
  for (PPLiterator it = peaks.begin(); it != peaks.end(); it++)
  {
    if ((* it)->getIndex() > limit)
    {
      peaks.erase(it, peaks.end());
      return;
    }
  }
}


PPLiterator PeakPtrs::begin()
{
  return peaks.begin();
}


PPLiterator PeakPtrs::end()
{
  return peaks.end();
}


PPLciterator PeakPtrs::cbegin() const
{
  return peaks.cbegin();
}


PPLciterator PeakPtrs::cend() const
{
  return peaks.cend();
}


Peak const * PeakPtrs::front() const
{
  return peaks.front();
}


Peak const * PeakPtrs::back() const
{
  return peaks.back();
}


unsigned PeakPtrs::size() const
{
  return peaks.size();
}


unsigned PeakPtrs::count(const PeakFncPtr& fptr) const
{
  unsigned c = 0;
  for (auto peak: peaks)
  {
    if ((peak->* fptr)())
      c++;
  }
  return c;
}


bool PeakPtrs::empty() const
{
  // TODO This test not needed?
  if (peaks.empty())
    return true;

  for (auto peak: peaks)
  {
    if (peak != nullptr)
      return false;
  }

  return true;
}


unsigned PeakPtrs::firstIndex() const
{
  for (auto peak: peaks)
  {
    if (peak != nullptr)
      return peak->getIndex();
  }

  return numeric_limits<unsigned>::max();
}


unsigned PeakPtrs::lastIndex() const
{
  for (auto pit = peaks.rbegin(); pit != peaks.rend(); pit++)
  {
     if (* pit != nullptr)
       return (* pit)->getIndex();
  }

  return peaks.back()->getIndex();
}


void PeakPtrs::getIndices(vector<unsigned>& indices) const
{
  for (auto peak: peaks)
  {
    if (peak)
      indices.push_back(peak->getIndex());
    else
      indices.push_back(0);
  }
}


bool PeakPtrs::isFourWheeler(const bool perfectFlag) const
{
  if (peaks.size() != 4)
    return false;

  unsigned numWheels = 0, numBogies = 0;
  PPLciterator pit = peaks.cbegin();
  list<PeakFncPtr>::const_iterator wf = wheelFncs.begin();
  list<PeakFncPtr>::const_iterator bf = bogieFncs.begin();

  while (pit != peaks.cend())
  {
    if (((* pit)->** wf)())
      numWheels++;
    if (((* pit)->** bf)())
      numBogies++;
    pit++; 
    wf++; 
    bf++;
  }
  
  if (perfectFlag)
    return (numWheels == 4 && numBogies == 4);
  else
    return (numWheels == 4 && numBogies >= 2);
}


PPLiterator PeakPtrs::next(
  const PPLiterator& it,
  const PeakFncPtr& fptr,
  const bool exclFlag)
{
  if (it == peaks.end())
    return peaks.end();

  PPLiterator itNext = (exclFlag ? std::next(it) : it);
  while (itNext != peaks.end() && ! ((* itNext)->* fptr)())
    itNext++;

  return itNext;
}


PPLciterator PeakPtrs::next(
  const PPLciterator& it,
  const PeakFncPtr& fptr,
  const bool exclFlag) const
{
  if (it == peaks.end())
    return peaks.end();

  PPLciterator itNext = (exclFlag ? std::next(it) : it);
  while (itNext != peaks.end() && ! ((* itNext)->* fptr)())
    itNext++;

  return itNext;
}


PPLiterator PeakPtrs::prev(
  const PPLiterator& it,
  const PeakFncPtr& fptr,
  const bool exclFlag)
{
  // Return something that cannot be a useful iterator.
  if (it == peaks.begin())
    return peaks.end();

  PPLiterator itPrev = (exclFlag ? std::prev(it) : it);
  while (true)
  {
    if (((* itPrev)->* fptr)())
      return itPrev;
    else if (itPrev == peaks.begin())
      return peaks.end();
    else
      itPrev = prev(itPrev);
  }
}

PPLciterator PeakPtrs::prev(
  const PPLciterator& it,
  const PeakFncPtr& fptr,
  const bool exclFlag) const
{
  // Return something that cannot be a useful iterator.
  if (it == peaks.begin())
    return peaks.end();

  PPLciterator itPrev = (exclFlag ? std::prev(it) : it);
  while (true)
  {
    if (((* itPrev)->* fptr)())
      return itPrev;
    else if (itPrev == peaks.begin())
      return peaks.end();
    else
      itPrev = std::prev(itPrev);
  }
}


Peak * PeakPtrs::locate(
  const unsigned lower,
  const unsigned upper,
  const unsigned hint,
  const PeakFncPtr& fptr,
  unsigned& indexUsed) const
{
  unsigned num = 0;
  unsigned i = 0;
  unsigned dist = numeric_limits<unsigned>::max();
  Peak * ptr = nullptr;
  for (auto& peak: peaks)
  {
    if (! (peak->* fptr)())
    {
      i++;
      continue;
    }

    const unsigned index = peak->getIndex();
    if (index >= lower && index <= upper)
    {
      num++;
      const unsigned distNew = 
        (index >= hint ? index - hint : hint - index);

      if (distNew < dist)
      {
        dist = distNew;
        indexUsed = i;
        ptr = peak;
      }
    }
    i++;
  }

  if (num == 0)
    return nullptr;

  if (num > 1)
    cout << "WARNING locate: Multiple choices in (" <<
      lower << ", " << upper << ")\n";

  return ptr;
}


void PeakPtrs::getClosest(
  const vector<unsigned>& indices,
  vector<Peak *>& peaksClose,
  unsigned& numClose,
  unsigned& distance)
{
  const unsigned ni = indices.size();
  peaksClose.clear();
  peaksClose.resize(ni, nullptr);

  if (ni != 4)
    return;

  // Roughly half a bogie gap.
  unsigned yardstick;
  if (indices[0] == 0 || indices[1] == 0)
    yardstick = (indices[3] - indices[2]) / 2;
  else
    yardstick = (indices[1] - indices[0] + indices[3] - indices[2]) / 4;

  auto pit = peaks.begin();
  vector<unsigned> dist;
  dist.resize(ni, numeric_limits<unsigned>::max());

  for (unsigned i = 0; i < ni; i++)
  {
    const unsigned iindex = indices[i];
    unsigned pindex = 0, d;
    while (pit != peaks.end() && (pindex = (* pit)->getIndex()) < iindex)
    {
      d = iindex - pindex;
      if (d < yardstick && d < dist[i])
      {
        peaksClose[i] = * pit;
        dist[i] = d;
      }
      pit++;
    }

    if (pit == peaks.end())
      continue;

    d = pindex - iindex;
    if (d < yardstick && d < dist[i])
    {
      if (iindex > 0)
      {
        // iindex == 0 indicates a sentinel, not a real peak.
        peaksClose[i] = * pit;
        dist[i] = d;
      }
      pit++;
    }
  }

  // Eliminate doubles.
  for (unsigned i = 0; i+1 < ni; i++)
  {
    if (peaksClose[i] == peaksClose[i+1] && peaksClose[i] != nullptr)
    {
      if (dist[i] <= dist[i+1])
        peaksClose[i+1] = nullptr;
      else
        peaksClose[i] = nullptr;
    }
  }


  numClose = 0;
  distance = 0;
  for (unsigned i = 0; i < ni; i++)
  {
    if (peaksClose[i] == nullptr)
      continue;

    numClose++;
    distance += dist[i] * dist[i];
  }
}


void PeakPtrs::fill(
  const unsigned start,
  const unsigned end,
  PeakPtrs& ppl,
  list<PPLciterator>& pil)
{
  // Pick out peaks in [start, end].
  ppl.clear();
  pil.clear();
  for (PPLiterator it = peaks.begin(); it != peaks.end(); it++)
  {
    Peak * peak = * it;
    const unsigned index = peak->getIndex();
    if (index > end)
      break;
    else if (index < start)
      continue;
    else
    {
      ppl.push_back(peak);
      pil.push_back(it);
    }
  }
}


void PeakPtrs::split(
  const PeakFncPtr& fptr1,
  const PeakFncPtr& fptr2,
  PeakPtrs& peaksMatched,
  PeakPtrs& peaksRejected) const
{
  // Pick out peaks satisfying fptr1 or fptr2.
  for (auto peak: peaks)
  {
    if ((peak->* fptr1)() || (peak->* fptr2)())
      peaksMatched.push_back(peak);
    else
      peaksRejected.push_back(peak);
  }
}


void PeakPtrs::split(
  const vector<Peak const *>& flattened,
  PeakPtrs& peaksMatched,
  PeakPtrs& peaksRejected)
{
  // In a sense we could clear peaks and then push back each element
  // of flattened.  But due to the constness of flattened, we do it
  // in this way.
  
  peaksMatched.clear();
  peaksRejected.clear();

  PPLiterator pit = peaks.begin();
  vector<Peak const *>::const_iterator fit = flattened.cbegin();

  while (pit != peaks.end() || fit != flattened.cend())
  {
    if (fit == flattened.cend())
    {
      peaksRejected.push_back(* pit);
      pit++;
      continue;
    }
    else if (* fit == nullptr)
    {
      peaksMatched.push_back(nullptr);
      fit++;
      continue;
    }
    else if (pit == peaks.end())
    {
      cout << "PeakPtrs::split ERROR1\n";
      return;
    }

    const unsigned pindex = (* pit)->getIndex();
    const unsigned findex = (* fit)->getIndex();

    if (findex == pindex)
    {
      // Match.
      peaksMatched.push_back(* pit);
      pit++;
      fit++;
    }
    else if (findex > pindex)
    {
      peaksRejected.push_back(* pit);
      pit++;
    }
    else
    {
      cout << "PeakPtrs::split ERROR2\n";
      return;
    }
  }
}


void PeakPtrs::flattenCar(CarPeaksPtr& carPeaksPtr)
{
  if (peaks.size() > 4)
    return;

  auto it = peaks.begin();
  if (peaks.size() == 2)
  {
    carPeaksPtr.firstBogieLeftPtr = nullptr;
    carPeaksPtr.firstBogieRightPtr = nullptr;
    carPeaksPtr.secondBogieLeftPtr = * it++;
    carPeaksPtr.secondBogieRightPtr = * it++;
  }
  else if (peaks.size() == 3)
  {
    carPeaksPtr.firstBogieLeftPtr = nullptr;
    carPeaksPtr.firstBogieRightPtr = * it++;
    carPeaksPtr.secondBogieLeftPtr = * it++;
    carPeaksPtr.secondBogieRightPtr = * it++;
  }
  else if (peaks.size() == 4)
  {
    carPeaksPtr.firstBogieLeftPtr = * it++;
    carPeaksPtr.firstBogieRightPtr = * it++;
    carPeaksPtr.secondBogieLeftPtr = * it++;
    carPeaksPtr.secondBogieRightPtr = * it++;
  }
}


void PeakPtrs::flatten(vector<Peak const *>& flattened)
{
  flattened.clear();
  for (auto& peak: peaks)
    flattened.push_back(peak);
}


void PeakPtrs::makeDistances(
  const PeakFncPtr& fptr1, // true of left peak
  const PeakFncPtr& fptr2, // true of right peak
  const PeakPairFncPtr& pairPtr, // true of peak pair
  vector<unsigned>& distances) const
{
  PPLciterator npit;
  for (PPLciterator pit = peaks.cbegin(); pit != peaks.cend(); )
  {
    if (! ((* pit)->* fptr1)())
    {
      pit++;
      continue;
    }

    npit = PeakPtrs::next(pit, fptr2);
    if (npit == peaks.cend())
      return;

    if (! ((* pit)->* pairPtr)(** npit))
    {
      pit = npit;
      continue;
    }

    distances.push_back((* npit)->getIndex() - (* pit)->getIndex());
    pit = npit;
  }
}


void PeakPtrs::truncate(
  const unsigned limitLower,
  const unsigned limitUpper)
{
  if (limitLower)
    PeakPtrs::erase_below(limitLower);
  if (limitUpper)
    PeakPtrs::erase_above(limitUpper);
}


void PeakPtrs::apply(const PeakRunFncPtr& fptr)
{
  for (auto& peak: peaks)
    (peak->* fptr)();
}


void PeakPtrs::markup()
{
  // Can't use apply() for this, as we need method arguments.
  if (peaks.size() != 4)
    return;

  unsigned i = 0;
  for (auto& peak: peaks)
  {
    if (peak)
      peak->markBogieAndWheel(wheelSpecs[i].bogie, wheelSpecs[i].wheel);
    i++;
  }
}


void PeakPtrs::sort()
{
  peaks.sort([](Peak *& pp1, Peak *& pp2) -> bool
    {
      return pp1->getIndex() < pp2->getIndex();
    });
}


bool PeakPtrs::check() const
{
  for (auto& peak: peaks)
  {
    if (! peak->check())
    {
      cout << "PEAKPTRS ERROR\n";
      cout << peak->strQuality();
      return false;
    }
  }
  return true;
}


string PeakPtrs::strQuality(
  const string& text,
  const unsigned offset,
  const PeakFncPtr& fptr) const
{
  if (peaks.empty())
    return "";

  stringstream ss;
  if (text != "")
    ss << text << "\n";
  ss << peaks.front()->strHeaderQuality();

  for (const auto& peak: peaks)
  {
    if ((peak->* fptr)())
      ss << peak->strQuality(offset);
  }
  ss << endl;
  return ss.str();
}

