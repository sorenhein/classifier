#ifndef TRAIN_TIMERs_H
#define TRAIN_TIMERs_H

#pragma warning(push)
#pragma warning(disable: 4365 4571 4625 4626 4774 5026 5027)
#include <chrono>
#pragma warning(pop)

#include <vector>
#include <string>

#include "Timer.h"

using namespace std;


enum TimerName
{
  TIMER_READ = 0,
  TIMER_CONDITION = 1,
  TIMER_DETECT_PEAKS = 2,
  TIMER_PREALIGN = 3,
  TIMER_ALIGN = 4,
  TIMER_REGRESS = 5,
  TIMER_DISPLACE_SIMPLE = 6,
  TIMER_DISPLACE_COMPLEX = 7,
  TIMER_WRITE = 8,
  TIMER_SIZE = 9
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
