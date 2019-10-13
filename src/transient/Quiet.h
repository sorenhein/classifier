#ifndef TRAIN_QUIET_H
#define TRAIN_QUIET_H

#include <vector>
#include <list>
#include <string>

#include "QuietData.h"
#include "trans.h"

using namespace std;


class Quiet
{
  private:

    unsigned durationCoarse;
    unsigned durationFine;
    unsigned padSamples;

    list<QuietData> quietCoarse;

    vector<float> synth;

    // The interval for synthetic output.
    Interval writeInterval;


    void makeStarts(
      const Interval& interval,
      const bool fromBackFlag,
      const unsigned duration,
      list<QuietData>& quietList) const;

    void makeStats(
      const vector<float>& samples,
      QuietData& qentry) const;

    unsigned annotateList(
      const vector<float>& samples,
      list<QuietData>& quietList) const;

    void getFinetuneStatistics(
      const vector<float>& samples,
      list<QuietData>& fineStarts,
      float& sdevThreshold) const;

    void adjustIntervals(
      const bool fromBackFlag,
      QuietData& qstatsCoarse,
      const unsigned index);

    void setFineInterval(
      const QuietData& qstatsCoarse,
      const bool fromBackFlag,
      const unsigned sampleSize,
      Interval& intervalFine) const;

    void finetune(
      const vector<float>& samples,
      const bool fromBackFlag,
      QuietData& qstats);

    void adjustOutputIntervals(
      const QuietData& quietList,
      const bool fromBackFlag,
      Interval& avail);

    void synthesize(const list<QuietData>& quietList);


  public:

    Quiet();

    ~Quiet();

    bool detect(
      const vector<float>& samples,
      const float sampleRate,
      const bool fromBackFlag,
      Interval& available);

    void detectIntervals(
      const vector<float>& samples,
      const Interval& available,
      list<Interval>& actives);

    void writeFile(const string& filename) const;
};

#endif
