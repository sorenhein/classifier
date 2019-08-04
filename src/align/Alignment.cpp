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
  topResiduals.clear();
}


bool Alignment::operator < (const Alignment& a2) const
{
  // Cars have same length?
  if (numDelete + a2.numAdd == a2.numDelete + numAdd)
    return (dist < a2.dist);

  // Only look closely at good matches.
  if (distMatch > ALIGN_DISTMATCH_THRESHOLD &&
      a2.distMatch > ALIGN_DISTMATCH_THRESHOLD)
    return (dist < a2.dist);

  // If only one looks good, go with it.
  if (distMatch > ALIGN_DISTMATCH_THRESHOLD)
    return false;
  if (a2.distMatch > ALIGN_DISTMATCH_THRESHOLD)
    return true;

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


float Alignment::time2pos(const float time) const
{
  return motion.time2pos(time);
  /*
  float res = 0.;
  float pow = 1.;
  for (unsigned c = 0; c < motion.estimate.size(); c++)
  {
    res += motion.estimate[c] * pow;
    pow *= time;
  }
  return res;
  */
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

  topResiduals.clear();
  if (last != numeric_limits<unsigned>::max())
  {
    for (i = 0; i <= last; i++)
    {
      residuals[i].frac = residuals[i].valueSq / distMatch;
      topResiduals.push_back(residuals[i]);
    }
  }

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

