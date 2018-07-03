#include <iostream>
#include <iomanip>
#include <sstream>

#include "Timer.h"


Timer::Timer()
{
  Timer::reset();
}


Timer::~Timer()
{
}


void Timer::reset()
{
  no = 0;
  sum = 0.;
}


void Timer::start()
{
  begin = std::chrono::high_resolution_clock::now();
}


void Timer::stop()
{
  auto end = std::chrono::high_resolution_clock::now();
  auto delta = std::chrono::duration_cast<std::chrono::microseconds>
    (end - begin);

  no++;
  sum += delta.count();
}


void Timer::operator += (const Timer& timer2)
{
  no += timer2.no;
  sum += timer2.sum;
}


string Timer::str(const int prec) const 
{
  if (no == 0)
    return "";

  stringstream ss;
  ss << fixed << setprecision(prec) << 
    sum / 1000. << " ms";
  if (no > 1)
    ss << " sum, " << sum / (1000. * no) << " ms average";
  return ss.str();
}

