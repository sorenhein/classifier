#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "PeakStats.h"
#include "Except.h"

#define DEFAULT_COUNT 100


PeakStats::PeakStats()
{
  PeakStats::reset();
}


PeakStats::~PeakStats()
{
}


void PeakStats::reset()
{
  matchedTrue.clear();
  matchedSeen.clear();
  statsSeen.clear();
  statsTrue.clear();

  countTrue = DEFAULT_COUNT;
  countSeen = DEFAULT_COUNT;

  matchedTrue.resize(DEFAULT_COUNT);
  matchedSeen.resize(DEFAULT_COUNT);
  statsSeen.resize(PEAK_TYPE_SIZE);
  statsTrue.resize(PEAK_TYPE_SIZE);
  typeNames.resize(PEAK_TYPE_SIZE);

  typeNames[PEAK_TENTATIVE] = "tentative";
  typeNames[PEAK_POSSIBLE] = "possible";
  typeNames[PEAK_CONCEIVABLE] = "conceivable";
  typeNames[PEAK_REJECTED] = "rejected";
  typeNames[PEAK_TRANSIENT] = "transient";
  typeNames[PEAK_TOO_EARLY] = "too early";
  typeNames[PEAK_TOO_LATE] = "too late";
  typeNames[PEAK_MISSING] = "missing";
}


void PeakStats::log(
  const int matchNoTrue,
  const int matchNoSeen,
  const PeakType type)
{
  const int iTrue = static_cast<int>(countTrue);
  const int iSeen = static_cast<int>(countSeen);

  if (matchNoTrue >= iTrue)
  {
    countTrue += max(DEFAULT_COUNT, matchNoTrue + 1 - iTrue);
    matchedTrue.resize(countTrue);
  }

  if (matchNoSeen >= iSeen)
  {
    countSeen += max(DEFAULT_COUNT, matchNoSeen + 1 - iSeen);
    matchedSeen.resize(countSeen);
  }

  if (matchNoSeen >= 0)
    statsSeen[type].no++;


  if (matchNoTrue >= 0)
  {
    statsTrue[type]++;
    matchedTrue[matchNoTrue].no++;

    if (matchNoSeen >= 0)
    {
      statsSeen[type].count++;
      matchedTrue[matchNoTrue].count++;

      matchedSeen[matchNoSeen].no++;
      matchedSeen[matchNoSeen].count++;
    }
  }
  else if (matchNoSeen >= 0)
    matchedSeen[matchNoSeen].no++;
  else
    THROW(ERR_ALGO_PEAKSTATS, "Bad call of log()");
}


string PeakStats::percent(
  const unsigned num,
  const unsigned denom) const
{
  stringstream ss;

  if (denom == 0 || num == 0)
    ss << "-";
  else
    ss << fixed << setprecision(2) << 
      100. * num / static_cast<double>(denom) << "%";
  
  return ss.str();
}


void PeakStats::printDetailTable(
  ofstream& fout,
  const string& title,
  const bool indexFlag,
  const vector<Entry>& matched) const
{
  fout << title << endl;

  string e;
  unsigned w;
  if (indexFlag)
  {
    e = "Index";
    w = 8;
  }
  else
  {
    e = "Type";
    w = 15;
  }

  fout << 
    setw(w) << left << e <<
    setw(8) << right << "True" <<
    setw(8) << right << "Out of" <<
    setw(8) << right << "%" << endl;

  for (unsigned i = 0; i < matched.size(); i++)
  {
    if (matched[i].no == 0)
      continue;

    fout << 
      setw(w) << left << (indexFlag ? to_string(i) : typeNames[i]) <<
      setw(8) << right << matched[i].count <<
      setw(8) << right << matched[i].no <<
      setw(8) << right << 
        PeakStats::percent(matched[i].count, matched[i].no) << endl;
  }
  fout << endl;
}


void PeakStats::print(const string& fname) const
{
  ofstream fout;
  fout.open(fname);

  PeakStats::printDetailTable(fout, 
    "Number of peaks that are true", false, statsSeen);

  fout << "Number of true peaks of different types\n";

  fout << 
    setw(15) << left << "Type" <<
    setw(8) << right << "True" <<
    setw(8) << right << "%" << endl;

  unsigned sum = 0;
  for (unsigned i = 0; i < statsTrue.size(); i++)
    sum += statsTrue[i];

  for (unsigned i = 0; i < statsTrue.size(); i++)
  {
    fout << 
      setw(15) << left << typeNames[i] <<
      setw(8) << right << statsTrue[i] <<
      setw(8) << right << PeakStats::percent(statsTrue[i], sum) << endl;
  }
  fout << endl;

  fout.close();
}


void PeakStats::printDetail(const string& fname) const
{
  ofstream fout;
  fout.open(fname);

  PeakStats::printDetailTable(fout, 
    "True peaks that are matched to seen peaks", true, matchedTrue);

  PeakStats::printDetailTable(fout, 
    "Candidate peaks that are matched to true peaks", true, matchedSeen);

  fout.close();
}

