#ifndef TRAIN_SEGACTIVE_H
#define TRAIN_SEGACTIVE_H

#include <vector>
#include <string>

#include "struct.h"

using namespace std;


class SegActive
{
  private:

    vector<float> synthSpeed;
    vector<float> synthPos;

    Interval writeInterval;

    void integrate(
      const vector<double>& samples,
      const Interval& aint,
      const double mean);

    void compensateSpeed();

    void integrateFloat(const Interval& aint);


  public:

    SegActive();

    ~SegActive();

    void reset();

    bool detect(
      const vector<double>& samples, // TODO: Should use times[]
      const vector<Interval>& active,
      const double mean);

    void writeBinary(
      const string& origname,
      const string& speeddirname,
      const string& posdirname) const;

    string headerCSV() const;

    string strCSV() const;
};

#endif
