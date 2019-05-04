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
class PeakPool;


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

  string strAbs(
    const string& title,
    const unsigned offset) const
  {
    stringstream ss;
    ss << title << ": " <<
      indices[0] + offset << " - " <<
      indices[1] + offset<< " - " <<
      indices[2] + offset<< " - " <<
      indices[3] + offset<< "\n";
    return ss.str();
  };

  string strRel(const string& title) const
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

    struct SingleEntry
    {
      unsigned target;
      unsigned lower;
      unsigned upper;
      unsigned count;

      bool operator < (const SingleEntry& se2)
      {
        return (count < se2.count);
      };

      string str(const unsigned offset)
      {
        stringstream ss;
        ss << target + offset << "(" <<
          lower + offset << "-" <<
          upper + offset << "), count " <<
          count << endl;
        return ss.str();
      };
    };

    unsigned offset;

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
      list<FullEntry const *>& feps) const;

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

    void updateUnused(
      PatternEntry const * pep,
      PeakPtrs& peakPtrsUnused) const;

    void updateUsed(
      const vector<Peak const *>& peaksClose,
      PeakPtrs& peakPtrsUsed) const;

    void update(
      PatternEntry const * pep,
      const vector<Peak const *>& peaksClose,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused) const;

    void printClosest(
      const vector<unsigned>& indices,
      const vector<Peak const *>& peaksClose) const;

    void examineCandidates(
      const PeakPtrs& peakPtrsUsed,
      list<PatternEntry>& candidates,
      list<PatternEntry>::iterator& pe4,
      vector<Peak const *>& peaksBest,
      list<SingleEntry>& singles) const;

    void addToSingles(
      const vector<unsigned>& indices,
      const vector<Peak const *>& peaksClose,
      list<SingleEntry>& singles) const;

    void condenseSingles(list<SingleEntry>& singles) const;

    bool fixSingles(
      PeakPool& peaks,
      list<SingleEntry>& singles,
      PeakPtrs& peakPtrsUsed) const;


  public:

    PeakPattern();

    ~PeakPattern();

    void reset();

    bool suggest(
      const CarModels& models,
      const PeakRange& range,
      const unsigned offset,
      list<PatternEntry>& candidates);

    bool verify(
      list<PatternEntry>& candidates,
      PeakPool& peaks,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused) const;
};

#endif
