#ifndef TRAIN_SEGQUIET_H
#define TRAIN_SEGQUIET_H

#include <vector>
#include <string>

#include "struct.h"

using namespace std;


enum QuietPlace
{
  QUIET_FRONT = 0,
  QUIET_BACK = 1,
  QUIET_SIZE = 2
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

    Interval writeInterval;
    Interval activeInterval;

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
      const QuietGrade grade,
      const double mean);

    unsigned curate() const;

    void setFinetuneRange(
      const vector<double>& samples,
      const QuietPlace direction,
      const Interval& quietInt,
      vector<unsigned>& fineStarts) const;

    void getFinetuneStatistics(
      const vector<double>& samples,
      vector<unsigned>& fineStarts,
      vector<QuietStats>& fineList,
      double& sdevThreshold) const;

    void adjustIntervals(
      const QuietPlace direction,
      Interval& quietInt,
      const unsigned index);

    void finetune(
      const vector<double>& samples,
      const QuietPlace direction,
      Interval& quietInt);

    void adjustOutputIntervals(
      const Interval& avail,
      const QuietPlace direction);

    void makeSynth();

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
      const Interval& available,
      const QuietPlace direction,
      Interval& active);

    void writeFile(
      const string& origname,
      const string& dirname) const;
};

#endif
