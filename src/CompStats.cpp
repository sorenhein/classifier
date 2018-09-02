#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "CompStats.h"

#define SEPARATOR ";"


CompStats::CompStats()
{
}


CompStats::~CompStats()
{
}


void CompStats::log(
  const string& key,
  const unsigned rank,
  const double residuals)
{
  auto it = stats.find(key);
  if (it == stats.end())
  {
    stats[key].resize(4);
  }

  const unsigned r = min(rank, 3u);
  vector<Entry>& v = stats[key];
  v[r].no++;
  v[r].sum += residuals;
}


void CompStats::printHeader(
  ofstream& fout,
  const string& tag) const
{
  fout << 
    setw(24) << left << tag <<
    setw(4) << right << "#1" << 
      setw(8) << "avg" <<
    setw(4) << right << "#2" << 
      setw(8) << "avg" <<
    setw(4) << right << "#3" << 
      setw(8) << "avg" <<
    setw(4) << right << "4+" << 
      setw(8) << "avg" << "\n";
}


string CompStats::strEntry(const Entry& entry) const
{
  stringstream ss;
  if (entry.no > 0)
    ss << setw(4) << right << entry.no <<
     setw(8) << fixed << setprecision(2) << entry.sum / entry.no;
  else
    ss << setw(4) << right << "-" << setw(8) << "-";
  return ss.str();
}


void CompStats::print(
  const string& fname,
  const string& tag) const
{
  ofstream fout;
  fout.open(fname);

  CompStats::printHeader(fout, tag);

  vector<Entry> sumEntries(4);

  for (auto& it: stats)
  {
    fout <<
      setw(24) << left << it.first <<
      CompStats::strEntry(it.second[0]) <<
      CompStats::strEntry(it.second[1]) <<
      CompStats::strEntry(it.second[2]) <<
      CompStats::strEntry(it.second[3]) << "\n";
    
    for (unsigned i = 0; i < 4; i++)
    {
      sumEntries[i].no += it.second[i].no;
      sumEntries[i].sum += it.second[i].sum;
    }
  }

  const string dashes(72, '-');
  fout << dashes << "\n";
  fout <<
    setw(24) << left << "Sum" <<
    CompStats::strEntry(sumEntries[0]) <<
    CompStats::strEntry(sumEntries[1]) <<
    CompStats::strEntry(sumEntries[2]) <<
    CompStats::strEntry(sumEntries[3]) << "\n";

  fout.close();
}


void CompStats::printHeaderCSV(
  ofstream& fout,
  const string& tag) const
{
  fout << tag << ";#1;avg;#2;avg;#3;avg;less;avg\n";
}


string CompStats::strEntryCSV(const Entry& entry) const
{
  stringstream ss;
  if (entry.no > 0)
    ss << SEPARATOR <<
      entry.no << SEPARATOR << 
      fixed << setprecision(2) << entry.sum / entry.no;
  else
    ss << SEPARATOR << entry.no << SEPARATOR << "-";
  return ss.str();
}


void CompStats::printCSV(
  const string& fname,
  const string& tag) const
{
  ofstream fout;
  fout.open(fname);

  CompStats::printHeaderCSV(fout, tag);

  for (auto& it: stats)
  {
    fout <<
      setw(24) << left << it.first <<
      CompStats::strEntryCSV(it.second[0]) <<
      CompStats::strEntryCSV(it.second[1]) <<
      CompStats::strEntryCSV(it.second[2]) <<
      CompStats::strEntryCSV(it.second[3]) << "\n";
  }


  fout.close();
}

