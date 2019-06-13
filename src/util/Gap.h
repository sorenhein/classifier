#ifndef TRAIN_GAP_H
#define TRAIN_GAP_H

#include <string>

using namespace std;


class Gap
{
  private:

    unsigned mid;
    unsigned lower;
    unsigned upper;
    unsigned _count;

    void setLocation(const unsigned midIn);

    bool openLeft() const;
    bool openRight() const;

  public:

    Gap();

    ~Gap();

    void reset();

    void set(
      const unsigned midIn,
      const unsigned countIn);

    void expand(const unsigned value);

    bool extend(const unsigned midIn);

    void recenter(const Gap& g2);

    unsigned center() const;

    unsigned count() const;

    float ratio(const unsigned value) const;

    bool empty() const;

    bool startsBelow(const unsigned value) const;
    bool wellBelow(const unsigned value) const;
    bool wellAbove(const unsigned value) const;

    bool overlap(
      const unsigned shortest,
      const unsigned longest) const;

    bool overlap(const Gap& g2) const;

    string str(const string& text = "") const;
};

#endif
