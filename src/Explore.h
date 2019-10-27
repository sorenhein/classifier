#ifndef TRAIN_EXPLORE_H
#define TRAIN_EXPLORE_H

#include <list>
#include <vector>
#include <string>

#include "Explore.h"
#include "transient/trans.h"

using namespace std;

class Peak;
class PeakPtrs;


class Explore
{
  private:

    struct Correlate
    {
      unsigned shift;
      unsigned overlap;
      float value;
    };

    struct Datum
    {
      QuietInterval const * qint;
      vector<Peak const *> peaksOrig;
      vector<Peak const *> peaksNew;
      vector<Correlate> correlates;
    };

    vector<float> filtered;

    vector<Datum> data;


    float filterEdge(
      const vector<float>& accel,
      const unsigned pos,
      const unsigned lobe) const;

    void filter(const vector<float>& accel);

    void correlateFixed(
      const QuietInterval& first,
      const QuietInterval& second,
      const unsigned offset,
      Correlate& corr) const;

    unsigned guessShiftSameLength(
      vector<Peak const *>& first,
      vector<Peak const *>& second,
      unsigned& devSq) const;

    unsigned guessShiftFirstLonger(
      vector<Peak const *>& first,
      vector<Peak const *>& second) const;

    unsigned guessShift(
      vector<Peak const *>& first,
      vector<Peak const *>& second) const;

    void correlateBest(
      const QuietInterval& first,
      const QuietInterval& second,
      const unsigned guess,
      Correlate& corr) const;

    string strHeader() const;

  public:

    Explore();

    ~Explore();

    void reset();

    void log(
      const list<QuietInterval>& actives,
      const PeakPtrs& candidates);

    void correlate(const vector<float>& accel);

    string str() const;
};

#endif
