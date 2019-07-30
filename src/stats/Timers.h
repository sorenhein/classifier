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
  TIMER_TRANSIENT = 1,
  TIMER_QUIET = 2,
  TIMER_FILTER = 3,

  TIMER_EXTRACT_PEAKS = 4,
  TIMER_LABEL_PEAKS = 5,
  TIMER_EXTRACT_CARS = 6,
  TIMER_ALIGN = 7,
  TIMER_REGRESS = 8,
  TIMER_WRITE = 9,
  TIMER_ALL_THREADS = 10,
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

    void operator += (const Timers& timers2);

    string str(const int prec = 1) const;
};

#endif
