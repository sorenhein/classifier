#include <iostream>
#include <iomanip>
#include <sstream>

#include "Timers.h"


Timers::Timers()
{
  Timers::reset();
}


Timers::~Timers()
{
}


void Timers::reset()
{
  timers.resize(TIMER_SIZE);
  names.resize(TIMER_SIZE);

  for (auto& t: timers)
    t.reset();
  
  names[TIMER_READ] = "Read";
  names[TIMER_CONDITION] = "Condition";
  names[TIMER_DETECT_PEAKS] = "Detect peaks";
  names[TIMER_PREALIGN] = "Pre-alignment";
  names[TIMER_ALIGN] = "Alignment";
  names[TIMER_REGRESS] = "Regression";
  names[TIMER_DISPLACE_SIMPLE] = "Simple displacement";
  names[TIMER_DISPLACE_COMPLEX] = "Complex displacement";
  names[TIMER_WRITE] = "Write";
}


void Timers::start(const TimerName tname)
{
  timers[tname].start();
}


void Timers::stop(const TimerName tname)
{
  timers[tname].stop();
}


string Timers::str(const int prec) const 
{
  stringstream ss;
  ss << setw(24) << left << "Name" << "Time" << "\n";
  for (auto& t: timers)
    ss << t.str(prec) << endl;;
  return ss.str();
}

