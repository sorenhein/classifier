/*
 *  A simple scheduler, e.g. to return trace numbers when
 *  multi-threading.
 */

#ifndef TRAIN_SCHEDULER_H
#define TRAIN_SCHEDULER_H

#include <atomic>

using namespace std;


class Scheduler
{
  private:

    atomic<int> currNumber;

    unsigned max;


  public:

    Scheduler() 
    { 
      Scheduler::reset(); 
    };

    ~Scheduler() {};

    void reset() 
    { 
      currNumber = -1; 
      max = 0; 
    };

    void setMax(const unsigned m) 
    { 
      max = m; 
    };

    bool next(unsigned& number) 
    {
      int i = ++currNumber;
      number = static_cast<unsigned>(i);
      return (number < max);
    };
};

#endif
