#ifndef TRAIN_PEAKPATTERN_H
#define TRAIN_PEAKPATTERN_H

#include <vector>
#include <list>

#include "struct.h"

using namespace std;

class CarModels;
class CarDetect;
class PeakRange;


enum PatternType
{
  PATTERN_NO_BORDERS = 0,
  PATTERN_SINGLE_SIDED = 1,
  PATTERN_DOUBLE_SIDED = 2
};

struct PatternEntry
{
  unsigned modelNo;
  bool reverseFlag;
  bool abutLeftFlag;
  bool abutRightFlag;
  vector<unsigned> indices;
  PatternType borders;
};


class PeakPattern
{
  private:

    enum RangeQuality
    {
      QUALITY_WHOLE_MODEL = 0,
      QUALITY_SYMMETRY = 1,
      QUALITY_GENERAL = 2,
      QUALITY_NONE = 3
    };

    bool getRangeQuality(
      const CarModels& models,
      const PeakRange& range,
      CarDetect const * carPtr,
      const bool leftFlag,
      RangeQuality& quality,
      unsigned& gap) const;

    CarDetect const * carBeforePtr;
    CarDetect const * carAfterPtr;

    bool guessNoBorders(list<PatternEntry>& candidates) const;

  public:

    PeakPattern();

    ~PeakPattern();

    void reset();

    bool suggest(
      const CarModels& models,
      const PeakRange& range,
      list<PatternEntry>& candidates);

};

#endif
