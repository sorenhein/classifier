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
  names[TIMER_TRANSIENT] = "Transient";
  names[TIMER_CONDITION] = "Condition";
  names[TIMER_DETECT_PEAKS] = "Detect peaks";
  names[TIMER_ALIGN] = "Alignment";
  names[TIMER_REGRESS] = "Regression";
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
  ss << setw(24) << left << "Name" << 
    setw(10) << right << "Sum" << 
    setw(10) << right << "Average" << "\n";
  for (unsigned i = 0; i < timers.size(); i++)
  {
    if (! timers[i].empty())
      ss << setw(24) << left << names[i] << timers[i].str(prec);
  }
  return ss.str();
}

