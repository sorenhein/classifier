#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "Stats.h"

#define SPEED_STEP 1. // m
#define SPEED_MAX_INDEX 100

#define ACCEL_STEP 0.05 // m/s^2
#define ACCEL_MAX 1.5 // m/s^2
#define ACCEL_MAX_INDEX 60


Stats::Stats()
{
  speedMap.resize(SPEED_MAX_INDEX);
  accelMap.resize(ACCEL_MAX_INDEX);
}


Stats::~Stats()
{
}


void Stats::log(
  const string& trainActual,
  const vector<double>& motionActual,
  const string& trainEstimate,
  const vector<double>& motionEstimate,
  const double residuals)
{
  statCross.log(trainActual, trainEstimate);

  if (trainActual != trainEstimate)
    return;

  if (motionActual[1] < 0.)
  {
    cout << "Negative speed\n";
    return;
  }

  const unsigned speedIndex = 
    static_cast<unsigned>(motionActual[1] / SPEED_STEP);
  if (speedIndex >= SPEED_MAX_INDEX)
  {
    cout << "Speed " << motionEstimate[1]  << " too high\n";
    return;
  }

  if (motionActual[2] < -ACCEL_MAX)
  {
    cout << "Too negative acceleration\n";
    return;
  }

  const unsigned accelIndex = 
    static_cast<unsigned>((motionActual[2] + ACCEL_MAX) / ACCEL_STEP);
  if (accelIndex >= ACCEL_MAX_INDEX)
  {
    cout << "Acceleration " << motionEstimate[1]  << " too high\n";
    return;
  }

  trainMap[trainActual].log(motionActual, motionEstimate, residuals);
  speedMap[speedIndex].log(motionActual, motionEstimate, residuals);
  accelMap[accelIndex].log(motionActual, motionEstimate, residuals);

  auto its = trainSpeedMap.find(trainActual);
  if (its == trainSpeedMap.end())
    trainSpeedMap[trainActual].resize(SPEED_MAX_INDEX);
  trainSpeedMap[trainActual][speedIndex].log
    (motionActual, motionEstimate, residuals);

  auto ita = trainAccelMap.find(trainActual);
  if (ita == trainAccelMap.end())
    trainAccelMap[trainActual].resize(SPEED_MAX_INDEX);
  trainAccelMap[trainActual][accelIndex].log
    (motionActual, motionEstimate, residuals);
}


void Stats::printCrossCountCSV(const string& fname) const
{
  statCross.printCountCSV(fname);
}


void Stats::printCrossPercentCSV(const string& fname) const
{
  statCross.printPercentCSV(fname);
}


void Stats::printSpeed(
  ofstream& fout,
  const vector<StatEntry>& sMap) const
{
  for (unsigned i = 0; i < sMap.size(); i++)
  {
    const double speed = i * SPEED_STEP;
    stringstream ss;
    ss << fixed << setprecision(0) << speed << "m/s " <<
      3.6 * speed << "km/h";

    sMap[i].print(fout, ss.str());
  }
  fout << "\n";
}


void Stats::printAccel(
  ofstream& fout,
  const vector<StatEntry>& aMap) const
{
  for (unsigned i = 0; i < aMap.size(); i++)
  {
    const double accel = i * ACCEL_STEP - ACCEL_MAX;
    stringstream ss;
    ss << fixed << setprecision(2) << accel << "m/s^2";

    aMap[i].print(fout, ss.str());
  }
  fout << "\n";
}


void Stats::printOverviewCSV(const string& fname) const
{
  ofstream fout;
  fout.open(fname);

  // To have something.
  auto it = trainMap.begin();

  it->second.printHeader(fout, "Train name");
  for (auto& itt: trainMap)
  {
    const string& trainName = itt.first;
    itt.second.print(fout, trainName);
  }
  fout << "\n";

  it->second.printHeader(fout, "Speed");
  Stats::printSpeed(fout, speedMap);

  it->second.printHeader(fout, "Acceleration");
  Stats::printAccel(fout, accelMap);

  fout.close();
}


void Stats::printDetailsCSV(const string& fname) const
{
  ofstream fout;
  fout.open(fname);

  // To have something.
  auto it = trainMap.begin();

  for (auto& its: trainSpeedMap)
  {
    const string& trainName = its.first;
    it->second.printHeader(fout, trainName);
    Stats::printSpeed(fout, its.second);
    fout << "\n";
  }

  for (auto& ita: trainAccelMap)
  {
    const string& trainName = ita.first;
    it->second.printHeader(fout, trainName);
    Stats::printAccel(fout, ita.second);
    fout << "\n";
  }

  fout.close();
}

