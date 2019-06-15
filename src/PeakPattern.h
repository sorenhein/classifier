#ifndef TRAIN_PEAKPATTERN_H
#define TRAIN_PEAKPATTERN_H

#include <vector>
#include <list>
#include <sstream>

#include "CarModels.h"
#include "struct.h"

#include "util/Target.h"
#include "util/Completions.h"

using namespace std;

class CarModels;
class CarDetect;
class PeakRange;
class PeakPool;


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
      Target pe;

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

    struct SpacingEntry
    {
      Peak const * peakLeft;
      Peak const * peakRight;
      unsigned dist;
      float numBogies; // dist relative to a typical bogie
      bool bogieLikeFlag;
      PeakQuality qualityShapeLower; // Lower of the two
      PeakQuality qualityWholeLower; // Lower of the two

      string str(const unsigned offset)
      {
        stringstream ss;
        ss << "Spacing " <<
          peakLeft->getIndex() + offset << " - " <<
          peakRight->getIndex() + offset << ", bogies " <<
          setw(5) << fixed << setprecision(2) << numBogies << ", q " <<
          static_cast<unsigned>(qualityWholeLower) << " " <<
          static_cast<unsigned>(qualityShapeLower) << ", like " << 
          bogieLikeFlag << endl;
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

    list<Target> targets;

    Completions completions;

    vector<SpacingEntry> spacings;


    bool setGlobals(
      const CarModels& models,
      const PeakRange& range,
      const unsigned offsetIn);

    void getActiveModels(
      const CarModels& models,
      const bool fullFlag);

    bool fillFromModel(
      const CarModels& models,
      const unsigned indexModel,
      const bool symmetryFlag,
      const unsigned indexRangeLeft,
      const unsigned indexRangeRight,
      const BordersType patternType);

    bool guessNoBorders();

    bool guessBothSingle(const CarModels& models);
    bool guessBothSingleShort();
    bool guessBothDouble(
      const CarModels& models, 
      const bool leftFlag);

    bool guessLeft(const CarModels& models);

    bool guessRight(const CarModels& models);

    bool plausibleCar(
      const bool leftFlag,
      const unsigned index1,
      const unsigned index2) const;

    void fixShort(
      const string& text,
      const unsigned indexFirst,
      const unsigned indexLast,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused) const;

    bool guessAndFixShortFromSpacings(
      const string& text,
      const bool leftFlag,
      const unsigned indexFirst,
      const unsigned indexLast,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused) const;

    bool guessAndFixShort(
      const bool leftFlag,
      const unsigned indexFirst,
      const unsigned indexLast,
      PeakPool& peaks,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused) const;

    bool guessAndFixShortLeft(
      PeakPool& peaks,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused) const;

    bool guessAndFixShortRight(
      PeakPool& peaks,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused) const;

    bool looksEmptyFirst(const PeakPtrs& peakPtrsUsed) const;

    void updateUnused(
      const Target& pe,
      PeakPtrs& peakPtrsUnused) const;

    void updateUnused(
      const unsigned limitLower,
      const unsigned limitUpper,
      PeakPtrs& peakPtrsUnused) const;

    void updateUsed(
      const vector<Peak const *>& peaksClosest,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused) const;

    void update(
      const NoneEntry& none,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused) const;

    void update(
      const vector<Peak const *>& closest,
      const unsigned limitLower,
      const unsigned limitUpper,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused) const;

    void addToSingles(const vector<unsigned>& indices);

    void addToDoubles(const Target& target);

    void addToTriples(const Target& target);

    bool checkDoubles(const Target& target) const;

    void examineTargets(const PeakPtrs& peakPtrsUsed);

    // TODO Move, don't delete
    // void readjust(list<SingleEntry>& singles);

    void processMessage(
      const string& origin,
      const string& verb,
      const unsigned target,
      Peak *& pptr) const;
      
    bool fix(
      PeakPool& peaks,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused,
      const bool forceFlag = false,
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
