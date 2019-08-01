#ifndef TRAIN_PEAKSPACING_H
#define TRAIN_PEAKSPACING_H

#include <vector>
#include <list>
#include <sstream>

#include "PeakRange.h"
#include "Peak.h"

using namespace std;

class CarModels;
class PeakRange;
class PeakPool;
class PeakPtrs;


class PeakSpacing
{
  private:

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

    RangeData rangeData;

    vector<SpacingEntry> spacings;


    bool setGlobals(
      const CarModels& models,
      const PeakRange& range,
      const unsigned offsetIn);

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

    void update(
      vector<Peak const *>& peaksClose,
      const unsigned limitLower,
      const unsigned limitUpper,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused) const;

    void getSpacings(PeakPtrs& peakPtrsUsed);


  public:

    PeakSpacing();

    ~PeakSpacing();

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
