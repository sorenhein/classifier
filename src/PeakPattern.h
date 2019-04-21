#ifndef TRAIN_PEAKPATTERN_H
#define TRAIN_PEAKPATTERN_H

#include <vector>
#include <list>

#include "struct.h"

using namespace std;

class CarModels;
class PeakRange;


struct PatternEntry
{
  unsigned modelNo;
  bool abutLeftFlag;
  bool abutRightFlag;
  vector<unsigned> indices;
};


class PeakPattern
{
  private:


  public:

    PeakPattern();

    ~PeakPattern();

    void reset();

    bool suggest(
      const CarModels& models,
      const PeakRange& range,
      list<PatternEntry>& candidates) const;

};

#endif
