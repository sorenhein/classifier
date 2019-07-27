#include <iomanip>
#include <sstream>

#include "Alignment.h"


Alignment::Alignment()
{
  Alignment::reset();
}


Alignment::~Alignment()
{
}


void Alignment::reset()
{
  topResiduals.clear();
}


bool Alignment::operator < (const Alignment& a2) const
{
  // Cars have same length?
  if (numDelete + a2.numAdd == a2.numDelete + numAdd)
    return (dist < a2.dist);

  // Only look closely at good matches.
  if (distMatch > 3. || a2.distMatch > 3.)
    return (dist < a2.dist);

  // One match distance is clearly better?
  if (distMatch < 0.7 * a2.distMatch)
    return true;
  else if (a2.distMatch < 0.7 * distMatch)
    return false;

  if (numDelete + a2.numAdd > a2.numDelete + numAdd)
  {
    // This car has more wheels.
    if (distMatch < a2.distMatch && dist < 2. * a2.dist)
      return true;
  }
  else
  {
    // a2 car has more wheels.
    if (a2.distMatch < distMatch && a2.dist < 2. * dist)
      return false;
  }

  // Unclear.
  return (dist < a2.dist);
}

