#include "Candidate.h"

#include "../const.h"


Candidate::Candidate()
{
}


Candidate::~Candidate()
{
}


void Candidate::calcRuns(const vector<float>& samples)
{
  // A run is a sequence of samples with the same sign.
  Run run;
  run.first = 0;
  run.len = 1;
  if (samples[0] >= 0.)
  {
    run.posFlag = true;
    run.cum = samples[0];
  }
  else
  {
    run.posFlag = false;
    run.cum = -samples[0];
  }

  const unsigned l = samples.size();
  for (unsigned i = 1; i < l; i++)
  {
    if (run.posFlag)
    {
      if (samples[i] >= 0.)
      {
        run.len++;
        run.cum += samples[i];
      }
      else
      {
        runs.push_back(run);
        run.first = i;
        run.len = 1;
        run.posFlag = false;
        run.cum = -samples[i];
      }
    }
    else if (samples[i] < 0.)
    {
      run.len++;
      run.cum -= samples[i];
    }
    else
    {
      runs.push_back(run);
      run.first = i;
      run.len = 1;
      run.posFlag = true;
      run.cum = samples[i];
    }

    // Enough runs?
    if (runs.size() >= TRANSIENT_RANGE)
      return;
  }
  runs.push_back(run);
}


bool Candidate::detectPossibleRun(
  const double sampleRate,
  unsigned& rno,
  TransientStatus& transientStatus)
{
  if (runs.size() < TRANSIENT_RANGE)
  {
    transientStatus = TSTATUS_TOO_FEW_RUNS;
    return false;
  }

  for (unsigned i = 0; i < TRANSIENT_RANGE; i++)
  {
    if (runs[i].len / sampleRate < TRANSIENT_MIN_DURATION)
      continue;

    if (runs[i].cum < TRANSIENT_MIN_AVG * runs[i].len)
      continue;
    
    rno = i;
    return true;
  } 

  transientStatus = TSTATUS_NO_CANDIDATE_RUN;
  return false;
}



bool Candidate::detect(
  const vector<float>& samples,
  const double sampleRate,
  Run& run,
  TransientType& transientType,
  TransientStatus& transientStatus)
{
  runs.clear();

  // Chop up the samples into runs (whose values have the same signs).
  Candidate::calcRuns(samples);

  unsigned rno;
  if (! Candidate::detectPossibleRun(sampleRate, rno, transientStatus))
  {
    transientType = TRANSIENT_NONE;
    return false;
  }

  run = runs[rno];
  return true;
}

