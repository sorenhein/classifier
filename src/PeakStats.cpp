#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <mutex>
#include <cassert>

#include "PeakStats.h"

#include "util/misc.h"

mutex mtxPeakStats;


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
    missedTrueFront[i].resize(PEAK_TRUE_SIZE);
    missedTrueBack[i].resize(PEAK_TRUE_SIZE);
  }

  missedTrueCore.resize(PEAK_TRUE_SIZE);

  typeNamesSeen.resize(PEAK_SEEN_SIZE);
  typeNamesSeen[PEAK_SEEN_TOO_EARLY] = "too early";
  typeNamesSeen[PEAK_SEEN_EARLY] = "early";
  typeNamesSeen[PEAK_SEEN_CORE] = "core";
  typeNamesSeen[PEAK_SEEN_LATE] = "late";
  typeNamesSeen[PEAK_SEEN_TOO_LATE] = "too late";

  typeNamesTrue.resize(PEAK_TRUE_SIZE);
  typeNamesTrue[PEAK_TRUE_TOO_EARLY] = "early";
  typeNamesTrue[PEAK_TRUE_MISSED] = "missed";
  typeNamesTrue[PEAK_TRUE_TOO_LATE] = "late";
}


void PeakStats::logSeenHit(const PeakSeenType stype)
{
  assert(stype < PEAK_SEEN_SIZE);
  mtxPeakStats.lock();
  statsSeen[stype].good++;
  statsSeen[stype].len++;
  mtxPeakStats.unlock();
}


void PeakStats::logSeenMiss(const PeakSeenType stype)
{
  assert(stype < PEAK_SEEN_SIZE);
  mtxPeakStats.lock();
  statsSeen[stype].len++;
  mtxPeakStats.unlock();
}


void PeakStats::logTrueHit(
  const unsigned trueNo,
  const unsigned trueLen)
{
  if (trueNo < PEAKSTATS_END_COUNT)
  {
    mtxPeakStats.lock();
    statsTrueFront[trueNo].good++;
    statsTrueFront[trueNo].len++;
    mtxPeakStats.unlock();
  }
  else if (trueNo + (PEAKSTATS_END_COUNT+1) > trueLen)
  {
    mtxPeakStats.lock();
    statsTrueBack[trueLen-1-trueNo].good++;
    statsTrueBack[trueLen-1-trueNo].len++;
    mtxPeakStats.unlock();
  }
  else
  {
    mtxPeakStats.lock();
    statsTrueCore.good++;
    statsTrueCore.len++;
    mtxPeakStats.unlock();
  }
}


void PeakStats::logTrueMiss(
  const unsigned trueNo,
  const unsigned trueLen,
  const PeakTrueType ttype)
{
  if (trueNo < PEAKSTATS_END_COUNT)
  {
    mtxPeakStats.lock();
    statsTrueFront[trueNo].len++;
    missedTrueFront[trueNo][ttype]++;
    mtxPeakStats.unlock();
  }
  else if (trueNo + (PEAKSTATS_END_COUNT+1) > trueLen)
  {
    mtxPeakStats.lock();
    statsTrueBack[trueLen-1-trueNo].len++;
    missedTrueBack[trueLen-1-trueNo][ttype]++;
    mtxPeakStats.unlock();
  }
  else
  {
    mtxPeakStats.lock();
    statsTrueCore.len++;
    missedTrueCore[ttype]++;
    mtxPeakStats.unlock();
  }
}


void PeakStats::writeTrueHeader(ofstream& fout) const
{
  fout << setw(12) << left << "True peaks" <<
    setw(8) << right << "Sum" <<
    setw(8) << "Seen #" <<
    setw(8) << "Seen %" <<
    setw(8) << "Not #" <<
    setw(8) << "Not %";

  for (unsigned i = 0; i < PEAK_TRUE_SIZE; i++)
    fout << setw(8) << typeNamesTrue[i];

  fout << endl;
}


void PeakStats::writeTrueLine(
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
    setw(8) << percent(e.good, e.len, 2u) << 
    setw(8) << right << e.len - e.good <<
    setw(8) << percent(e.len - e.good, e.len, 2u);

  ecum += e;

  for (unsigned i = 0; i < PEAK_TRUE_SIZE; i++)
  {
    fout << setw(8) << v[i];
    vcum[i] += v[i];
  }
  fout << "\n";
}



void PeakStats::writeTrueTable(ofstream& fout) const
{
  PeakStats::writeTrueHeader(fout);

  Entry sumTrue;
  vector<unsigned> sumMisses;
  sumMisses.resize(PEAK_TRUE_SIZE);

  for (unsigned i = 0; i < statsTrueFront.size(); i++)
  {
    PeakStats::writeTrueLine(fout, to_string(i),
      statsTrueFront[i], missedTrueFront[i],
      sumTrue, sumMisses);
  }

  PeakStats::writeTrueLine(fout, "core",
    statsTrueCore, missedTrueCore,
    sumTrue, sumMisses);

  for (unsigned i = 0; i < statsTrueBack.size(); i++)
  {
    const unsigned rev = statsTrueBack.size() - i - 1;

    PeakStats::writeTrueLine(fout, "-" + to_string(rev),
      statsTrueBack[rev], missedTrueBack[rev],
      sumTrue, sumMisses);
  }

  fout << string(76, '-') << "\n";
  PeakStats::writeTrueLine(fout, "Sum",
    sumTrue, sumMisses,
    sumTrue, sumMisses);
  fout << "\n";
}


void PeakStats::writeSeenHeader(ofstream& fout) const
{
  fout << setw(12) << left << "Seen peaks" <<
    setw(8) << right << "Sum" <<
    setw(8) << "True #" <<
    setw(8) << "True %" <<
    setw(8) << "Not #" <<
    setw(8) << "Not %" <<
    endl;
}


void PeakStats::writeSeenLine(
  ofstream& fout,
  const string& text,
  const Entry& e) const
{
  fout << setw(12) << left << text <<
    setw(8) << right << e.len <<
    setw(8) << right << e.good <<
    setw(8) << percent(e.good, e.len, 2u) << 
    setw(8) << right << e.len - e.good <<
    setw(8) << percent(e.len - e.good, e.len, 2u) << endl;
}


void PeakStats::writeSeenTable(ofstream& fout) const
{
  PeakStats::writeSeenHeader(fout);

  Entry sumSeen;
  for (unsigned i = 0; i < statsSeen.size(); i++)
  {
    PeakStats::writeSeenLine(fout, typeNamesSeen[i], statsSeen[i]);
    sumSeen += statsSeen[i];
  }

  fout << string(52, '-') << "\n";
  PeakStats::writeSeenLine(fout, "Sum", sumSeen);
}


void PeakStats::write(const string& fname) const
{
  ofstream fout;
  fout.open(fname);

  PeakStats::writeTrueTable(fout);
  PeakStats::writeSeenTable(fout);

  fout.close();
}

