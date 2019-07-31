#ifndef TRAIN_REGRESS_H
#define TRAIN_REGRESS_H

#include <vector>
#include <string>

#include "struct.h"

using namespace std;

class TrainDB;
class Control;
struct Alignment;
struct Motion;


class Regress
{
  private:

    float time2pos(
      const float& time,
      const vector<float>& coeffs) const;

    float residuals(
      const vector<float>& x,
      const vector<float>& y,
      const vector<float>& coeffs) const;

    void summarizeResiduals(
      const vector<float>& times,
      const vector<float>& refPeaks,
      const vector<float>& coeffs,
      Alignment& match) const;


  public:

    Regress();

    ~Regress();

    void specificMatch(
      const vector<float>& times,
      const vector<float>& refPeaks,
      Alignment& match) const;

    void bestMatch(
      const vector<float>& times,
      const TrainDB& trainDB,
      const Control& control,
      vector<Alignment>& matches) const;

};

#endif
