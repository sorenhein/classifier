#ifndef TRAIN_SEGQUIET_H
#define TRAIN_SEGQUIET_H

#include <vector>
#include <string>

#include "struct.h"

using namespace std;


enum QuietPlace
{
  QUIET_FRONT = 0,
  QUIET_INTRA = 1,
  QUIET_BACK = 2,
  QUIET_SIZE = 3
};

enum QuietGrade
{
  GRADE_GREEN = 0,
  GRADE_AMBER = 1,
  GRADE_RED = 2,
  GRADE_SIZE = 3
};

struct Interval
{
  unsigned first;
  unsigned len;
  QuietGrade grade;
};


class SegQuiet
{
  private:

    struct QuietStats
    {
      double mean;
      double sdev;
      unsigned len;
    };

    vector<Interval> quiet;

    vector<float> synth;

    unsigned offset;
    unsigned lastSample;

    void makeStarts(
      const Interval& interval,
      const QuietPlace direction,
      vector<unsigned>& startList) const;

    void makeStats(
      const vector<double>& samples,
      const unsigned first,
      const unsigned len,
      QuietStats& qstats) const;

    QuietGrade isQuiet(const QuietStats& qstats) const;

    void addQuiet(
      const unsigned start,
      const unsigned len,
      const QuietGrade grade);

    unsigned curate(
      const unsigned runReds,
      const unsigned totalReds) const;

    void setFinetuneRange(
      const vector<double>& samples,
      const QuietPlace direction,
      vector<unsigned>& fineStarts) const;

    void getFinetuneStatistics(
      const vector<double>& samples,
      vector<unsigned>& fineStarts,
      vector<QuietStats>& fineList,
      double& sdevThreshold) const;

    void adjustIntervals(
      const QuietPlace direction,
      const unsigned index);

    void finetune(
      const vector<double>& samples,
      const QuietPlace direction);

    void adjustOffset(
      const Interval& avail,
      const QuietPlace direction,
      const unsigned numInt);

    void makeSynth();

    void makeActive(
      vector<Interval>& active,
      const unsigned firstSampleNo) const;

    void printStats(
      const QuietStats& qstats,
      const unsigned first,
      const bool flag) const;

  public:

    SegQuiet();

    ~SegQuiet();

    void reset();

    bool detect(
      const vector<double>& samples, // TODO: Should use times[]
      const vector<Interval>& available,
      const QuietPlace direction,
      vector<Interval>& active);

    void writeBinary(
      const string& origname,
      const string& dirname) const;

    string headerCSV() const;

    string strCSV() const;
};

#endif
