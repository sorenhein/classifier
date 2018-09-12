#ifndef TRAIN_PEAK_H
#define TRAIN_PEAK_H

#include <vector>
#include <string>

using namespace std;


class Peak
{
  private:

    unsigned index;
    float value;
    bool maxFlag;

    // The flank goes to the left of the peak.
    // There is a dummy peak at the end.

    float len;
    float range;
    float area;

    unsigned clusterNo;

  public:

    Peak();

    ~Peak();

    void reset();

    void log(
      const unsigned indexIn,
      const float valueIn,
      const bool maxFlagIn,
      const float lenIn,
      const float rangeIn,
      const float areaIn);

    void logCluster(const unsigned cno);

    unsigned getIndex() const;
    bool getMaxFlag() const;
    float getValue() const;
    float getArea() const;

    bool check(
      const Peak& p2,
      const unsigned offset) const;

    void operator += (const Peak& p2);

    void operator /= (const unsigned no);

    string strHeader() const;

    string str(const unsigned offset = 0) const;
};

#endif
