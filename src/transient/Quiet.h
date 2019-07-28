#ifndef TRAIN_QUIET_H
#define TRAIN_QUIET_H

#include <vector>
#include <string>

#include "trans.h"

using namespace std;


enum QuietPlace
{
  QUIET_FRONT = 0,
  QUIET_BACK = 1,
  QUIET_SIZE = 2
};

class Quiet
{
  private:

    struct QuietStats
    {
      float mean;
      float sdev;
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
      const vector<float>& samples,
      const unsigned first,
      const unsigned len,
      QuietStats& qstats) const;

    QuietGrade isQuiet(const QuietStats& qstats) const;

    void addQuiet(
      const unsigned start,
      const unsigned len,
      const QuietGrade grade,
      const float mean);

    unsigned curate() const;

    void setFinetuneRange(
      const vector<float>& samples,
      const QuietPlace direction,
      const Interval& quietInt,
      vector<unsigned>& fineStarts) const;

    void getFinetuneStatistics(
      const vector<float>& samples,
      vector<unsigned>& fineStarts,
      vector<QuietStats>& fineList,
      float& sdevThreshold) const;

    void adjustIntervals(
      const QuietPlace direction,
      Interval& quietInt,
      const unsigned index);

    void finetune(
      const vector<float>& samples,
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

    Quiet();

    ~Quiet();

    void reset();

    bool detect(
      const vector<float>& samples,
      const Interval& available,
      const QuietPlace direction,
      Interval& active);

    void writeFile(const string& filename) const;
};

#endif
