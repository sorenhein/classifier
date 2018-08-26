#ifndef TRAIN_SEGACTIVE_H
#define TRAIN_SEGACTIVE_H

#include <vector>
#include <string>

#include "Median/median.h"
#include "struct.h"

using namespace std;


class SegActive
{
  private:

    struct SignedPeak
    {
      unsigned index;
      float value;
      bool maxFlag;
      unsigned leftFlank;
      unsigned rightFlank;
      float leftRange;
      float rightRange;
    };

    vector<float> synthSpeed;
    vector<float> synthPos;
    vector<float> synthPeaks;
    vector<MediatorStats> posStats;

    Interval writeInterval;

    void integrate(
      const vector<double>& samples,
      const Interval& aint,
      const double mean);

    void compensateSpeed();
    void compensateMedian();

    void integrateFloat(const Interval& aint);

    void getPosStats();
    void getPosStatsNew(const bool maxFlag);

    void getPeaks(vector<SignedPeak>& posPeaks) const;

    bool checkPeaks(const vector<SignedPeak>& posPeaks) const;

    void getLargePeaks(vector<SignedPeak>& posPeaks) const;

    void levelizePos(const vector<SignedPeak>& posPeaks);

    void makeSynthPeaks(const vector<SignedPeak>& posPeaks);

  public:

    SegActive();

    ~SegActive();

    void reset();

    bool detect(
      const vector<double>& samples, // TODO: Should use times[]
      const vector<Interval>& active,
      const double mean);

    void writeBinary(
      const string& origname,
      const string& speeddirname,
      const string& posdirname,
      const string& peakdirname) const;

    string headerCSV() const;

    string strCSV() const;
};

#endif
