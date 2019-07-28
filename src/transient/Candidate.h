/*
 * A run is a sequence of samples whose values have the same sign.
 * A run is a possible transient if it is among the first
 * TRANSIENT_RANGE runs, covers at least TRANSIENT_MIN_DURATION
 * seconds, and has an average of at leave TRANSIENT_MIN_AVG g's.
 */

#ifndef TRAIN_CANDIDATE_H
#define TRAIN_CANDIDATE_H

#include <vector>

#include "trans.h"

using namespace std;


struct Run
{
  unsigned first;
  unsigned len;
  bool posFlag;
  float cum;
};



class Candidate
{
  private:

    vector<Run> runs;


    void calcRuns(const vector<float>& samples);

    bool detectPossibleRun(
      const double sampleRate,
      unsigned& rno,
      TransientStatus& transientStatus);


  public:

    Candidate();

    ~Candidate();

    bool detect(
      const vector<float>& samples,
      const double sampleRate,
      Run& run,
      TransientType& transientType,
      TransientStatus& transientStatus);
};

#endif
