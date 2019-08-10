#ifndef TRAIN_PEAKPROFILE_H
#define TRAIN_PEAKPROFILE_H

#include <vector>
#include <string>

#include "Peak.h"

using namespace std;

class PeakPtrs;


enum PeakSource
{
  PEAK_SOURCE_FIRST = 1,
  PEAK_SOURCE_INNER = 2,
  PEAK_SOURCE_LAST = 4,
  PEAK_SOURCE_SIZE = 8
};

struct RecogEntry
{
  bool flag;
  unsigned value;

  bool match(const unsigned actual) const
  {
    return (! flag || value == actual);
  };
};

struct RecogParams
{
  RecogEntry source;
  RecogEntry sumGreat;
  RecogEntry starsGood;
};

struct Recognizer
{
  RecogParams params;
  unsigned numWheels;
  PeakQuality quality;
  string text;
};



class PeakProfile
{
  private:

    // Peaks that are labeled both w.r.t. bogies and wheels
    // (left bogie left/right wheel, right bogie left/right wheel).
    vector<unsigned> bogieWheels;

    // Peaks that are only labeled as wheels (left/right).
    vector<unsigned> wheels;

    // Unlabeled peaks by quality: ***, **, *, poor quality.
    vector<unsigned> stars;

    unsigned sumSelected;

    unsigned sumGreat;

    unsigned sum;

    unsigned source;


    string strEntry(const unsigned value) const;


  public:

    PeakProfile();

    ~PeakProfile();

    void reset();

    void make(
      const PeakPtrs& peakPtrs,
      const unsigned source);

    unsigned numGood() const;
    unsigned numGreat() const;
    unsigned numSelected() const;
    unsigned num() const;

    bool match(const Recognizer& recog) const;

    bool looksEmpty() const;
    bool looksEmptyLast() const;
    bool looksLikeTwoCars() const;
    bool looksLong() const;

    string str() const;

    string strWarnGreat() const;
};

#endif
