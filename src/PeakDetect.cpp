#include <list>
#include <iostream>
#include <iomanip>
#include <functional>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <random>

#include "PeakDetect.h"
#include "PeakCluster.h"
#include "Except.h"

#define SMALL_AREA_FACTOR 100.f

// Only a check limit, not algorithmic parameters.
#define ABS_RANGE_LIMIT 1.e-4
#define ABS_AREA_LIMIT 1.e-4

#define SAMPLE_RATE 2000.

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))

PeakDetect::PeakDetect()
{
  PeakDetect::reset();
}


PeakDetect::~PeakDetect()
{
}


void PeakDetect::reset()
{
  len = 0;
  peakList.clear();
}


float PeakDetect::integral(
  const vector<float>& samples,
  const unsigned i0,
  const unsigned i1) const
{
  float sum = 0.f;
  for (unsigned i = i0; i < i1; i++)
    sum += samples[i];

  return sum;
}


void PeakDetect::annotate()
{
  if (peakList.size() < 2)
    THROW(ERR_NO_PEAKS, "Too few peaks: " + to_string(peakList.size()));

  const auto peakFirst = peakList.begin();
  const auto peakLast = prev(peakList.end());

  for (auto it = peakFirst; it != peakList.end(); it++)
  {
    const Peak * peakPrev = (it == peakFirst ? nullptr : &*prev(it));
    const Peak * peakNext = (it == peakLast ? nullptr : &*next(it));

    it->annotate(peakPrev, peakNext);
  }
}


bool PeakDetect::check(const vector<float>& samples) const
{
  if (samples.size() == 0)
    THROW(ERR_SHORT_ACCEL_TRACE, "Accel trace length: " + to_string(len));

  if (peakList.size() < 2)
    THROW(ERR_NO_PEAKS, "Too few peaks: " + to_string(peakList.size()));

  const auto peakFirst = peakList.begin();
  const auto peakLast = prev(peakList.end());
  bool flag = true;

  for (auto it = peakFirst; it != peakList.end(); it++)
  {
    Peak peakSynth;
    Peak const * peakPrev = nullptr, * peakNext = nullptr;
    float areaFull = 0.f;
    const unsigned index = it->getIndex();

    if (it != peakFirst)
    {
      peakPrev = &*prev(it);
      areaFull = peakPrev->getAreaCum() + 
        PeakDetect::integral(samples, peakPrev->getIndex(), index);
    }

    if (it != peakLast)
      peakNext = &*next(it);

    peakSynth.log(index, it->getValue(), areaFull, it->getMaxFlag());
    peakSynth.annotate(peakPrev, peakNext);

    if (! it->check(peakSynth, offset))
      flag = false;
  }
  return flag;
}


void PeakDetect::log(
  const vector<float>& samples,
  const unsigned offsetSamples)
{
  len = samples.size();
  if (len < 2)
    THROW(ERR_SHORT_ACCEL_TRACE, "Accel trace length: " + to_string(len));

  offset = offsetSamples;
  peakList.clear();

  // We put a sentinel peak at the beginning.
  peakList.emplace_back(Peak());
  peakList.back().logSentinel(samples[0]);
  bool maxFlag = false;

  for (unsigned i = 1; i < len-1; i++)
  {
    while (i < len-1 && samples[i] == samples[i-1])
      i++;

    if (i == len-1)
      break;

    if (samples[i] > samples[i-1])
    {
      // Use the last of equals as the starting point.
      while (i < len-1 && samples[i] == samples[i+1])
        i++;

      if (i == len-1 || samples[i] <= samples[i+1])
        continue;
      else
        maxFlag = true;
    }
    else
    {
      while (i < len-1 && samples[i] == samples[i+1])
        i++;

      if (i == len-1 || samples[i] >= samples[i+1])
        continue;
      else
        maxFlag = false;
    }

    const auto& peakPrev = peakList.back();
    const float areaFull = PeakDetect::integral(samples, 
      peakPrev.getIndex(), i);
    const float areaCumPrev = peakPrev.getAreaCum();

    // The peak contains data for the interval preceding it.
    peakList.emplace_back(Peak());
    peakList.back().log(i, samples[i], areaCumPrev + areaFull, maxFlag);
  }

  // We put another peak at the end.
  const auto& peakPrev = peakList.back();
  const float areaFull = 
    PeakDetect::integral(samples, peakPrev.getIndex(), len-1);
  const float areaCumPrev = peakPrev.getAreaCum();

  peakList.emplace_back(Peak());
  peakList.back().log(
    len-1, 
    samples[len-1], 
    areaCumPrev + areaFull, 
    ! peakPrev.getMaxFlag());

  PeakDetect::annotate();

  PeakDetect::check(samples);
}


