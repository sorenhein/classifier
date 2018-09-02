#ifndef TRAIN_ARGS_H
#define TRAIN_ARGS_H

#include "struct.h"


void usage(const char base[]);

void printOptions(const Control& control);

void readArgs(
  int argc,
  char * argv[],
  Control& control);

#endif

