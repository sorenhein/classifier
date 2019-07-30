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

    double time2pos(
      const double& time,
      const vector<double>& coeffs) const;

    double residuals(
      const vector<double>& x,
      const vector<double>& y,
      const vector<double>& coeffs) const;

    void summarizeResiduals(
      const vector<double>& times,
      const TrainDB& trainDB,
      const vector<double>& coeffs,
      Alignment& match) const;


  public:

    Regress();

    ~Regress();

    void specificMatch(
      const vector<double>& times,
      const TrainDB& trainDB,
      const Alignment& match,
      vector<double>& coeffs,
      double& residuals) const;

    void bestMatch(
      const vector<double>& times,
      const TrainDB& trainDB,
      const Control& control,
      vector<Alignment>& matches,
      Alignment& bestAlign,
      Motion& motion) const;

};

#endif
