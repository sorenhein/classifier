#include <iostream>
#include <iomanip>
#include <string>

#include "print.h"

using namespace std;


void printDataHeader();

void printDataLine(
  const string& text,
  const double actual,
  const double estimate);



void printPeakPosCSV(
  const vector<PeakPos>& peaks,
  const int level)
{
  for (unsigned i = 0; i < peaks.size(); i++)
    cout << fixed << setprecision(4) << 
      peaks[i].pos << ";" << level << endl;
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

