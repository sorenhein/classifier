#ifndef TRAIN_TRANS_H
#define TRAIN_TRANS_H


enum TransientStatus
{
  TSTATUS_TOO_FEW_RUNS = 0,
  TSTATUS_NO_CANDIDATE_RUN = 1,
  TSTATUS_NO_FULL_DECLINE = 2,
  TSTATUS_NO_MID_DECLINE = 3,
  TSTATUS_BACK_CORRECTED = 4,
  TSTATUS_FRONT_ACTUAL_CORR = 5,
  TSTATUS_BACK_SYNTH_CORR = 6,
  TSTATUS_BAD_FIT = 7,
  TSTATUS_SIZE = 8
};


enum TransientType
{
  TRANSIENT_NONE = 0,
  TRANSIENT_FOUND = 1,
  TRANSIENT_SIZE = 6
};


struct Run
{
  unsigned first;
  unsigned len;
  bool posFlag;
  float cum;
};

#endif
