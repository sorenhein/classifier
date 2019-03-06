#ifndef TRAIN_PEAKPTRS_H
#define TRAIN_PEAKPTRS_H

#include <string>
#include <list>

#include "Peak.h"

using namespace std;


class Peak;

// Iterators for the pointer list itself.
typedef list<Peak *>::iterator PPLiterator;
typedef list<Peak *>::const_iterator PPLciterator;

// A list of iterators to the pointer list.
typedef list<PPLiterator> PeakIterListNew;

// Iterators for the iterator list.
typedef PeakIterListNew::iterator PILiterator;
typedef PeakIterListNew::const_iterator PILciterator;


class PeakPtrs
{
  private:

    list<Peak *> peaks;


  public:

    PeakPtrs();

    ~PeakPtrs();

    void clear();

    void push_back(Peak * peak);

    bool add(Peak * peak);

    void insert(
      PPLiterator& it,
      Peak * peak);

    void shift_down(Peak * peak);

    PPLiterator erase(PPLiterator& it);

    void erase_below(const unsigned limit);

    PPLiterator begin();
    PPLiterator end();

    PPLciterator cbegin() const;
    PPLciterator cend() const;

    unsigned size() const;
    unsigned count(const PeakFncPtr& fptr) const;
    bool empty() const;

    unsigned firstIndex() const;
    unsigned lastIndex() const;

    void getIndices(vector<unsigned>& indices) const;

    bool isFourWheeler() const;


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

    void apply(const PeakRunFncPtr& fptr);

    void flattenTODO(
      vector<Peak *>& flattened);
      


    string strQuality(
      const string& text,
      const unsigned offset,
      const PeakFncPtr& fptr = &Peak::always) const;

};

#endif
