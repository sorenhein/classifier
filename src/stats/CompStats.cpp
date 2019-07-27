#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <mutex>

#include "CompStats.h"
#include "../const.h"

#define BUCKETS 4u

mutex mtxCompStats;


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
  mtxCompStats.lock();
  auto it = stats.find(key);
  if (it == stats.end())
    stats[key].resize(BUCKETS);

  const unsigned r = min(rank, BUCKETS-1);
  vector<Entry>& v = stats[key];
  v[r].no++;
  v[r].sum += residuals;
  mtxCompStats.unlock();
}


void CompStats::fail(const string& key)
{
  CompStats::log(key, 10, 1000.);
}


string CompStats::strHeader(const string& tag) const
{
  stringstream ss;
  ss << setw(24) << left << tag;
  for (unsigned i = 1; i < BUCKETS; i++)
  {
    const string s = "#" + to_string(i);
    ss << setw(4) << right << s << setw(8) << "avg";
  }
  const string s = to_string(BUCKETS) + "+";
  ss << right << setw(4) << s << setw(8) << "avg" << "\n";

  return ss.str();
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


void CompStats::write(
  const string& fname,
  const string& tag) const
{
  ofstream fout;
  fout.open(fname);

  fout << CompStats::strHeader(tag);

  vector<Entry> sumEntries(BUCKETS);

  for (auto& it: stats)
  {
    fout << setw(24) << left << it.first;
    for (unsigned i = 0; i < BUCKETS; i++)
      fout << CompStats::strEntry(it.second[i]);
    fout << "\n";
    
    for (unsigned i = 0; i < BUCKETS; i++)
    {
      sumEntries[i].no += it.second[i].no;
      sumEntries[i].sum += it.second[i].sum;
    }
  }

  const string dashes(24 + BUCKETS * 12, '-');
  fout << dashes << "\n";
  fout << setw(24) << left << "Sum";

  for (unsigned i = 0; i < BUCKETS; i++)
    fout << CompStats::strEntry(sumEntries[i]);

  fout << "\n";
  fout.close();
}


string CompStats::strHeaderCSV(const string& tag) const
{
  stringstream ss;
  ss << tag << SEPARATOR;
  for (unsigned i = 1; i < BUCKETS; i++)
    ss << "#" << i << SEPARATOR << "avg" << SEPARATOR;
  ss << BUCKETS << "+" << SEPARATOR << "avg" << "\n";
  return ss.str();
}


string CompStats::strEntryCSV(const Entry& entry) const
{
  stringstream ss;
  if (entry.no > 0)
    ss << SEPARATOR << entry.no << SEPARATOR << 
      fixed << setprecision(2) << entry.sum / entry.no;
  else
    ss << SEPARATOR << entry.no << SEPARATOR << "-";
  return ss.str();
}


void CompStats::writeCSV(
  const string& fname,
  const string& tag) const
{
  ofstream fout;
  fout.open(fname);

  fout << CompStats::strHeaderCSV(tag);

  for (auto& it: stats)
  {
    fout << setw(24) << left << it.first;
    for (unsigned i = 0; i < BUCKETS; i++)
      fout << CompStats::strEntryCSV(it.second[0]);
    fout << "\n";
  }

  fout.close();
}