void PeakDetect::collapsePeaks(
  list<Peak>::iterator peak1,
  list<Peak>::iterator peak2)
{
  // Analogous to list.erase(), peak1 does not survive, while peak2 does.
  if (peak1 == peak2)
    return;

  // Need same polarity.
  if (peak1->getMaxFlag() != peak2->getMaxFlag())
    THROW(ERR_PEAK_COLLAPSE, "Collapsing peaks with different polarity");

  Peak * peak0 = 
    (peak1 == peakList.begin() ? &*peak1 : &*prev(peak1));
  Peak * peakN = (next(peak2) == peakList.end() ? nullptr : &*next(peak2));
  peak2->update(peak0, peakN);

  peakList.erase(peak1, peak2);
}


void PeakDetect::reduceSmallAreas(const float areaLimit)
{
  if (peakList.empty())
    THROW(ERR_NO_PEAKS, "Peak list is empty");

  const auto peakLast = prev(peakList.end());
  auto peak = next(peakList.begin());

  while (peak != peakList.end())
  {
    const float area = peak->getArea();
    if (area >= areaLimit)
    {
      peak++;
      continue;
    }

    auto peakCurrent = peak, peakMax = peak;
    const bool maxFlag = peak->getMaxFlag();
    float sumArea = 0.f, lastArea = 0.f;
    float valueMax = numeric_limits<float>::lowest();

    do
    {
      peak++;
      if (peak == peakList.end())
        break;

      sumArea = peak->getArea(* peakCurrent);
      lastArea = peak->getArea();
 // cout << "index " << peak->getIndex() + offset << ": " << sumArea <<
   // ", " << lastArea << endl;
      const float value = peak->getValue();
      if (! maxFlag && value > valueMax)
      {
        valueMax = value;
        peakMax = peak;
      }
      else if (maxFlag && -value > valueMax)
      {
        valueMax = -value;
        peakMax = peak;
      }
    }
    while (abs(sumArea) < areaLimit || abs(lastArea) < areaLimit);
// cout << "broke out: " << sumArea << ", " << lastArea << endl;

    if (abs(sumArea) < areaLimit || abs(lastArea) < areaLimit)
    {
// cout << "Reached the end\n";
      // It's the last set of peaks.  We could keep the largest peak
      // of the same polarity as peakCurrent (instead of peakCurrent).
      // It's a bit random whether or not this would be a "real" peak,
      // and we also don't keep track of this above.  So we we just stop.
      if (peakCurrent != peakList.end())
        peakList.erase(peakCurrent, peakList.end());
      break;
    }
    else if (peak->getMaxFlag() != maxFlag)
    {
// cout << "Different polarity\n";
      // Keep from peakCurrent to peak which is also often peakMax.
      PeakDetect::collapsePeaks(--peakCurrent, peak);
      peak++;
    }
    else
    {
// cout << "Same polarity\n";
      // Keep the start, the most extreme peak of opposite polarity,
      // and the end.
      PeakDetect::collapsePeaks(--peakCurrent, peakMax);
      PeakDetect::collapsePeaks(++peakMax, peak);
      peak++;
    }
  }
}


