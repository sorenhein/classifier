#ifndef TRAIN_CARPEAKS_H
#define TRAIN_CARPEAKS_H

#include <string>

#include "Peak.h"

using namespace std;


struct CarPeaksNumbers
{
  unsigned numFirstLeft;
  unsigned numFirstRight;
  unsigned numSecondLeft;
  unsigned numSecondRight;

  void reset()
  {
    numFirstLeft = 0;
    numFirstRight = 0;
    numSecondLeft = 0;
    numSecondLeft = 0;
  };
};

struct CarPeaksPtr
{
  Peak * firstBogieLeftPtr; 
  Peak * firstBogieRightPtr; 
  Peak * secondBogieLeftPtr; 
  Peak * secondBogieRightPtr; 
};


class CarPeaks
{
  private:

    Peak firstBogieLeft; 
    Peak firstBogieRight; 
    Peak secondBogieLeft; 
    Peak secondBogieRight; 


  public:

    CarPeaks();

    ~CarPeaks();

    void reset();

    void logAll(
      const Peak& firstBogieLeftIn,
      const Peak& firstBogieRightIn,
      const Peak& secondBogieLeftIn,
      const Peak& secondBogieRightIn);

    void operator += (const CarPeaks& cp2);

    void getPeaksPtr(CarPeaksPtr& cp);

    void increment(
      const CarPeaksPtr& cp2,
      CarPeaksNumbers& cpn);

    void increment(const CarPeaksPtr& cp2);

    void average(const CarPeaksNumbers& cpn);
    void average(const unsigned count);
};

#endif
