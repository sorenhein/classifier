/* 
   Makes a synthethic trace of peaks corresponding to a given train
   type.  Only the peak times and not the peak amplitudes are modeled
   realistically.  The peak times are subject to several disturbances
   controlled by five parameters:

   - noicePercent set the standard deviation of the (normally
     distributed) noise on the interval length between two ideal
     peaks.

   - injectUpTo sets the maximum number (uniformly distributed between
     0 and this number) of peaks that are randomly inserted anywhere
     in the trace.  "Anywhere includes up to +/- 2% beyond the
     actual length of the train.
   
   - deleteUpTo is analogous and sets the maximum number of peaks
     that are randomly deleted among the actual peaks.

   - pruneFrontUpTo sets the maximum number of actual leading 
     (consecutive) peaks that are randomly deleted at the front of
     a train.

   - pruneBackUpTo is similar for the back.
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <random>

#include "SynthTrain.h"
#include "read.h"


SynthTrain::SynthTrain()
{
  disturbance.noisePercent = 0.;
  disturbance.injectUpTo = 0;
  disturbance.deleteUpTo = 0;
  disturbance.pruneFrontUpTo = 0;
  disturbance.pruneBackUpTo = 0;
  srand(static_cast<unsigned>(time(NULL)));
}


SynthTrain::~SynthTrain()
{
}


void SynthTrain::readDisturbance(const string& fname)
{
  ifstream fin;
  fin.open(fname);
  string line;
  while (getline(fin, line))
  {
    if (line == "" || line.front() == '#')
      continue;

    const string err = "File " + fname + ": Bad line '" + line + "'";

    unsigned sp = line.find(" ");
    if (sp == string::npos || sp == 0 || sp == line.size()-1)
    {
      cout << err << endl;
      break;
    }

    const string& field = line.substr(0, sp);
    const string& rest = line.substr(sp+1);

    if (field == "NOISE_PERCENT")
    {
      if (! readFloat(rest, disturbance.noisePercent, err)) break;
    }
    else if (field == "INJECT_UP_TO")
    {
      if (! readInt(rest, disturbance.injectUpTo, err)) break;
    }
    else if (field == "DELETE_UP_TO")
    {
      if (! readInt(rest, disturbance.deleteUpTo, err)) break;
    }
    else if (field == "PRUNE_FRONT_UP_TO")
    {
      if (! readInt(rest, disturbance.pruneFrontUpTo, err)) break;
    }
    else if (field == "PRUNE_BACK_UP_TO")
    {
      if (! readInt(rest, disturbance.pruneBackUpTo, err)) break;
    }
    else
    {
      cout << err << endl;
    }
  }
}


void SynthTrain::makeNormalNoise(vector<Peak>& synthPeaks) const
{
  if (disturbance.noisePercent == 0.)
    return;

  random_device rd;
  mt19937 var(rd());
  normal_distribution<> dist(0, 1);
  
  for (unsigned i = 1; i < synthPeaks.size(); i++)
  {
    // The noise probably doesn't grow with the interval length,
    // but this is a convenient way to get noise in the right ballpark.

    const int diff = synthPeaks[i].sampleNo - synthPeaks[i-1].sampleNo;
    const float sdev = static_cast<float>(diff) *
      disturbance.noisePercent;

    const int delta = static_cast<int>(round(sdev * dist(var)));
    synthPeaks[i-1].sampleNo += delta;
  }
}


void SynthTrain::makeRandomInsertions(vector<Peak>& synthPeaks) const
{
  if (disturbance.injectUpTo == 0)
    return;

  const unsigned n =
    static_cast<unsigned>(rand() % disturbance.injectUpTo);

  for (unsigned i = 0; i < n; i++)
  {
    const unsigned hit = 
      static_cast<unsigned>(rand() % (synthPeaks.size()-1));
    
    const int tLeft = synthPeaks[hit].sampleNo;
    const float vLeft = synthPeaks[hit].value;

    const int tRight = synthPeaks[hit+1].sampleNo;
    const float vRight = synthPeaks[hit+1].value;

    Peak newPeak;
    newPeak.sampleNo = tLeft + 1 + (rand() % (tRight - tLeft - 2));
    newPeak.value = (vLeft + vRight) / 2.f;

    synthPeaks.insert(synthPeaks.begin()+1, newPeak);
  }
}


void SynthTrain::makeRandomDeletions(vector<Peak>& synthPeaks) const
{
  if (disturbance.deleteUpTo == 0)
    return;

  const unsigned n =
    static_cast<unsigned>(rand() % disturbance.deleteUpTo);

  for (unsigned i = 0; i < n; i++)
  {
    const unsigned hit = 
      static_cast<unsigned>(rand() % synthPeaks.size());
    
    synthPeaks.erase(synthPeaks.begin() + hit);
  }
}


void SynthTrain::makeRandomFrontDeletions(vector<Peak>& synthPeaks) const
{
  if (disturbance.pruneFrontUpTo == 0)
    return;

  const unsigned n =
    static_cast<unsigned>(rand() % disturbance.pruneFrontUpTo);
  
  if (n == 0)
    return;

  synthPeaks.erase(synthPeaks.begin(), synthPeaks.begin() + n);
}


void SynthTrain::makeRandomBackDeletions(vector<Peak>& synthPeaks) const
{
  if (disturbance.pruneBackUpTo == 0)
    return;

  const unsigned n =
    static_cast<unsigned>(rand() % disturbance.pruneBackUpTo);
  
  if (n == 0)
    return;

  synthPeaks.erase(synthPeaks.end() - n, synthPeaks.end());
}


void SynthTrain::scaleTrace(
   vector<Peak>& synthPeaks,
   const int origSpeed,
   const int newSpeed) const
{
  if (synthPeaks.size() == 0)
    return;

  const int offset = synthPeaks[0].sampleNo;
  const float factor = static_cast<float>(newSpeed) /
    static_cast<float>(origSpeed);

  for (unsigned i = 0; i < synthPeaks.size(); i++)
  {
    synthPeaks[i].sampleNo = offset +
      static_cast<int>((synthPeaks[i].sampleNo - offset) * factor);
  }
}

void printPeaks(const vector<Peak>& synthPeaks, const int level);
#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))

void SynthTrain::disturb(
  const vector<Peak>& perfectPeaks,
  vector<Peak>& synthPeaks,
  const int origSpeed,
  const int minSpeed,
  const int maxSpeed,
  int& newSpeed) const
{
  synthPeaks = perfectPeaks;
if (synthPeaks[0].sampleNo > synthPeaks[1].sampleNo)
  cout << "perfect ERROR" << endl;

  SynthTrain::makeNormalNoise(synthPeaks);
if (synthPeaks[0].sampleNo > synthPeaks[1].sampleNo)
  cout << "normal ERROR" << endl;
  SynthTrain::makeRandomInsertions(synthPeaks);
if (synthPeaks[0].sampleNo > synthPeaks[1].sampleNo)
  cout << "insert ERROR" << endl;
  SynthTrain::makeRandomDeletions(synthPeaks);
if (synthPeaks[0].sampleNo > synthPeaks[1].sampleNo)
  cout << "delete ERROR" << endl;
  SynthTrain::makeRandomFrontDeletions(synthPeaks);
if (synthPeaks[0].sampleNo > synthPeaks[1].sampleNo)
  cout << "front ERROR" << endl;
  SynthTrain::makeRandomBackDeletions(synthPeaks);
if (synthPeaks[0].sampleNo > synthPeaks[1].sampleNo)
  cout << "back ERROR" << endl;

  newSpeed = minSpeed + (rand() % (maxSpeed-minSpeed));

  UNUSED(origSpeed);
  // SynthTrain::scaleTrace(synthPeaks, origSpeed, newSpeed);
// cout << endl << "scale " << newSpeed << endl;
}

