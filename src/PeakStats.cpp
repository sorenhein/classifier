#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "PeakStats.h"
#include "Except.h"

#define END_COUNT 4


PeakStats::PeakStats()
{
  PeakStats::reset();
}


PeakStats::~PeakStats()
{
}


void PeakStats::reset()
{
  matchedTrueFront.clear();
  matchedTrueMiddle.clear();
  matchedTrueBack.clear();

  statsSeen.clear();
  statsTrue.clear();

  matchedTrueFront.resize(END_COUNT);
  matchedTrueMiddle.resize(1);
  matchedTrueBack.resize(END_COUNT);

  statsSeen.resize(PEAK_TYPE_SIZE);
  statsTrue.resize(PEAK_TYPE_SIZE);

  typeNames.resize(PEAK_TYPE_SIZE);
  typeNames[PEAK_TENTATIVE] = "tentative";
  typeNames[PEAK_POSSIBLE] = "possible";
  typeNames[PEAK_CONCEIVABLE] = "conceivable";
  typeNames[PEAK_REJECTED] = "rejected";
  typeNames[PEAK_TRANS_FRONT] = "trans-front";
  typeNames[PEAK_TRANS_BACK] = "trans-back";
  typeNames[PEAK_TRUE_TOO_EARLY] = "true too early";
  typeNames[PEAK_TRUE_TOO_LATE] = "true too late";
  typeNames[PEAK_TRUE_MISSING] = "true missing";
}


void PeakStats::logSeenMatch(
  const unsigned matchNoTrue,
  const unsigned lenTrue,
  const PeakType type)
{
  if (matchNoTrue < END_COUNT)
  {
    matchedTrueFront[matchNoTrue].good++;
    matchedTrueFront[matchNoTrue].len++;
  }
  else if (matchNoTrue >= lenTrue-1-END_COUNT)
  {
    matchedTrueBack[lenTrue-1-matchNoTrue].good++;
    matchedTrueBack[lenTrue-1-matchNoTrue].len++;
  }
  else
  {
    matchedTrueMiddle[0].good++;
    matchedTrueMiddle[0].len++;
  }

  statsSeen[type].good++;
  statsSeen[type].len++;

  statsTrue[type].good++;
  statsTrue[type].len++;
}


void PeakStats::logSeenMiss(const PeakType type)
{
  statsSeen[type].len++;
}


void PeakStats::logTrueReverseMatch(
  const unsigned matchNoTrue,
  const unsigned lenTrue,
  const PeakType type)
{
  // Was already logged as a seen miss.

  if (matchNoTrue < END_COUNT)
  {
    matchedTrueFront[matchNoTrue].reverse++;
    matchedTrueFront[matchNoTrue].len++;
  }
  else if (matchNoTrue >= lenTrue-1-END_COUNT)
  {
    matchedTrueBack[lenTrue-1-matchNoTrue].reverse++;
    matchedTrueBack[lenTrue-1-matchNoTrue].len++;
  }
  else
  {
    matchedTrueMiddle[0].reverse++;
    matchedTrueMiddle[0].len++;
  }
  
  statsSeen[type].reverse++;

  statsTrue[type].reverse++;
  statsTrue[type].len++;
}


void PeakStats::logTrueReverseMiss(
  const unsigned matchNoTrue,
  const unsigned lenTrue)
{
  if (matchNoTrue < END_COUNT)
    matchedTrueFront[matchNoTrue].len++;
  else if (matchNoTrue >= lenTrue-1-END_COUNT)
    matchedTrueBack[lenTrue-1-matchNoTrue].len++;
  else
    matchedTrueMiddle[0].len++;

  statsTrue[PEAK_TRUE_MISSING].len++;
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
    setw(8) << right << "Total" <<
    setw(16) << right << "Match" <<
    setw(16) << right << "Reverse" <<
    setw(16) << right << "Unmatched" << endl;

  for (unsigned i = 0; i < matched.size(); i++)
  {
    if (matched[i].len == 0)
      continue;

    const unsigned d = 
      matched[i].len - matched[i].good - matched[i].reverse;

    fout << 
      setw(w) << left << (indexFlag ? to_string(i) : typeNames[i]) <<
      setw(8) << right << matched[i].len <<
      setw(8) << right << matched[i].good <<
      setw(8) << right << 
        PeakStats::percent(matched[i].good, matched[i].len) << 
      setw(8) << right << matched[i].reverse <<
      setw(8) << right << 
        PeakStats::percent(matched[i].reverse, matched[i].len) << 
      setw(8) << right << d <<
      setw(8) << right << PeakStats::percent(d, matched[i].len) << endl;
  }
  fout << endl;
}


void PeakStats::print(const string& fname) const
{
  ofstream fout;
  fout.open(fname);

  PeakStats::printDetailTable(fout, 
    "Number of seen peaks", false, statsSeen);

  PeakStats::printDetailTable(fout, 
    "Number of true peaks", false, statsTrue);
  fout << "Number of true peaks\n";

  fout.close();
}


void PeakStats::printDetail(const string& fname) const
{
  ofstream fout;
  fout.open(fname);

  PeakStats::printDetailTable(fout, "True fronts", true, 
    matchedTrueFront);

  PeakStats::printDetailTable(fout, "True middles", true, 
    matchedTrueMiddle);

  PeakStats::printDetailTable(fout, "True backs", true, 
    matchedTrueBack);

  fout.close();
}

