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
  PATTERN_SINGLE_SIDED = 1,
  PATTERN_DOUBLE_SIDED = 2
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

    void fillFromModel(
      const CarModels& models,
      const unsigned indexModel,
      const bool symmetryFlag,
      const unsigned indexRangeLeft,
      const unsigned indexRangeRight,
      list<PatternEntry>& candidates) const;

    bool guessNoBorders(list<PatternEntry>& candidates) const;

    bool guessLeft(
      const CarModels& models,
      const PeakRange& range,
      list<PatternEntry>& candidates) const;

    bool guessRight(
      const CarModels& models,
      const PeakRange& range,
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
