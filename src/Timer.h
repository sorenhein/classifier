#ifndef TRAIN_TIMER_H
#define TRAIN_TIMER_H

#pragma warning(push)
#pragma warning(disable: 4365 4571 4625 4626 4774 5026 5027)
#include <chrono>
#pragma warning(pop)

using namespace std;


class Timer
{
  private:

    unsigned no;
    double sum;
    std::chrono::time_point<std::chrono::high_resolution_clock> begin;


  public:

    Timer();

    ~Timer();

    void reset();

    void start();

    void stop();

    void operator += (const Timer& timer2);

    bool isUsed() const;

    string str(const int prec = 1) const;
};

#endif
