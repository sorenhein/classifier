#ifndef TRAIN_TIMERs_H
#define TRAIN_TIMERs_H

#pragma warning(push)
#pragma warning(disable: 4365 4571 4625 4626 4774 5026 5027)
#include <chrono>
#pragma warning(pop)

#include <vector>
#include <string>

#include "util/Timer.h"

using namespace std;


enum TimerName
{
  TIMER_READ = 0,
  TIMER_TRANSIENT = 1,
  TIMER_ENDS = 2,
  TIMER_CONDITION = 3,
  TIMER_DETECT_PEAKS = 4,
  TIMER_PREALIGN = 5,
  TIMER_ALIGN = 6,
  TIMER_REGRESS = 7,
  TIMER_DISPLACE_SIMPLE = 8,
  TIMER_DISPLACE_COMPLEX = 9,
  TIMER_WRITE = 10,
  TIMER_SIZE = 11
};


class Timers
{
  private:

    vector<Timer> timers;
    vector<string> names;


  public:

    Timers();

    ~Timers();

    void reset();

    void start(const TimerName tname);

    void stop(const TimerName tname);

    string str(const int prec = 1) const;
};

#endif
