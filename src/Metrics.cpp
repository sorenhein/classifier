#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "Metrics.h"

#define SEPARATOR ";"


Metrics::Metrics()
{
}


Metrics::~Metrics()
{
}


double Metrics::distanceAlignment(
  const double sumsq,
  const unsigned numPeaksUsed,
  const unsigned numAdd,
  const unsigned numDelete,
  const bool correctFlag)
{
  MetricEntry entry;
  entry.dist1 = (numPeaksUsed == 0 ? 0. : sumsq / numPeaksUsed);
  entry.dist2 = numAdd + numDelete;

  if (correctFlag)
    entriesCorrect.push_back(entry);
  else
    entriesIncorrect.push_back(entry);

  return entry.dist1 + entry.dist2;
}



void Metrics::printCSV(const string& fname) const
{
  ofstream fout;
  fout.open(fname);

  fout << "Correct" << endl;
  for (auto& entry: entriesCorrect)
  {
    fout << fixed << setprecision(2) << entry.dist1 << SEPARATOR <<
      fixed << setprecision(2) << entry.dist2 << "\n";
  }
  fout << "\n";

  fout << "Incorrect" << endl;
  for (auto& entry: entriesIncorrect)
  {
    fout << fixed << setprecision(2) << entry.dist1 << SEPARATOR <<
      fixed << setprecision(2) << entry.dist2 << "\n";
  }
  fout << "\n";

  fout.close();
}

