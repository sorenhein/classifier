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

    struct ActiveEntry
    {
      ModelData const * data;
      unsigned index;

      // Interval for different range qualities.
      vector<unsigned> lenLo;
      vector<unsigned> lenHi;

      string strShort(
        const string& title,
        const RangeQuality& qual) const
      {
        stringstream ss;
        ss << title << ": model " << index <<
          ", len peak-peak " << data->lenPP <<
          ", len range " << lenLo[qual] << " - " << lenHi[qual] << endl;
        return ss.str();
      };
    };

    struct NoneEntry
    {
      PatternEntry pe;
      vector<Peak const *> peaksBest;

      NoneEntry()
      {
        peaksBest.clear();
      };

      bool empty()
      {
        return peaksBest.empty();
      };
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
        ss << target + offset << " (" <<
          lower + offset << "-" <<
          upper + offset << "), count " <<
          count << endl;
        return ss.str();
      };
    };

    struct DoubleEntry
    {
      SingleEntry first;
      SingleEntry second;
      unsigned count;

      bool operator < (const DoubleEntry& de2)
      {
        return (count < de2.count);
      };

      string str(const unsigned offset)
      {
        stringstream ss;
        ss << first.str(offset) <<
          second.str(offset) <<
          "count " << count << endl;
        return ss.str();
      };
    };


    unsigned offset;

    CarDetect const * carBeforePtr;
    CarDetect const * carAfterPtr;

    RangeData rangeData;

    vector<ActiveEntry> activeEntries;

    list<PatternEntry> candidates;


    bool setGlobals(
      const CarModels& models,
      const PeakRange& range,
      const unsigned offsetIn);

    void getActiveModels(
      const CarModels& models,
      const bool fullFlag);

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
      const PatternType patternType);

    bool guessNoBorders();

    bool guessBothSingle(const CarModels& models);
    bool guessBothDouble(
      const CarModels& models, 
      const bool leftFlag);

    bool guessLeft( const CarModels& models);

    bool guessRight(const CarModels& models);

    void updateUnused(
      const PatternEntry& pe,
      PeakPtrs& peakPtrsUnused) const;

    void updateUsed(
      const vector<Peak const *>& peaksClose,
      PeakPtrs& peakPtrsUsed) const;

    void update(
      const NoneEntry& none,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused) const;

    void setNone(
      PatternEntry& pe,
      vector<Peak const *>& peaksBest,
      NoneEntry& none) const;

    void addToSingles(
      const vector<unsigned>& indices,
      const vector<Peak const *>& peaksClose,
      list<SingleEntry>& singles) const;

    void addToDoubles(
      const PatternEntry& pe,
      const vector<Peak const *>& peaksClose,
      list<DoubleEntry>& doubles) const;

    void examineCandidates(
      const PeakPtrs& peakPtrsUsed,
      NoneEntry& none,
      list<SingleEntry>& singles,
      list<DoubleEntry>& doubles);

    void condenseSingles(list<SingleEntry>& singles) const;

    void condenseDoubles(list<DoubleEntry>& doubles) const;

    void fixOnePeak(
      const string& text,
      const unsigned target,
      const unsigned lower,
      const unsigned upper,
      PeakPool& peaks,
      Peak *& pptr) const;

    bool fixSingles(
      PeakPool& peaks,
      list<SingleEntry>& singles,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused) const;

    bool fixDoubles(
      PeakPool& peaks,
      list<DoubleEntry>& doubles,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused) const;

    bool fix(
      PeakPool& peaks,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused);

    string strClosest(
      const vector<unsigned>& indices,
      const vector<Peak const *>& peaksClose) const;



  public:

    PeakPattern();

    ~PeakPattern();

    void reset();

    bool locate(
      const CarModels& models,
      PeakPool& peaks,
      const PeakRange& range,
      const unsigned offsetIn,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused);
};

#endif
