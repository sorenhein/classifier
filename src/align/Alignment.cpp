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


string Alignment::str() const
{
  stringstream ss;
  ss << 
    setw(24) << left << trainName <<
    setw(10) << right << fixed << setprecision(2) << dist <<
    setw(10) << right << fixed << setprecision(2) << distMatch <<
    setw(8) << numAdd <<
    setw(8) << numDelete << endl;
  return ss.str();
}


string Alignment::strTopResiduals() const
{
  if (topResiduals.empty())
    return "";

  stringstream ss;
  ss << "Top residuals\n";
  ss << 
    setw(6) << "index" <<
    setw(8) << "value" <<
    setw(8) << "valSq" <<
    setw(8) << "frac" << "\n";

  for (auto& r: topResiduals)
  {
    ss << 
      setw(6) << r.index <<
      setw(8) << fixed << setprecision(2) << r.value <<
      setw(8) << fixed << setprecision(2) << r.valueSq <<
      setw(7) << fixed << setprecision(2) << 100. * r.frac << "%\n";
  }
  return ss.str() + "\n";
}

