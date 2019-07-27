#include <iostream>
#include <iomanip>
#include <string>

#include "print.h"
#include "const.h"
#include "align/Alignment.h"

using namespace std;


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


void printMatches(
  const vector<Alignment>& matches)
{
  cout << "Matching alignment\n";
  for (auto& match: matches)
    cout << match.str();
  cout << "\n";
}

