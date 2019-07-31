#include <iomanip>
#include <sstream>
#include <algorithm>

#include "Alignment.h"

#include "../const.h"


Alignment::Alignment()
{
  Alignment::reset();
}


Alignment::~Alignment()
{
}


void Alignment::reset()
{
  topResiduals2.clear();
}


bool Alignment::operator < (const Alignment& a2) const
{
  // Cars have same length?
  if (numDelete + a2.numAdd == a2.numDelete + numAdd)
    return (dist < a2.dist);

  // Only look closely at good matches.
  if (distMatch > ALIGN_DISTMATCH_THRESHOLD || 
      a2.distMatch > ALIGN_DISTMATCH_THRESHOLD)
    return (dist < a2.dist);

  // One match distance is clearly better?
  if (distMatch < ALIGN_DISTMATCH_BETTER * a2.distMatch)
    return true;
  else if (a2.distMatch < ALIGN_DISTMATCH_BETTER * distMatch)
    return false;

  if (numDelete + a2.numAdd > a2.numDelete + numAdd)
  {
    // This car has more wheels.
    if (distMatch < a2.distMatch && dist < ALIGN_DIST_NOT_WORSE * a2.dist)
      return true;
  }
  else
  {
    // a2 car has more wheels.
    if (a2.distMatch < distMatch && a2.dist < ALIGN_DIST_NOT_WORSE * dist)
      return false;
  }

  // Unclear.
  return (dist < a2.dist);
}


void Alignment::setTopResiduals()
{
  if (residuals.empty())
    return;

  sort(residuals.rbegin(), residuals.rend());

  const unsigned lr = residuals.size();
  const float average = distMatch / lr;

  unsigned i = 0;
  unsigned last = numeric_limits<unsigned>::max();
  while (i+1 < lr && residuals[i].valueSq > 2. * average)
  {
    if (residuals[i].valueSq > residuals[i+1].valueSq + 0.5 * average)
      last = i;
    i++;
  }

  topResiduals2.clear();
  if (last != numeric_limits<unsigned>::max())
  {
    for (i = 0; i <= last; i++)
    {
      residuals[i].frac = residuals[i].valueSq / distMatch;
      topResiduals2.push_back(residuals[i]);
    }
  }

}


string Alignment::str() const
{
  stringstream ss;
  ss << 
    setw(24) << left << trainName <<
    setw(10) << right << fixed << setprecision(2) << static_cast<double>(dist )<<
    setw(10) << right << fixed << setprecision(2) << static_cast<double>(distMatch) <<
    setw(8) << numAdd <<
    setw(8) << numDelete << endl;
  return ss.str();
}


string Alignment::strTopResiduals2() const
{
  if (topResiduals2.empty())
    return "";

  stringstream ss;
  ss << "Top residuals\n";
  ss << 
    setw(6) << "index" <<
    setw(8) << "value" <<
    setw(8) << "valSq" <<
    setw(8) << "frac" << "\n";

  for (auto& r: topResiduals2)
  {
    ss << 
      setw(6) << r.index <<
      setw(8) << fixed << setprecision(2) << r.value <<
      setw(8) << fixed << setprecision(2) << r.valueSq <<
      setw(7) << fixed << setprecision(2) << 100. * r.frac << "%\n";
  }
  return ss.str() + "\n";
}

