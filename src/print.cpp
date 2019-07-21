#include <iostream>
#include <iomanip>
#include <string>

#include "print.h"
#include "const.h"

using namespace std;


void printDataHeader();

void printDataLine(
  const string& text,
  const double actual,
  const double estimate);



void printPeakPosCSV(
  const vector<double>& peaks,
  const int level)
{
  for (unsigned i = 0; i < peaks.size(); i++)
    cout << fixed << setprecision(4) << 
      peaks[i] << ";" << level << endl;
  cout << endl;
}


void printPeakTimeCSV(
  const vector<PeakTime>& peaks,
  const int level)
{
  for (unsigned i = 0; i < peaks.size(); i++)
    cout << fixed << setprecision(4) <<
      peaks[i].time << ";" << level << endl;
  cout << endl;
}


void printDataHeader()
{
  cout << setw(8) << left << "" <<
    setw(10) << right << "Actual" <<
    setw(10) << right << "Est" <<
    setw(10) << right << "Error" << endl;
}


void printDataLine(
  const string& text,
  const double actual,
  const double estimate)
{
  double dev;
  if (actual == 0.)
    dev = 0.;
  else
  {
    const double f = (actual - estimate) / actual;
    dev = 100. * (f >= 0. ? f : -f);
  }

  cout << setw(8) << left << text <<
    setw(10) << right << fixed << setprecision(2) << actual <<
    setw(10) << right << fixed << setprecision(2) << estimate <<
    setw(9) << right << fixed << setprecision(1) << dev << "%" << endl;
}


void printCorrelation(
  const vector<double> actual,
  const vector<double> estimate)
{
  printDataHeader();
  printDataLine("Offset", actual[0], estimate[0]);
  printDataLine("Speed", actual[1], estimate[1]);
  printDataLine("Accel", actual[2], estimate[2]);
  cout << "\n";
}


void printAlignment(
  const Alignment& match,
  const string& trainName)
{
  cout << setw(24) << left << trainName <<
    setw(10) << right << fixed << setprecision(2) << match.dist <<
    setw(10) << right << fixed << setprecision(2) << match.distMatch <<
    setw(8) << match.numAdd <<
    setw(8) << match.numDelete << endl;
}


void printMotion(const vector<double>& motionEstimate)
{
  cout << 
    setw(16) << left << "Speed" <<
    setw(8) << right << fixed << setprecision(2) << 
      motionEstimate[1] << " m/s   = " <<
    setw(8) << MS_TO_KMH * motionEstimate[1] << " km/h\n";

  cout << 
    setw(16) << left << "Acceleration" <<
    setw(8) << right << fixed << setprecision(2) << 
      motionEstimate[2] << " m/s^2 = " <<
    setw(8) << motionEstimate[2] / G_FORCE << " g\n";
  
  cout << endl;
}


void printMatches(
  const TrainDB& trainDB,
  const vector<Alignment>& matches)
{
  cout << "Matching alignment\n";
  for (auto& match: matches)
    printAlignment(match, trainDB.lookupName(match.trainNo));
  cout << "\n";
}


void printTopResiduals(const Alignment& match)
{
  if (match.topResiduals.empty())
    return;

  cout << "Top residuals" << endl;

  cout << setw(6) << "index" <<
    setw(8) << "value" <<
    setw(8) << "valSq" <<
    setw(8) << "frac" << endl;

  for (auto& r: match.topResiduals)
  {
    cout << setw(6) << r.index <<
      setw(8) << fixed << setprecision(2) << r.value <<
      setw(8) << fixed << setprecision(2) << r.valueSq <<
      setw(7) << fixed << setprecision(2) << 100. * r.frac << "%" << endl;
  }
  cout << endl;
}

