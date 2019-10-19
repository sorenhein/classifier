#ifndef TRAIN_QUIETDATA_H
#define TRAIN_QUIETDATA_H

#include <string>

#include "trans.h"

using namespace std;


struct QuietData
{
  unsigned start;
  unsigned len;
  float mean;
  float sdev;
  QuietGrade grade;


  void setGrade();

  bool meanIsQuiet() const;

  float writeLevel() const;

  string str() const;
};

#endif
