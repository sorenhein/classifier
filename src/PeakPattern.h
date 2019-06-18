#ifndef TRAIN_PEAKPATTERN_H
#define TRAIN_PEAKPATTERN_H

#include <vector>
#include <list>

#include "PeakRange.h"

#include "util/Target.h"
#include "util/Completions.h"

using namespace std;

class CarModels;
class CarDetect;
class PeakPool;
struct ModelData;


class PeakPattern
{
  private:

    struct ModelActive
    {
      ModelData const * data;
      unsigned index;
      bool fullFlag;

      // Interval for different range qualities.
      vector<unsigned> lenLo;
      vector<unsigned> lenHi;
    };


    unsigned offset;

    unsigned bogieTypical;
    unsigned longTypical;
    unsigned sideTypical;

    CarDetect const * carBeforePtr;
    CarDetect const * carAfterPtr;

    RangeData rangeData;

    vector<ModelActive> modelsActive;

    list<Target> targets;

    Completions completions;


    void setMethods();

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

    bool guessBothDouble(
      const CarModels& models, 
      const bool leftFlag);

    bool guessNoBorders(const CarModels& models);
    bool guessBothSingle(const CarModels& models);
    bool guessBothSingleShort(const CarModels& models);
    bool guessBothDoubleLeft(const CarModels& models);
    bool guessBothDoubleRight(const CarModels& models);
    bool guessLeft(const CarModels& models);
    bool guessRight(const CarModels& models);

    bool looksEmptyFirst(const PeakPtrs& peakPtrsUsed) const;

    void update(
      const vector<Peak const *>& closest,
      const unsigned limitLower,
      const unsigned limitUpper,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused) const;

    void targetsToCompletions(PeakPtrs& peakPtrsUsed);

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
      const bool forceFlag = false);


  public:

    PeakPattern();

    ~PeakPattern();

    void reset();

    FindCarType locate(
      const CarModels& models,
      const PeakRange& range,
      const unsigned offsetIn,
      PeakPool& peaks,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused);
};

#endif
