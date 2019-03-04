#ifndef TRAIN_PEAKPTRLIST_H
#define TRAIN_PEAKPTRLIST_H

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


class PeakPtrListNew
{
  private:

    list<Peak *> peaks;


  public:

    PeakPtrListNew();

    ~PeakPtrListNew();

    void clear();

    void push_back(Peak * peak);

    PPLiterator begin();
    PPLiterator end();

    PPLciterator cbegin() const;
    PPLciterator cend() const;


    PPLiterator nextExcl(
      const PPLiterator& it,
      const PeakFncPtr& fptr,
      const bool softFlag);

    PPLciterator nextExcl(
      const PPLciterator& it,
      const PeakFncPtr& fptr,
      const bool softFlag) const;


    PPLiterator nextIncl(
      const PPLiterator& it,
      const PeakFncPtr& fptr) const;

    PPLciterator nextIncl(
      const PPLciterator& it,
      const PeakFncPtr& fptr) const;


    PPLiterator prevExcl(
      const PPLiterator& it,
      const PeakFncPtr& fptr,
      const bool softFlag);

    PPLciterator prevExcl(
      const PPLciterator& it,
      const PeakFncPtr& fptr,
      const bool softFlag) const;


    PPLiterator prevIncl(
      const PPLiterator& it,
      const PeakFncPtr& fptr,
      const bool softFlag) const;

    PPLciterator prevIncl(
      const PPLciterator& it,
      const PeakFncPtr& fptr,
      const bool softFlag) const;


    void extractPtrs(
      const unsigned start,
      const unsigned end,
      PeakPtrListNew& pplNew);

    void extractPtrs(
      const unsigned start,
      const unsigned end,
      const PeakFncPtr& fptr,
      PeakPtrListNew& pplNew);


    void extractIters(
      const unsigned start,
      const unsigned end,
      PeakIterListNew& pplNew);

    void extractIters(
      const unsigned start,
      const unsigned end,
      const PeakFncPtr& fptr,
      PeakIterListNew& pplNew);

};

#endif
