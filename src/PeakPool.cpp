#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>

#include "PeakPool.h"
#include "Except.h"


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


Piterator PeakPool::begin()
{
  return peaks->begin();
}


Piterator PeakPool::end()
{
  return peaks->end();
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


