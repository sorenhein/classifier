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
  PATTERN_DOUBLE_SIDED_SINGLE_SHORT = 4,
  PATTERN_DOUBLE_SIDED_DOUBLE = 5
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
      indices[1] + offset << " - " <<
      indices[2] + offset << " - " <<
      indices[3] + offset << "\n";
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
      vector<Peak const *> peaksClose;
      bool emptyFlag;

      NoneEntry()
      {
        emptyFlag = true;
      };

      bool empty()
      {
        return emptyFlag;
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

    struct SpacingEntry
    {
      Peak const * peakLeft;
      Peak const * peakRight;
      unsigned dist;
      float numBogies; // dist relative to a typical bogie
      PeakQuality qualityLower; // Lower of the two

      string str(const unsigned offset)
      {
        stringstream ss;
        ss << "Spacing " <<
          peakLeft->getIndex() + offset << " - " <<
          peakRight->getIndex() + offset << ", bogies " <<
          setw(5) << fixed << setprecision(2) << numBogies << ", q " <<
          qualityLower << endl;
        return ss.str();
      };
    };


    unsigned offset;

    unsigned bogieTypical;
    unsigned longTypical;
    unsigned sideTypical;

    CarDetect const * carBeforePtr;
    CarDetect const * carAfterPtr;

    RangeData rangeData;

    vector<ActiveEntry> activeEntries;

    vector<Peak const *> peaksClose;

    list<PatternEntry> candidates;

    list<SpacingEntry> spacings;


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
    bool guessBothSingleShort();
    bool guessBothDouble(
      const CarModels& models, 
      const bool leftFlag);

    bool guessLeft( const CarModels& models);

    bool guessRight(const CarModels& models);

    void updateUnused(
      const PatternEntry& pe,
      PeakPtrs& peakPtrsUnused) const;

    void updateUsed(
      const vector<Peak const *>& peaksClosest,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused) const;

    void update(
      const NoneEntry& none,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused) const;

    void setNone(
      PatternEntry& pe,
      NoneEntry& none) const;

    void addToSingles(
      const vector<unsigned>& indices,
      list<SingleEntry>& singles) const;

    void addToDoubles(
      const PatternEntry& pe,
      list<DoubleEntry>& doubles) const;

    void examineCandidates(
      const PeakPtrs& peakPtrsUsed,
      NoneEntry& none,
      list<SingleEntry>& singles,
      list<DoubleEntry>& doubles);

    void condenseSingles(list<SingleEntry>& singles) const;

    void condenseDoubles(list<DoubleEntry>& doubles) const;

    void readjust(list<SingleEntry>& singles);

    void processMessage(
      const string& origin,
      const string& verb,
      const unsigned target,
      Peak *& pptr) const;
      
    void reviveOnePeak(
      const string& text,
      const SingleEntry& single,
      PeakPtrs& peakPtrsUnused,
      Peak *& pptr) const;
      
    void fixOnePeak(
      const string& text,
      const SingleEntry& single,
      PeakPool& peaks,
      Peak *& pptr) const;

    void processOnePeak(
      const string& origin,
      const SingleEntry& single,
      PeakPtrs& peakPtrsUnused,
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
      PeakPtrs& peakPtrsUnused,
      const bool flexibleFlag = false);

    void getSpacings(PeakPtrs& peakPtrsUsed);

    string strClosest(
      const vector<unsigned>& indices) const;



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
