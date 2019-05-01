#ifndef TRAIN_PEAKPATTERN_H
#define TRAIN_PEAKPATTERN_H

#include <vector>
#include <list>
#include <sstream>

#include "CarModels.h"
#include "struct.h"

using namespace std;

class CarModels;
class CarDetect;
class PeakRange;


enum PatternType
{
  PATTERN_NO_BORDERS = 0,
  PATTERN_SINGLE_SIDED_LEFT = 1,
  PATTERN_SINGLE_SIDED_RIGHT = 2,
  PATTERN_DOUBLE_SIDED_SINGLE = 3,
  PATTERN_DOUBLE_SIDED_DOUBLE = 4
};

struct PatternEntry
{
  unsigned modelNo;
  bool reverseFlag;
  bool abutLeftFlag;
  bool abutRightFlag;
  unsigned start;
  unsigned end;
  vector<unsigned> indices;
  PatternType borders;

  string str(const string& title) const
  {
    stringstream ss;
    ss << title << ": " <<
      indices[0] - start << " - " <<
      indices[1] - indices[0] << " - " <<
      indices[2] - indices[1] << " - " <<
      indices[3] - indices[2] << " - " <<
      end - indices[3] << "\n";
    return ss.str();
  };
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

    struct FullEntry
    {
      ModelData const * data;
      unsigned index;

      // Interval for different range qualities.
      vector<unsigned> lenLo;
      vector<unsigned> lenHi;
    };

    CarDetect const * carBeforePtr;
    CarDetect const * carAfterPtr;

    RangeQuality qualLeft;
    RangeQuality qualRight;
    unsigned gapLeft;
    unsigned gapRight;

    vector<FullEntry> fullEntries;

    bool getRangeQuality(
      const CarModels& models,
      const PeakRange& range,
      CarDetect const * carPtr,
      const bool leftFlag,
      RangeQuality& quality,
      unsigned& gap) const;

    void getFullModels(const CarModels& models);

    bool findSingleModel(
      const RangeQuality qualOverall,
      const unsigned lenRange,
      FullEntry const *& fep) const;

    bool findDoubleModel(
      const RangeQuality qualOverall,
      const unsigned lenRange,
      list<FullEntry const *>& feps) const;

    bool fillPoints(
      const list<unsigned>& carPoints,
      const unsigned indexBase,
      const bool reverseFlag,
      PatternEntry& pe) const;
      
    bool fillFromModel(
      const CarModels& models,
      const unsigned indexModel,
      const bool symmetryFlag,
      const unsigned indexRangeLeft,
      const unsigned indexRangeRight,
      const PatternType patternType,
      list<PatternEntry>& candidates) const;

    bool guessNoBorders(list<PatternEntry>& candidates) const;

    bool guessLeft(
      const CarModels& models,
      list<PatternEntry>& candidates) const;

    bool guessRight(
      const CarModels& models,
      list<PatternEntry>& candidates) const;

    bool guessBoth(
      const CarModels& models,
      list<PatternEntry>& candidates) const;

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
