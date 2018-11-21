#ifndef TRAIN_PEAKPROFILE_H
#define TRAIN_PEAKPROFILE_H

#include <vector>
#include <string>

#include "struct.h"

using namespace std;

class Peak;


class PeakProfile
{
  private:

    // Peaks that are labeled both w.r.t. bogeys and wheels
    // (left bogey left/right wheel, right bogey left/right wheel).
    vector<unsigned> bogeyWheels;

    // Peaks that are only labeled as wheels (left/right).
    vector<unsigned> wheels;

    // Unlabeled peaks by quality: ***, **, *, poor quality.
    vector<unsigned> stars;

    unsigned sumGreat;

    unsigned sum;

    unsigned source;


    string strEntry(const unsigned value) const;


  public:

    PeakProfile();

    ~PeakProfile();

    void reset();

    void make(
      const vector<Peak *>& peakPtrs,
      const unsigned source);

    bool match(const Recognizer& recog) const;

    bool looksEmpty() const;
    bool looksLikeTwoCars() const;

    string str() const;
};

#endif
