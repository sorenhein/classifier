#ifndef TRAIN_PEAKRANGE_H
#define TRAIN_PEAKRANGE_H

#include <list>
#include <string>

#include "PeakProfile.h"
#include "PeakPool.h"

#include "struct.h"

using namespace std;

class CarDetect;


class PeakRange2
{
  private:

    // The car after the range
    list<CarDetect>::const_iterator carAfter; 
    PeakSource source;
    unsigned start;
    unsigned end;
    bool leftGapPresent;
    bool rightGapPresent;
    bool leftOriginal;
    bool rightOriginal;

    PeakIterVector peakIters;
    PeakPtrVector peakPtrs;
    PeakProfile profile;
    

  public:

    PeakRange2();

    ~PeakRange2();

    void reset();

    void init(
      const list<CarDetect>& cars,
      const PeakPool& peaks);

    void fill(const PeakPool& peaks);

    void shortenLeft(const CarDetect& car);

    void shortenRight(
      const CarDetect& car,
      const list<CarDetect>::iterator& carIt);

    bool updateImperfections(
      const list<CarDetect>& cars,
      Imperfections& imperf) const;

    string strInterval(
      const unsigned offset,
      const string& text) const;
    
    string strPos() const;

    string strFull(const unsigned offset) const;

    string strProfile() const;
};

#endif
