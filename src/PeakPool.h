#ifndef TRAIN_PEAKPOOL_H
#define TRAIN_PEAKPOOL_H

/*
   We keep several layers of peaks here.  The first layer is the
   set of raw peaks (actually we process those in place, as there
   are so many and we don't want to copy them).  The next layers
   are various levels of processing, e.g. after eliminating tiny
   areas or certain kinks.

   We keep these layers as it is sometimes necessary to go back and
   undo a certain aggregation locally (for a certain set of samples).  
   For instance, something may have looked like a kink,  but it really 
   was a peak.

   The user calls copy() to make a new layer and to leave the
   processed layer in place at this point.  Other than that,
   processing happens on the current layer and modifies it.
   The current layer mostly behaves like a normal list.

   It is probably possible to do the whole class "in place", i.e.
   to reconstruct peaks from information in neighboring peaks.
   This would save some time and space from copying.
 */


#include <vector>
#include <list>
#include <string>

#include "Peak.h"
#include "struct.h"

using namespace std;

typedef list<Peak> PeakList;
typedef PeakList::iterator Piterator;
typedef PeakList::const_iterator Pciterator;

typedef list<Peak *> PeakPtrList;
typedef PeakPtrList::iterator PPiterator;
typedef vector<Peak *> PeakPtrVector;


class PeakPool
{
  private:

    list<PeakList> peakLists;

    PeakList * peaks;

    // Could have one for each PeakList as well.
    PeakPtrList candidates;


    float getFirstPeakTime() const;


  public:

    PeakPool();

    ~PeakPool();

    void clear();

    bool empty() const;

    unsigned size() const;

    Piterator begin() const;
    Pciterator cbegin() const;

    Piterator end() const;
    Pciterator cend() const;

    Peak& front();
    Peak& back();

    void extend(); // Like emplace_back()

    void copy();

    Piterator erase(
      Piterator pit1,
      Piterator pit2);

    Piterator collapse(
      Piterator pit1,
      Piterator pit2);


    void makeCandidates();

    void getCandPtrs(
      const unsigned start,
      const unsigned end,
      PeakPtrVector& peakPtrs) const;

    unsigned firstCandIndex() const;
    unsigned lastCandIndex() const;

    PPiterator nextCandExcl(
      PPiterator& pit,
      const PeakFncPtr& fptr) const;

    PPiterator nextCandIncl(
      PPiterator& pit,
      const PeakFncPtr& fptr) const;

    PPiterator prevCandExcl(
      PPiterator& pit,
      const PeakFncPtr& fptr) const;

    PPiterator prevCandIncl(
      PPiterator& pit,
      const PeakFncPtr& fptr) const;


    void getSelectedSamples(vector<float>& selected) const;

    void getSelectedTimes(vector<PeakTime>& times) const;


    string strAll(
      const string& text,
      const unsigned& offset) const;

    string strSelectedTimesCSV(const string& text) const;

    string strCounts() const;
};

#endif
