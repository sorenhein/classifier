#include <list>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "PeakPtrs.h"
#include "Except.h"


struct WheelSpec
{
  BogeyType bogey;
  WheelType wheel;
};

static const vector<WheelSpec> wheelSpecs =
{
  {BOGEY_LEFT, WHEEL_LEFT},
  {BOGEY_LEFT, WHEEL_RIGHT},
  {BOGEY_RIGHT, WHEEL_LEFT},
  {BOGEY_RIGHT, WHEEL_RIGHT}
};


const list<PeakFncPtr> wheelFncs =
{
  &Peak::isLeftWheel, &Peak::isRightWheel, 
  &Peak::isLeftWheel, &Peak::isRightWheel
};

const list<PeakFncPtr> bogeyFncs =
{
  &Peak::isLeftBogey, &Peak::isLeftBogey, 
  &Peak::isRightBogey, &Peak::isRightBogey
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
    if ((* it)->getIndex() > pindex)
    {
      peaks.insert(it, peak);
      return true;
    }
  }
  return false;
}


void PeakPtrs::insert(
  PPLiterator& it,
  Peak * peak)
{
  peaks.insert(it, peak);
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
  return peaks.empty();
}


unsigned PeakPtrs::firstIndex() const
{
  if (peaks.empty())
    return numeric_limits<unsigned>::max();
  else
    return peaks.front()->getIndex();
}


unsigned PeakPtrs::lastIndex() const
{
  if (peaks.empty())
    return numeric_limits<unsigned>::max();
  else
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

  unsigned numWheels = 0, numBogeys = 0;
  PPLciterator pit = peaks.cbegin();
  list<PeakFncPtr>::const_iterator wf = wheelFncs.begin();
  list<PeakFncPtr>::const_iterator bf = bogeyFncs.begin();

  while (pit != peaks.cend())
  {
    if (((* pit)->** wf)())
      numWheels++;
    if (((* pit)->** bf)())
      numBogeys++;
    pit++; 
    wf++; 
    bf++;
  }
  
  if (perfectFlag)
    return (numWheels == 4 && numBogeys == 4);
  else
    return (numWheels == 4 && numBogeys >= 2);
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


Peak const * PeakPtrs::locate(
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
      if ((index >= hint && index - hint < dist) ||
          (index < hint && hint - index < dist))
      {
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
    if (* fit == nullptr)
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
    else if (fit == flattened.cend())
    {
      peaksRejected.push_back(* pit);
      pit++;
      continue;
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
    carPeaksPtr.firstBogeyLeftPtr = nullptr;
    carPeaksPtr.firstBogeyRightPtr = nullptr;
    carPeaksPtr.secondBogeyLeftPtr = * it++;
    carPeaksPtr.secondBogeyRightPtr = * it++;
  }
  else if (peaks.size() == 3)
  {
    carPeaksPtr.firstBogeyLeftPtr = nullptr;
    carPeaksPtr.firstBogeyRightPtr = * it++;
    carPeaksPtr.secondBogeyLeftPtr = * it++;
    carPeaksPtr.secondBogeyRightPtr = * it++;
  }
  else if (peaks.size() == 4)
  {
    carPeaksPtr.firstBogeyLeftPtr = * it++;
    carPeaksPtr.firstBogeyRightPtr = * it++;
    carPeaksPtr.secondBogeyLeftPtr = * it++;
    carPeaksPtr.secondBogeyRightPtr = * it++;
  }
}


void PeakPtrs::flatten(vector<Peak const *>& flattened)
{
  flattened.clear();
  for (auto& peak: peaks)
    flattened.push_back(peak);
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
      peak->markBogeyAndWheel(wheelSpecs[i].bogey, wheelSpecs[i].wheel);
    i++;
  }
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

