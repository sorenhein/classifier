#ifndef TRAIN_REGRESS_H
#define TRAIN_REGRESS_H

#include <vector>
#include <string>

#include "struct.h"

using namespace std;

class Database;


class Regress
{
  private:

    double residuals(
      const vector<double>& x,
      const vector<double>& y,
      const vector<double>& coeffs) const;


  public:

    Regress();

    ~Regress();

    void bestMatch(
      const vector<PeakTime>& times,
      const Database& db,
      const unsigned order,
      const Control& control,
      vector<Alignment>& matches,
      Alignment& bestAlign,
      vector<double>& motionEstimate) const;

};

#endif
