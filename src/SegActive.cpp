#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <math.h>

#include "SegActive.h"

#define G_FORCE 9.8f

#define SAMPLE_RATE 2000.

#define SEPARATOR ";"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))

SegActive::SegActive()
{
}


SegActive::~SegActive()
{
}


void SegActive::reset()
{
}


void SegActive::integrate(
  const vector<double>& samples,
  const Interval& aint,
  const double mean)
{
  UNUSED(mean);
  /*
  double sum = 0.;
  for (unsigned i = aint.first; i < aint.first + aint.len; i++)
    sum += samples[i];
  double avg = sum / aint.len;
  */
  double avg = 0.;

  // synthSpeed is then in 0.01 m/s.
  synthSpeed[aint.first - writeInterval.first] = 100.f * G_FORCE * 
    static_cast<float>((samples[aint.first] - avg) / SAMPLE_RATE);

  for (unsigned i = aint.first+1; i < aint.first + aint.len; i++)
    synthSpeed[i - writeInterval.first] = 
      synthSpeed[i - writeInterval.first - 1] + 
      100.f * G_FORCE * static_cast<float>((samples[i] - avg) / SAMPLE_RATE);
}


void SegActive::compensateSpeed()
{
  // The acceleration noise generates a random walk in the speed.
  // We attempt to correct for this with a rough lowpass filter.

  const unsigned filterWidth = 1001;
  const unsigned filterMid = (filterWidth-1) >> 1;

  const unsigned ls = synthSpeed.size();
  if (ls <= filterWidth)
  {
    cout << "CAN'T COMPENSATE\n";
    return;
  }

  vector<float> newSpeed(ls);

  float runningSum = 0.f;
  for (unsigned i = 0; i < filterWidth; i++)
    runningSum += synthSpeed[i];

  for (unsigned i = filterMid; i < ls - filterMid; i++)
  {
    newSpeed[i] = runningSum / filterWidth;
    if (i+1 < ls - filterMid)
      runningSum += synthSpeed[i + filterMid + 1] -
        synthSpeed[i - filterMid];
  }

  const float step0 = (newSpeed[filterMid] - synthSpeed[0]) /
    filterMid;

  for (unsigned i = 0; i < filterMid; i++)
    newSpeed[i] = synthSpeed[0] + step0 * i;

  const float step1 = 
    (synthSpeed[ls-1] - newSpeed[ls-filterMid-1]) / filterMid;

  for (unsigned i = ls - filterMid; i < ls; i++)
    newSpeed[i] = synthSpeed[ls-1] - step1 * (ls-1-i);
  
  for (unsigned i = 0; i < ls; i++)
    synthSpeed[i] -= newSpeed[i];
}


void SegActive::integrateFloat(const Interval& aint)
{
  /*
  float sum = 0.;
  for (unsigned i = aint.first; i < aint.first + aint.len; i++)
    sum += synthSpeed[i - writeInterval.first];
  float avg = sum / aint.len;
  */
  float avg = 0.f;

  // synthPos is then in tenth of a mm.
  synthPos[aint.first - writeInterval.first] = 
    100.f * (synthSpeed[aint.first - writeInterval.first] - avg) / 
    static_cast<float>(SAMPLE_RATE);

  for (unsigned i = aint.first + 1 - writeInterval.first; 
    i < aint.first + aint.len - writeInterval.first; i++)
    synthPos[i] = synthPos[i-1] + 
      100.f * (synthSpeed[i] - avg) / static_cast<float>(SAMPLE_RATE);
}


bool SegActive::detect(
  const vector<double>& samples,
  const vector<Interval>& active,
  const double mean)
{
/*
for (unsigned i = 0; i < samples.size(); i++)
  cout << i << ";" << samples[i] << "\n";
cout << "\n";
*/

  writeInterval.first = active.front().first;
  writeInterval.len = active.back().first + active.back().len - 
    writeInterval.first;

  synthSpeed.resize(writeInterval.len);
  synthPos.resize(writeInterval.len);
  for (unsigned i = 0; i < writeInterval.len; i++)
  {
    synthSpeed[i] = 0.f;
    synthPos[i] = 0.f;
  }

  for (const Interval& aint: active)
  {
    SegActive::integrate(samples, aint, mean);

    SegActive::compensateSpeed();

    SegActive::integrateFloat(aint);
  }

  float speedMin = 999.f;
  float speedMax = 0.f;
  for (unsigned i = 0; i < writeInterval.len; i++)
  {
    if (abs(synthSpeed[i]) > speedMax)
      speedMax = abs(synthSpeed[i]);
    if (abs(synthSpeed[i]) < speedMin)
      speedMin = abs(synthSpeed[i]);
  }
  cout << "Min " << speedMin << ", max " << speedMax << endl;

  float posMin = 999.f;
  float posMax = 0.f;
  for (unsigned i = 0; i < writeInterval.len; i++)
  {
    if (abs(synthPos[i]) > posMax)
      posMax = abs(synthPos[i]);
    if (abs(synthPos[i]) < posMin)
      posMin = abs(synthPos[i]);
  }
  cout << "Pos Min " << posMin << ", max " << posMax << endl;

  return true;
}


void SegActive::writeBinary(
  const string& origname,
  const string& speeddirname,
  const string& posdirname) const
{
  // Make the transient file name by:
  // * Replacing /raw/ with /dirname/
  // * Adding _offset_N before .dat

  // TODO Should probably come out of Seg*.cpp and go somewhere
  // central.

  if (synthSpeed.size() == 0)
    return;

  string sname = origname;
  string pname = origname;
  auto tp1 = sname.find("/raw/");
  if (tp1 == string::npos)
    return;

  auto tp2 = sname.find(".dat");
  if (tp2 == string::npos)
    return;

  sname.insert(tp2, "_offset_" + to_string(writeInterval.first));
  sname.replace(tp1, 5, "/" + speeddirname + "/");

  pname.insert(tp2, "_offset_" + to_string(writeInterval.first));
  pname.replace(tp1, 5, "/" + posdirname + "/");

  ofstream sout(sname, std::ios::out | std::ios::binary);

  sout.write(reinterpret_cast<const char *>(synthSpeed.data()),
    synthSpeed.size() * sizeof(float));
  sout.close();

  ofstream pout(pname, std::ios::out | std::ios::binary);

  pout.write(reinterpret_cast<const char *>(synthPos.data()),
    synthPos.size() * sizeof(float));
  pout.close();
}


string SegActive::headerCSV() const
{
  stringstream ss;

  return ss.str();
}


string SegActive::strCSV() const
{
  stringstream ss;
  
  return ss.str();
}

