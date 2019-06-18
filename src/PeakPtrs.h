#ifndef TRAIN_PEAKPTRS_H
#define TRAIN_PEAKPTRS_H

#include <string>
#include <list>

#include "Peak.h"
#include "CarPeaks.h"

using namespace std;


class Peak;

// Iterators for the pointer list itself.
typedef list<Peak *>::iterator PPLiterator;
typedef list<Peak *>::const_iterator PPLciterator;


class PeakPtrs
{
  private:

    list<Peak *> peaks;


  public:

    PeakPtrs();

    ~PeakPtrs();

    void clear();

    void push_back(Peak * peak);

    void assign(
      const unsigned num,
      Peak * const peak);

    bool add(Peak * peak);
    bool remove(Peak const * peak);

    void insert(
      PPLiterator& it,
      Peak * peak);

    void moveFrom(PeakPtrs& fromPtrs);

    void shift_down(Peak * peak);

    PPLiterator erase(PPLiterator& it);

    void erase_below(const unsigned limit);
    void erase_above(const unsigned limit);

    PPLiterator begin();
    PPLiterator end();

    PPLciterator cbegin() const;
    PPLciterator cend() const;

    Peak const * front() const;
    Peak const * back() const;

    unsigned size() const;
    unsigned count(const PeakFncPtr& fptr) const;
    bool empty() const;

    unsigned firstIndex() const;
    unsigned lastIndex() const;

    void getIndices(vector<unsigned>& indices) const;

    bool isFourWheeler(const bool perfectFlag = false) const;


    PPLiterator next(
      const PPLiterator& it,
      const PeakFncPtr& fptr = &Peak::always,
      const bool exclFlag = true);

    PPLciterator next(
      const PPLciterator& it,
      const PeakFncPtr& fptr = &Peak::always,
      const bool exclFlag = true) const;


    PPLiterator prev(
      const PPLiterator& it,
      const PeakFncPtr& fptr = &Peak::always,
      const bool exclFlag = true);

    PPLciterator prev(
      const PPLciterator& it,
      const PeakFncPtr& fptr = &Peak::always,
      const bool exclFlag = true) const;


    Peak * locate(
      const unsigned lower,
      const unsigned upper,
      const unsigned hint,
      const PeakFncPtr& fptr,
      unsigned& indexUsed) const;

    void getClosest(
      const vector<unsigned>& indices,
      vector<Peak *>& peaksClose,
      unsigned& numClose,
      unsigned& distance);

    void fill(
      const unsigned start,
      const unsigned end,
      PeakPtrs& pplNew,
      list<PPLciterator>& pilNew);

    void split(
      const PeakFncPtr& fptr1,
      const PeakFncPtr& fptr2,
      PeakPtrs& peaksMatched,
      PeakPtrs& peaksRejected) const;

    void split(
      const vector<Peak const *>& flattened,
      PeakPtrs& peaksMatched,
      PeakPtrs& peaksRejected);

    void truncate(
      const unsigned limitLower,
      const unsigned limitUpper);

    void apply(const PeakRunFncPtr& fptr);

    void markup();

    void sort();

    void flattenCar(CarPeaksPtr& carPeaksPtr);

    void flatten(vector<Peak const *>& flattened);

    void makeDistances(
      const PeakFncPtr& fptr1, // true of left peak
      const PeakFncPtr& fptr2, // true of right peak
      const PeakPairFncPtr& pairPtr, // true of peak pair
      vector<unsigned>& distances) const;

    bool check() const;

    string strQuality(
      const string& text,
      const unsigned offset,
      const PeakFncPtr& fptr = &Peak::always) const;

};

#endif
