#include <iomanip>
#include <sstream>
#include <algorithm>

#include "Alignment.h"

#include "../stats/CountStats.h"

#include "../const.h"

extern CountStats alignStats;


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


void Alignment::updateStats() const
{
  vector<unsigned> refHits;
  refHits.resize(numAxles);

  int firstHit = -1;
  int lastHit = -1;
  unsigned hitCount = 0;
  unsigned spuriousCount = 0;
  for (int a2r: actualToRef)
  {
    if (a2r >= 0)
    {
      if (firstHit == -1)
        firstHit = a2r;
      lastHit = a2r;

      refHits[a2r] = 1;
      alignStats.log("match");
      hitCount++;
    }
    else
      spuriousCount++;
  }

  unsigned missCum = 0;
  if (firstHit > 0)
  {
    alignStats.log("miss early", static_cast<unsigned>(firstHit));
    missCum += static_cast<unsigned>(firstHit);
  }

  if (lastHit > 0 && static_cast<unsigned>(lastHit) + 1 < refHits.size())
  {
    const unsigned m = static_cast<unsigned>(refHits.size() - lastHit - 1);
    alignStats.log("miss late", m);
    missCum += m;
  }

  if (missCum + hitCount < numAxles)
    alignStats.log("miss within", numAxles - missCum - hitCount);

  unsigned i = 0;
  while (i < actualToRef.size() && actualToRef[i] == -1)
    i++;

  unsigned spuriousCum = 0;
  if (i > 0)
  {
    alignStats.log("spurious early", i);
    spuriousCum += i;
  }

  i = actualToRef.size();
  unsigned j = 0;
  while (i >= 1 && actualToRef[i-1] == -1)
  {
    i--;
    j++;
  }

  if (j > 0)
  {
    alignStats.log("spurious early", j);
    spuriousCum += j;
  }

  if (spuriousCum < spuriousCount)
    alignStats.log("spurious within", spuriousCount - spuriousCum);
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


string Alignment::strDeviation() const
{
  if (distMatch <= 1)
    return "<= 1";
  else if (distMatch <= 3)
    return "1-3";
  else if (distMatch <= 10)
    return "3-10";
  else if (distMatch <= 100)
    return "10-100";
  else
    return "100+";
}

