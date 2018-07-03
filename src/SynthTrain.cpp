#include <iostream>
#include <iomanip>
#include <sstream>

#include "SynthTrain.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


SynthTrain::SynthTrain()
{
}


SynthTrain::~SynthTrain()
{
}


void SynthTrain::readDisturbance(const string& fname)
{
  UNUSED(fname);
}


void SynthTrain::disturb(
  const vector<Peak>& perfectPeaks,
  vector<Peak>& synthPeaks,
  const int origSpeed,
  int& newSpeed) const
{
  UNUSED(perfectPeaks);
  UNUSED(synthPeaks);
  UNUSED(origSpeed);
  UNUSED(newSpeed);
}

