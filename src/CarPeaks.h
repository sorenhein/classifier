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
};

struct CarPeaksPtr
{
 Peak * firstBogeyLeftPtr; 
 Peak * firstBogeyRightPtr; 
 Peak * secondBogeyLeftPtr; 
 Peak * secondBogeyRightPtr; 
};


class CarPeaks
{
  private:

    Peak firstBogeyLeft; 
    Peak firstBogeyRight; 
    Peak secondBogeyLeft; 
    Peak secondBogeyRight; 


  public:

    CarPeaks();

    ~CarPeaks();

    void reset();

    void logAll(
      const Peak& firstBogeyLeftIn,
      const Peak& firstBogeyRightIn,
      const Peak& secondBogeyLeftIn,
      const Peak& secondBogeyRightIn);

    void operator += (const CarPeaks& cp2);

    void getPeaksPtr(CarPeaksPtr& cp);

    void increment(
      const CarPeaksPtr& cp2,
      CarPeaksNumbers& cpn);

    void average(const CarPeaksNumbers& cpn);



};

#endif
