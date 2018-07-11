#include <iomanip>
#include <fstream>
#include <sstream>

#include "StatEntry.h"


StatEntry::StatEntry()
{
  entries.clear();
  entries.resize(ENTRY_SIZE);
}


StatEntry::~StatEntry()
{
}


bool StatEntry::log(
  const vector<double>& actual,
  const vector<double>& estimate,
  const double residuals)
{
  const unsigned l = actual.size();

  // Either (offset, v) or (offset, v, a).
  if (l != 1 && l != 2)
    return false;

  if (estimate.size() != l)
    return false;

  for (unsigned i = 0; i < l; i++)
  {
    entries[i].count++;

    const double delta = estimate[i] - actual[i];
    entries[i].sum += delta;
    entries[i].sumsq += delta * delta;
  }

  entries[ENTRY_RESIDUAL].count++;
  entries[ENTRY_RESIDUAL].sum += residuals;
}


string StatEntry::avg(const unsigned index) const
{
  if (entries[index].count == 0)
    return "-";

  stringstream ss;
  ss << fixed << setprecision(2) << 
    entries[index].sum / entries[index].count;
  return ss.str();
}


string StatEntry::sdev(const unsigned index) const
{
  if (entries[index].count == 0)
    return "-";

  stringstream ss;
  const double n = static_cast<double>(entries[index].count);
  const double sdev = 
    sqrt((n * entries[index].sumsq -
        entries[index].sum * entries[index].sum) / (n * (n-1.)));

  ss << fixed << setprecision(2) << sdev;
  return ss.str();
}


void StatEntry::printHeader(
  ofstream& fout,
  const string& header) const
{
  fout << setw(16) << header << " | " <<
    setw(6) << "count" << " | " <<
    setw(16) << "offset [m]" << " | " <<
    setw(12) << "speed [m/s]" << " | " <<
    setw(12) << "accel [m/s2]" << " | " <<
    setw(7) << right << "RMS [m]" << endl;

  fout << setw(16) << "" << " | " <<
    setw(6) << "" << " | " <<
    setw(6) << right << "avg" << setw(6) << "sdev" << " | " <<
    setw(6) << right << "avg" << setw(6) << "sdev" << " | " <<
    setw(6) << right << "avg" << setw(6) << "sdev" << " | " <<
    setw(7) << right << "avg" << endl;
}


void StatEntry::print(
  ofstream& fout,
  const string& title) const
{
  fout << setw(16) << title << " | " <<
    setw(6) << entries[0].count << " | " <<
    setw(6) << right << StatEntry::avg(0) << 
    setw(6) << StatEntry::sdev(0) << " | " <<
    setw(6) << right << StatEntry::avg(1) << 
    setw(6) << StatEntry::sdev(1) << " | " <<
    setw(6) << right << StatEntry::avg(2) << 
    setw(6) << StatEntry::sdev(2) << " | " <<
    setw(6) << StatEntry::avg(3) << endl;
}

