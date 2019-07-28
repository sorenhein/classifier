#ifndef TRAIN_QUIET_H
#define TRAIN_QUIET_H

#include <vector>
#include <list>
#include <string>

#include "trans.h"

using namespace std;


class Quiet
{
  private:

    unsigned durationCoarse;
    unsigned durationFine;

    struct QuietStats
    {
      unsigned start;
      unsigned len;
      float mean;
      float sdev;
      QuietGrade grade;
    };

    list<QuietStats> quietCoarse;


    vector<float> synth;

    Interval writeInterval;
    Interval activeInterval;


    void makeStarts(
      const Interval& interval,
      const bool fromBackFlag,
      const unsigned duration,
      list<QuietStats>& quietList) const;

    void makeStats(
      const vector<float>& samples,
      QuietStats& qentry) const;

    QuietGrade isQuiet(const QuietStats& qstats) const;

    unsigned curate(const list<QuietStats>& quietList) const;

    void getFinetuneStatistics(
      const vector<float>& samples,
      list<QuietStats>& fineStarts,
      float& sdevThreshold) const;

    void adjustIntervals(
      const bool fromBackFlag,
      QuietStats& qstatsCoarse,
      const unsigned index);

    void setFineInterval(
      const QuietStats& qstatsCoarse,
      const bool fromBackFlag,
      const unsigned sampleSize,
      Interval& intervalFine) const;

    void finetune(
      const vector<float>& samples,
      const bool fromBackFlag,
      QuietStats& qstats);

    void adjustOutputIntervals(
      const list<QuietStats>& quietList,
      const Interval& avail,
      const bool fromBackFlag);

    void makeSynth(const list<QuietStats>& quietList);

    void printStats(
      const QuietStats& qstats,
      const unsigned first,
      const bool flag) const;

  public:

    Quiet();

    ~Quiet();

    bool detect(
      const vector<float>& samples,
      const double sampleRate,
      const Interval& available,
      const bool fromBackFlag,
      Interval& active);

    void writeFile(const string& filename) const;
};

#endif
