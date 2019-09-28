#include <iomanip>
#include <sstream>

#include "ResidualStats.h"

#include "../align/Alignment.h"

#include "../util/io.h"

#include "../const.h"


// Need this many instances in order to calculate any statistics.

#define MIN_FOR_STATISTICS 5

#define LOG_THRESHOLD 3.


ResidualStats::ResidualStats()
{
  ResidualStats::reset();
}


ResidualStats::~ResidualStats()
{
}


void ResidualStats::reset()
{
  sum.clear();
  sumSq.clear();
  count.clear();
  mean.clear();
  sdev.clear();
}


void ResidualStats::log(const Alignment& alignment)
{
  if (alignment.distMatch >= LOG_THRESHOLD)
    return;

  if (sum.size() < alignment.numAxles)
  {
    sum.resize(alignment.numAxles);
    sumSq.resize(alignment.numAxles);
    count.resize(alignment.numAxles);
    mean.resize(alignment.numAxles);
    sdev.resize(alignment.numAxles);
  }

  for (auto& re: alignment.residuals)
  {
    const unsigned i = re.refIndex;

    sum[i] += re.value;
    sumSq[i] += re.valueSq;
    count[i]++;
  }
}


void ResidualStats::calculate()
{
  numCloseToZero = 0;
  numSignificant = 0;
  numInsignificant = 0;
  avg = 0.f;
  avgAbs = 0.f;
  unsigned numAvg = 0;

  for (unsigned i = 0; i < sum.size(); i++)
  {
    if (count[i] < MIN_FOR_STATISTICS)
    {
      numInsignificant++;
      continue;
    }

    numSignificant++;
    mean[i] = sum[i] / count[i];
    sdev[i] = static_cast<float>(
      sqrt((count[i] * sumSq[i] - sum[i] * sum[i]) / 
      (count[i] * (count[i] - 1.f))));

    if (mean[i] + sdev[i] >= 0.f && mean[i] - sdev[i] <= 0.f)
      numCloseToZero++;

    avg += mean[i];
    avgAbs += abs(mean[i]);
    numAvg++;
  }

  if (numAvg < MIN_FOR_STATISTICS)
  {
    avg = 0.f;
    avgAbs = 0.f;
  }
  else
  {
    avg /= numAvg;
    avgAbs /= numAvg;
  }
}


string ResidualStats::strHeader() const
{
  stringstream ss;
  ss << 
    setw(18) << left << "Train" <<
    setw(8) << right << "~ zero" <<
    setw(8) << right << "signif" <<
    setw(8) << right << "insign" <<
    setw(8) << "Avg" <<
    setw(8) << "Avg abs" << "\n";
  return ss.str();    
}


string ResidualStats::str() const
{
 stringstream ss;
 ss << 
   setw(8) << right << numCloseToZero <<
   setw(8) << right << numSignificant <<
   setw(8) << right << numInsignificant;

 if (numSignificant == 0)
   ss <<
     setw(8) << "-" << 
     setw(8) << "-" << "\n";
 else
   ss <<
     setw(8) << fixed << setprecision(2) << avg << 
     setw(8) << fixed << setprecision(2) << avgAbs << "\n";
 return ss.str();    
}


void ResidualStats::write(const string& filename) const
{
  const unsigned lm = mean.size();
  vector<float> concat;
  concat.resize(3 * lm);

  for (unsigned i = 0; i < lm; i++)
    concat[i] = mean[i];
  for (unsigned i = 0; i < lm; i++)
    concat[lm + i] = mean[i] - sdev[i];
  for (unsigned i = 0; i < lm; i++)
    concat[2*lm + i] = mean[i] + sdev[i];

  writeBinaryFloat(filename, concat);
}

