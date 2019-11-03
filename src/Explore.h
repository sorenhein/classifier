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

    void filter(
      const vector<float>& integrand,
      vector<float>& result);

    void integrate(
      const vector<float>& accel,
      const unsigned start,
      const unsigned end,
      const float sampleRate,
      vector<float>& speed) const;

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
      const float refValue,
      Correlate& corr) const;

    unsigned powerOfTwo(const unsigned len) const;

    void setCorrelands(
      const vector<float>& accel,
      const unsigned len,
      const unsigned start,
      const unsigned lower,
      const unsigned upper,
      vector<float>& bogieRev,
      vector<float>& wheel1Rev,
      vector<float>& wheel2Rev) const;

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
