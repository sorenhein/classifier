#ifndef TRAIN_FILTER_H
#define TRAIN_FILTER_H

#include <vector>
#include <string>

using namespace std;

struct FilterDouble;
struct FilterFloat;


class Filter
{
  private:

    vector<float> accelFloat;
    vector<float> synthSpeed;
    vector<float> synthPos;

    unsigned startInterval;
    unsigned lenInterval;


    void integrateFloat(
      const vector<float>& integrand,
      const float sampleRate,
      const bool a2vflag, // true if accel->speed, false if speed->pos
      const unsigned startIndex,
      const unsigned len,
      vector<float>& result) const;

    void filterFloat(
      const FilterFloat& filter,
      vector<float>& integrand);

    void highpass(
      const FilterDouble& filter,
      vector<float>& integrand);

    void makeIntegratingFilter(
      const FilterFloat& filterFloatIn,
      FilterFloat& filterFloat) const;

    void makeIntegratingFilterDouble(
      const FilterDouble& filterDoubleIn,
      FilterDouble& filterDouble) const;

  public:

    Filter();

    ~Filter();

    void reset();

    void detect(
      const double sampleRate,
      const unsigned start,
      const unsigned len);

    vector<float>& getAccel();

    const vector<float>& getDeflection() const;

    void writeSpeed(const string& filename) const;

    void writePos(const string& filename) const;
};

#endif
