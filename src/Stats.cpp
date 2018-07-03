#include <iostream>
#include <iomanip>
#include <sstream>

#include "Stats.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


Stats::Stats()
{
  stats.clear();
}


Stats::~Stats()
{
}


void Stats::log(
  const string& trainReal,
  const string& trainClassified)
{
  UNUSED(trainReal);
  UNUSED(trainClassified);
}


void Stats::print(const string& fname) const
{
  UNUSED(fname);
}

