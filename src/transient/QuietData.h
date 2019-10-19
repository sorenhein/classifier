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
  bool _meanIsQuiet;


  void setGrade(
    const float& meanSomewhatQuiet,
    const float& meanQuietLimit,
    const float& SdevMeanFactor);

  bool meanIsQuiet() const;

  string str() const;
};

#endif
