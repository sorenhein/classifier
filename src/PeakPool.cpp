#include <iostream>
#include <iomanip>
#include <sstream>

#include "PeakPool.h"


PeakPool::PeakPool()
{
  PeakPool::clear();
}


PeakPool::~PeakPool()
{
}


void PeakPool::clear()
{
  peakLists.clear();
  peakLists.resize(1);
  peaks = &peakLists.front();
}


bool PeakPool::empty() const
{
  return peaks->empty();
}


unsigned PeakPool::size() const 
{
  return peaks->size();
}


Piterator PeakPool::begin() const
{
  return peaks->begin();
}


Pciterator PeakPool::cbegin() const
{
  return peaks->cbegin();
}


Piterator PeakPool::end() const
{
  return peaks->end();
}


Pciterator PeakPool::cend() const
{
  return peaks->cend();
}


Peak& PeakPool::front()
{
  return peaks->front();
}


Peak& PeakPool::back()
{
  return peaks->back();
}


void PeakPool::extend()
{
  peaks->emplace_back(Peak());
}


void PeakPool::copy()
{
  peakLists.emplace_back(PeakList());
  peakLists.back() = * peaks;
  peaks = &peakLists.back();
}


Piterator PeakPool::erase(
  Piterator pit1,
  Piterator pit2)
{
  return peaks->erase(pit1, pit2);
}


string PeakPool::str(
  const string& text,
  const unsigned& offset) const
{
  if (peaks->empty())
    return "";

  stringstream ss;
  if (text != "")
    ss << text << ": " << peaks->size() << "\n";
  ss << peaks->front().strHeader();

  for (auto& peak: * peaks)
    ss << peak.str(offset);
  ss << endl;
  return ss.str();
}


string PeakPool::strCounts() const
{
  stringstream ss;
  ss << 
    setw(8) << left << "Level" << 
    setw(6) << right << "Count" << endl;

  unsigned i = 0;
  for (auto& pl: peakLists)
  {
    ss << setw(8) << left << i++ <<
      setw(6) << right << pl.size() << endl;
  }
  ss << endl;
  return ss.str();
}

