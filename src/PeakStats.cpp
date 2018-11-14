#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "PeakStats.h"
#include "Except.h"


PeakStats::PeakStats()
{
  PeakStats::reset();
}


PeakStats::~PeakStats()
{
}


void PeakStats::reset()
{
  statsSeen.clear();
  statsSeen.resize(PEAK_SEEN_SIZE);

  statsTrueFront.clear();
  statsTrueFront.resize(PEAKSTATS_END_COUNT);

  statsTrueCore.good = 0;
  statsTrueCore.len = 0;

  statsTrueBack.clear();
  statsTrueBack.resize(PEAKSTATS_END_COUNT);

  missedTrueFront.resize(PEAKSTATS_END_COUNT);
  missedTrueBack.resize(PEAKSTATS_END_COUNT);

  for (unsigned i = 0; i < PEAKSTATS_END_COUNT; i++)
  {
    missedTrueFront[i].resize(PEAK_TRUE2_SIZE);
    missedTrueBack[i].resize(PEAK_TRUE2_SIZE);
  }

  missedTrueCore.resize(PEAK_TRUE2_SIZE);

  typeNamesSeen.resize(PEAK_SEEN_SIZE);
  typeNamesSeen[PEAK_SEEN_TOO_EARLY] = "too early";
  typeNamesSeen[PEAK_SEEN_EARLY] = "early";
  typeNamesSeen[PEAK_SEEN_CORE] = "core";
  typeNamesSeen[PEAK_SEEN_LATE] = "late";
  typeNamesSeen[PEAK_SEEN_TOO_LATE] = "too late";

  typeNamesTrue.resize(PEAK_TRUE2_SIZE);
  typeNamesTrue[PEAK_TRUE2_TOO_EARLY] = "early";
  typeNamesTrue[PEAK_TRUE2_MISSED] = "missed";
  typeNamesTrue[PEAK_TRUE2_TOO_LATE] = "late";
}


void PeakStats::logSeenHit(const PeakSeenType stype)
{
  statsSeen[stype].good++;
  statsSeen[stype].len++;
}


void PeakStats::logSeenMiss(const PeakSeenType stype)
{
  statsSeen[stype].len++;
}


void PeakStats::logTrueHit(
  const unsigned trueNo,
  const unsigned trueLen)
{
  if (trueNo < PEAKSTATS_END_COUNT)
  {
    statsTrueFront[trueNo].good++;
    statsTrueFront[trueNo].len++;
  }
  else if (trueNo + (PEAKSTATS_END_COUNT+1) > trueLen)
  {
    statsTrueBack[trueLen-1-trueNo].good++;
    statsTrueBack[trueLen-1-trueNo].len++;
  }
  else
  {
    statsTrueCore.good++;
    statsTrueCore.len++;
  }
}


void PeakStats::logTrueMiss(
  const unsigned trueNo,
  const unsigned trueLen,
  const PeakTrueType ttype)
{
  if (trueNo < PEAKSTATS_END_COUNT)
  {
    statsTrueFront[trueNo].len++;
    missedTrueFront[trueNo][ttype]++;
  }
  else if (trueNo + (PEAKSTATS_END_COUNT+1) > trueLen)
  {
    statsTrueBack[trueLen-1-trueNo].len++;
    missedTrueBack[trueLen-1-trueNo][ttype]++;
  }
  else
  {
    statsTrueCore.len++;
    missedTrueCore[ttype]++;
  }
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


void PeakStats::printTrueHeader(ofstream& fout) const
{
  fout << setw(12) << left << "True peaks" <<
    setw(8) << right << "Sum" <<
    setw(8) << "Seen #" <<
    setw(8) << "Seen %" <<
    setw(8) << "Not #" <<
    setw(8) << "Not %";

  for (unsigned i = 0; i < PEAK_TRUE2_SIZE; i++)
    fout << setw(8) << typeNamesTrue[i];

  fout << endl;
}


void PeakStats::printTrueLine(
  ofstream& fout,
  const string& text,
  const Entry& e,
  const vector<unsigned>& v,
  Entry& ecum,
  vector<unsigned>& vcum) const
{
  fout << setw(12) << left << text <<
    setw(8) << right << e.len <<
    setw(8) << e.good <<
    setw(8) << PeakStats::percent(e.good, e.len) << 
    setw(8) << right << e.len - e.good <<
    setw(8) << PeakStats::percent(e.len - e.good, e.len);

  ecum += e;

  for (unsigned i = 0; i < PEAK_TRUE2_SIZE; i++)
  {
    fout << setw(8) << v[i];
    vcum[i] += v[i];
  }
  fout << "\n";
}



void PeakStats::printTrueTable(ofstream& fout) const
{
  PeakStats::printTrueHeader(fout);

  Entry sumTrue;
  vector<unsigned> sumMisses;
  sumMisses.resize(PEAK_TRUE2_SIZE);

  for (unsigned i = 0; i < statsTrueFront.size(); i++)
  {
    PeakStats::printTrueLine(fout, to_string(i),
      statsTrueFront[i], missedTrueFront[i],
      sumTrue, sumMisses);
  }

  PeakStats::printTrueLine(fout, "core",
    statsTrueCore, missedTrueCore,
    sumTrue, sumMisses);

  for (unsigned i = 0; i < statsTrueBack.size(); i++)
  {
    const unsigned rev = statsTrueBack.size() - i - 1;

    PeakStats::printTrueLine(fout, "-" + to_string(rev),
      statsTrueBack[rev], missedTrueBack[rev],
      sumTrue, sumMisses);
  }

  fout << string(76, '-') << "\n";
  PeakStats::printTrueLine(fout, "Sum",
    sumTrue, sumMisses,
    sumTrue, sumMisses);
  fout << "\n";
}


void PeakStats::printSeenHeader(ofstream& fout) const
{
  fout << setw(12) << left << "Seen peaks" <<
    setw(8) << right << "Sum" <<
    setw(8) << "True #" <<
    setw(8) << "True %" <<
    setw(8) << "Not #" <<
    setw(8) << "Not %" <<
    endl;
}


void PeakStats::printSeenLine(
  ofstream& fout,
  const string& text,
  const Entry& e) const
{
  fout << setw(12) << left << text <<
  setw(8) << right << e.len <<
  setw(8) << right << e.good <<
  setw(8) << PeakStats::percent(e.good, e.len) << 
  setw(8) << right << e.len - e.good <<
  setw(8) << PeakStats::percent(e.len - e.good, e.len) << 
  endl;
}


void PeakStats::printSeenTable(ofstream& fout) const
{
  PeakStats::printSeenHeader(fout);

  Entry sumSeen;
  for (unsigned i = 0; i < statsSeen.size(); i++)
  {
    PeakStats::printSeenLine(fout, typeNamesSeen[i], statsSeen[i]);
    sumSeen += statsSeen[i];
  }

  fout << string(52, '-') << "\n";
  PeakStats::printSeenLine(fout, "Sum", sumSeen);
}


void PeakStats::print(const string& fname) const
{
  ofstream fout;
  fout.open(fname);

  PeakStats::printTrueTable(fout);

  PeakStats::printSeenTable(fout);

  fout.close();
}