void PeakDetect::eliminateKinks()
{
  if (peakList.empty())
    THROW(ERR_NO_PEAKS, "Peak list is empty");

  for (auto peak = next(peakList.begin(), 2); 
      peak != prev(peakList.end()); peak++)
  {
    const auto peakPrev = prev(peak);
    const auto peakPrevPrev = prev(peakPrev);
    const auto peakNext = next(peak);

    const float ratioPrev = 
      peakPrev->getArea(* peakPrevPrev) / peak->getArea(* peakPrev);
    const float ratioNext = 
      peakNext->getArea(* peak) / peak->getArea(* peakPrev);

    if (ratioPrev > 1.f && 
        ratioNext > 1.f &&
        ratioPrev * ratioNext > 100.f)
    {
      PeakDetect::collapsePeaks(peakPrev, peakNext);
    }
  }
}


float PeakDetect::estimateScale(vector<float>& data) const
{
  // The algorithm is not particularly sensitive to the 
  // parameters here.  It's just to get an idea of scale.

  const unsigned ld = data.size();
  sort(data.begin(), data.end(), greater<float>());

  // If there are large values, they are often at the beginning
  // of a trace as a residual of the transient.

  unsigned first = 0;
  while (first < ld && 
      (data[first] > 2. * data[first+1] ||
       data[first] > 2. * data[first+2]))
    first++;

  if (first > 2)
  {
    cout << "WARNING estimateScale: More than two large peaks\n";
    for (unsigned i = 0; i < first; i++)
      cout << "i " << i << ": " << data[i] << endl;
    cout << endl;
  }
  
  // We take a number of the rest.

  const float firstVal = data[first];
  unsigned last = first;
  while (last < ld && data[last] > 0.5 * data[first])
    last++;

  return abs(data[(first + last) / 2]);
}


void PeakDetect::estimateScales()
{
  scalesList.reset();
  unsigned no = 0;
  for (auto peak = next(peakList.begin()); peak != peakList.end(); peak++)
  {
    if (! peak->getMaxFlag() && peak->getValue() < 0.f)
    {
      scalesList += * peak;
      no++;
    }
  }

  if (no == 0)
    THROW(ERR_NO_PEAKS, "No negative minima");

  scalesList /= no;
}


void PeakDetect::runKmeansOnce(
  const Koptions& koptions,
  vector<Peak>& clusters,
  float& distance)
{
  static random_device seed;
  static mt19937 rng(seed());
  uniform_int_distribution<unsigned> indices(0, peakList.size()-1);

  // Pick random centroids from the data points.
  clusters.resize(koptions.numClusters);
  const auto peakFirst = peakList.begin();

  unsigned c = 0;
  for (auto peak = next(peakList.begin()); peak != peakList.end(); peak++)
  {
    if (peak->isCandidate())
    {
      clusters[c] = * peak;
      c++;
      if (c == koptions.numClusters)
        break;
    }
  }

  // for (auto& cluster: clusters)
    // cluster = * next(peakFirst, indices(rng));

  distance = numeric_limits<float>::max();

  for (unsigned iter = 0; iter < koptions.numIterations; iter++)
  {
    vector<Peak> clustersNew(koptions.numClusters);
    vector<unsigned> counts(koptions.numClusters, 0);
    float distGlobal = 0.f;

    for (auto peak = next(peakList.begin());
        peak != peakList.end(); peak++)
    {
      if (! peak->isCandidate())
        continue;

      float distBest = numeric_limits<float>::max();
      unsigned bestCluster = 0;
      for (unsigned cno = 0; cno < koptions.numClusters; cno++)
      {
        const float dist = peak->distance(clusters[cno], scalesList);
        if (dist < distBest)
        {
          distBest = dist;
          bestCluster = cno;
        }
      }

      peak->logCluster(bestCluster);
      clustersNew[bestCluster] += * peak;
      counts[bestCluster]++;
      distGlobal += distBest;
    }

    for (unsigned cno = 0; cno < koptions.numClusters; cno++)
    {
      clusters[cno] = clustersNew[cno];
      clusters[cno] /= counts[cno];
    }

    if (distGlobal / distance >= koptions.convCriterion)
    {
      cout << "Finishing after " << iter << " iterations" << endl;
      cout << "Distance was " << distance << ", is " << distGlobal << endl;
      break;
    }
    else if (distGlobal <= distance)
      distance = distGlobal;

    cout << "Iteration " << iter << ": dist " << distGlobal << 
      ", distGlobal " << distGlobal << endl;
  }
}


