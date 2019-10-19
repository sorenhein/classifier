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
      Interval& qintCoarse,
      const unsigned index);

    void setFineInterval(
      const Interval& qintCoarse,
      const bool fromBackFlag,
      const unsigned sampleSize,
      Interval& intervalFine) const;

    void finetune(
      const vector<float>& samples,
      const bool fromBackFlag,
      Interval& qint);

    void adjustOutputIntervals(
      const Interval& qint,
      const bool fromBackFlag,
      Interval& avail);

    void synthesize(
      const Interval& available,
      const list<Interval>& actives);


  public:

    Quiet();

    ~Quiet();

    void detect(
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
