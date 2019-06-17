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
      bool fullFlag;

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


    unsigned offset;

    unsigned bogieTypical;
    unsigned longTypical;
    unsigned sideTypical;

    CarDetect const * carBeforePtr;
    CarDetect const * carAfterPtr;

    RangeData rangeData;

    vector<ActiveEntry> activeEntries;

    list<Target> targets;

    Completions completions;


    bool setGlobals(
      const CarModels& models,
      const PeakRange& range,
      const unsigned offsetIn);

    void getActiveModels(const CarModels& models);

    bool addModelTargets(
      const CarModels& models,
      const unsigned indexModel,
      const bool symmetryFlag,
      const BordersType patternType);

    bool guessNoBorders();

    bool guessBothSingle(const CarModels& models);
    bool guessBothSingleShort();

    bool guessBothDouble(
      const CarModels& models, 
      const bool leftFlag);
    bool guessBothDoubleLeft(const CarModels& models);
    bool guessBothDoubleRight(const CarModels& models);

    bool guessLeft(const CarModels& models);

    bool guessRight(const CarModels& models);

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

    void targetsToCompletions(const PeakPtrs& peakPtrsUsed);

    // TODO Move, don't delete
    // void readjust(list<SingleEntry>& singles);

    void annotateCompletions(
      PeakPool& peaks,
      PeakPtrs& peakPtrsUnused,
      const bool forceFlag);

    void fillCompletions(
      PeakPool& peaks,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused,
      const bool forceFlag);

    bool fix(
      PeakPool& peaks,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused,
      const bool forceFlag = false,
      const bool flexibleFlag = false);

    string strClosest(
      vector<Peak const *>& peaksClose,
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