void PeakDetect::reduce()
{
  PeakDetect::reduceSmallAreas(0.1f);
// cout << "Non-tiny list peaks: " << peakList.size() << "\n";
// PeakDetect::print();

  PeakDetect::eliminateKinks();

// cout << "Non-kinky list peaks: " << peakList.size() << "\n";
// PeakDetect::print();

  PeakDetect::estimateScales();

cout << "Scale list\n";
cout << scalesList.str(0) << "\n";

  const float scaledCutoffList = 0.05f * scalesList.getArea();

cout << "Area list cutoff " << scaledCutoffList << endl;

  PeakDetect::reduceSmallAreas(scaledCutoffList);

// cout << "Reasonable list peaks: " << peakList.size() <<  endl;
// PeakDetect::print();
// cout << endl;

  Koptions koptions;
  koptions.numClusters = 8;
  koptions.numIterations = 20;
  koptions.convCriterion = 0.999f;

  vector<Peak> clusters;
  float distance;
  PeakDetect::runKmeansOnce(koptions, clusters, distance);

  cout << "clusters" << endl;
  cout << clusters.front().strHeader();
  for (auto& c: clusters)
    cout << c.str(offset);
  cout << endl;

  for (unsigned i = 0; i < koptions.numClusters; i++)
  {
    cout << i << ":";
    for (auto& peak: peakList)
    {
      if (peak.isCluster(i))
        cout << " " << peak.getIndex() + offset;
    }
    cout << "\n";
  }
  cout << endl;

  /*
  cout << "distance from scale list:\n";
  for (unsigned i = 0; i < koptions.numClusters; i++)
  {
    cout << i << ": " << clusters[i].distance(scalesList, scalesList) <<
      "\n";
  }
  cout << endl;
  */

  for (auto peak = next(peakList.begin());
      peak != peakList.end(); peak++)
  {
    if (peak->isCandidate() && 
      peak->measure(scalesList) >= 3.f &&
      peak->getSymmetry() >= 0.3f &&
      peak->getSymmetry() <= 2.f)
    {
      peak->select();
    }
  }
}


void PeakDetect::makeSynthPeaks(vector<float>& synthPeaks) const
{
  for (unsigned i = 0; i < synthPeaks.size(); i++)
    synthPeaks[i] = 0;

  for (auto& peak: peakList)
  {
    if (peak.isSelected())
      synthPeaks[peak.getIndex()] = peak.getValue();
  }
}


void PeakDetect::getPeakTimes(vector<PeakTime>& times) const
{
  times.clear();

  unsigned findex = 0;
  bool flag = false;
  for (auto& peak: peakList)
  {
    if (peak.isSelected())
    {
      findex = peak.getIndex();
      flag = true;
      break;
    }
  }

  if (! flag)
    THROW(ERR_NO_PEAKS, "No output peaks");

  const float t0 = findex / static_cast<float>(SAMPLE_RATE);
  for (auto& peak: peakList)
  {
    if (peak.isSelected())
    {
      times.emplace_back(PeakTime());
      PeakTime& p = times.back();
      p.time = peak.getIndex() / SAMPLE_RATE - t0;
      p.value = peak.getValue();
    }
  }
}


void PeakDetect::print() const
{
  cout << peakList.front().strHeader();

  for (auto peak = next(peakList.begin()); peak != peakList.end(); peak++)
    cout << peak->str(offset);
  cout << "\n";
}

