#include <list>
#include <iostream>
#include <iomanip>
#include <functional>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "PeakDetect.h"
#include "PeakStats.h"
#include "Except.h"


#define SAMPLE_RATE 2000.

#define KINK_RATIO 100.f

#define TIME_PROXIMITY 0.03 // s

#define AREA_CUTOFF 0.05f
#define SCORE_CUTOFF 0.75

#define KMEANS_CLUSTERS 8
#define KMEANS_ITERATIONS 20
#define KMEANS_CONVERGENCE 0.999f

#define QUIET_FACTOR 0.8f

#define MEASURE_CUTOFF 1.0f

#define SHARP_CUTOFF 1.0f

#define MEDIAN_DEVIATIONS 3

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
  peaks.clear();
  quietCandidates.clear();
}


float PeakDetect::integrate(
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
  if (peaks.size() < 2)
    THROW(ERR_NO_PEAKS, "Too few peaks: " + to_string(peaks.size()));

  const auto peakFirst = peaks.begin();
  const auto peakLast = prev(peaks.end());

  for (auto it = peakFirst; it != peaks.end(); it++)
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

  if (peaks.size() < 2)
    THROW(ERR_NO_PEAKS, "Too few peaks: " + to_string(peaks.size()));

  const auto peakFirst = peaks.begin();
  const auto peakLast = prev(peaks.end());
  bool flag = true;

  for (auto it = peakFirst; it != peaks.end(); it++)
  {
    Peak peakSynth;
    Peak const * peakPrev = nullptr, * peakNext = nullptr;
    float areaFull = 0.f;
    const unsigned index = it->getIndex();

    if (it != peakFirst)
    {
      peakPrev = &*prev(it);
      areaFull = peakPrev->getAreaCum() + 
        PeakDetect::integrate(samples, peakPrev->getIndex(), index);
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


void PeakDetect::logFirst(const vector<float>& samples)
{
  peaks.emplace_back(Peak());

  // Find the initial peak polarity.
  bool maxFlag = false;
  for (unsigned i = 1; i < len; i++)
  {
    if (samples[i] > samples[0])
    {
      maxFlag = false;
      break;
    }
    else if (samples[i] < samples[0])
    {
      maxFlag = true;
      break;
    }
  }
  peaks.back().logSentinel(samples[0], maxFlag);
}


void PeakDetect::logLast(const vector<float>& samples)
{
  const auto& peakPrev = peaks.back();
  const float areaFull = 
    PeakDetect::integrate(samples, peakPrev.getIndex(), len-1);
  const float areaCumPrev = peakPrev.getAreaCum();

  peaks.emplace_back(Peak());
  peaks.back().log(
    samples.size()-1, 
    samples[samples.size()-1], 
    areaCumPrev + areaFull, 
    ! peakPrev.getMaxFlag());
}


void PeakDetect::log(
  const vector<float>& samples,
  const unsigned offsetSamples)
{
  len = samples.size();
  if (len < 2)
    THROW(ERR_SHORT_ACCEL_TRACE, "Accel trace length: " + to_string(len));

  offset = offsetSamples;
  peaks.clear();

  // The first peak is a dummy extremum at the first sample.
  PeakDetect::logFirst(samples);

  for (unsigned i = 1; i < len-1; i++)
  {
    bool maxFlag;
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

    const auto& peakPrev = peaks.back();
    const float areaFull = PeakDetect::integrate(samples, 
      peakPrev.getIndex(), i);
    const float areaCumPrev = peakPrev.getAreaCum();

    // The peak contains data for the interval preceding it.
    peaks.emplace_back(Peak());
    peaks.back().log(i, samples[i], areaCumPrev + areaFull, maxFlag);
  }

  // The last peak is a dummy extremum at the last sample.
  PeakDetect::logLast(samples);

  PeakDetect::annotate();

  PeakDetect::check(samples);
}


void PeakDetect::collapsePeaks(
  const list<Peak>::iterator peak1,
  const list<Peak>::iterator peak2)
{
  // Analogous to list.erase(), peak1 does not survive, while peak2 does.
  if (peak1 == peak2)
    return;

  // Need same polarity.
  if (peak1->getMaxFlag() != peak2->getMaxFlag())
    THROW(ERR_ALGO_PEAK_COLLAPSE, "Peaks with different polarity");

  Peak * peak0 = 
    (peak1 == peaks.begin() ? &*peak1 : &*prev(peak1));
  Peak * peakN = 
    (next(peak2) == peaks.end() ? nullptr : &*next(peak2));

  peak2->update(peak0, peakN);

  peaks.erase(peak1, peak2);
}


const list<Peak>::iterator PeakDetect::collapsePeaksNew(
  const list<Peak>::iterator peak1,
  const list<Peak>::iterator peak2)
{
  // Analogous to list.erase(), peak1 does not survive, while peak2 does.
  if (peak1 == peak2)
    return peak1;

  Peak * peak0 = 
    (peak1 == peaksNew.begin() ? nullptr : &*prev(peak1));
  Peak * peakN = 
    (next(peak2) == peaksNew.end() ? nullptr : &*next(peak2));

/*
if (peak1->getIndex()+offset == 0 &&
    peak2->getIndex()+offset == 23)
{
  cout << "About to fail? " << endl;
}
*/
  peak2->update(peak0, peakN);

if (peak1->getIndex()+offset == 0 &&
    peak2->getIndex()+offset == 23)
  cout << "Updated" << endl;

  return peaksNew.erase(peak1, peak2);

/*
if (peak1->getIndex()+offset == 0 &&
    peak2->getIndex()+offset == 23)
  cout << "Erased" << endl;
  */
}


void PeakDetect::reduceSmallRanges(const float rangeLimit)
{
  if (peaksNew.empty())
    THROW(ERR_NO_PEAKS, "Peak list is empty");

  const auto peakLast = prev(peaksNew.end());
  auto peak = next(peaksNew.begin());

  while (peak != peaksNew.end())
  {
    const float range = peak->getRange();
    if (range >= rangeLimit)
    {
      peak++;
      continue;
    }

    auto peakCurrent = peak, peakMax = peak;
    const bool maxFlag = peak->getMaxFlag();
    float sumRange = 0.f, lastRange = 0.f;
    float valueMax = numeric_limits<float>::lowest();

    do
    {
      peak++;
      if (peak == peaksNew.end())
        break;

      // sumRange = peak->getArea(* peakCurrent);
      sumRange = abs(peak->getValue() - peakCurrent->getValue());
      lastRange = peak->getRange();
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
    while (abs(sumRange) < rangeLimit || abs(lastRange) < rangeLimit);

    if (abs(sumRange) < rangeLimit || abs(lastRange) < rangeLimit)
    {
      // It's the last set of peaks.  We could keep the largest peak
      // of the same polarity as peakCurrent (instead of peakCurrent).
      // It's a bit random whether or not this would be a "real" peak,
      // and we also don't keep track of this above.  So we just stop.
      if (peakCurrent != peaksNew.end())
        peaksNew.erase(peakCurrent, peaksNew.end());
      break;
    }
    else if (peak->getMaxFlag() != maxFlag)
    {
      // Keep from peakCurrent to peak which is also often peakMax.
      PeakDetect::collapsePeaksNew(--peakCurrent, peak);
      peak++;
    }
    else
    {
      // Keep the start, the most extreme peak of opposite polarity,
      // and the end.
/*
cout << "peakCurrent " << peakCurrent->getIndex() + offset << endl;
cout << "peakMax " << peakMax->getIndex() + offset << endl;
cout << "peak " << peak->getIndex() + offset << endl;
cout << peakCurrent->str(offset);
cout << peakMax->str(offset);
cout << peak->str(offset);
*/

      PeakDetect::collapsePeaks(--peakCurrent, peakMax);
      PeakDetect::collapsePeaks(++peakMax, peak);
      peak++;
    }
  }
}


void PeakDetect::reduceSmallAreas(const float areaLimit)
{
  if (peaks.empty())
    THROW(ERR_NO_PEAKS, "Peak list is empty");

  const auto peakLast = prev(peaks.end());
  auto peak = next(peaks.begin());

  while (peak != peaks.end())
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
      if (peak == peaks.end())
        break;

      sumArea = peak->getArea(* peakCurrent);
      lastArea = peak->getArea();
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

    if (abs(sumArea) < areaLimit || abs(lastArea) < areaLimit)
    {
      // It's the last set of peaks.  We could keep the largest peak
      // of the same polarity as peakCurrent (instead of peakCurrent).
      // It's a bit random whether or not this would be a "real" peak,
      // and we also don't keep track of this above.  So we just stop.
      if (peakCurrent != peaks.end())
        peaks.erase(peakCurrent, peaks.end());
      break;
    }
    else if (peak->getMaxFlag() != maxFlag)
    {
      // Keep from peakCurrent to peak which is also often peakMax.
      PeakDetect::collapsePeaks(--peakCurrent, peak);
      peak++;
    }
    else
    {
      // Keep the start, the most extreme peak of opposite polarity,
      // and the end.
/*
cout << "peakCurrent " << peakCurrent->getIndex() + offset << endl;
cout << "peakMax " << peakMax->getIndex() + offset << endl;
cout << "peak " << peak->getIndex() + offset << endl;
cout << peakCurrent->str(offset);
cout << peakMax->str(offset);
cout << peak->str(offset);
*/

      PeakDetect::collapsePeaks(--peakCurrent, peakMax);
      PeakDetect::collapsePeaks(++peakMax, peak);
      peak++;
    }
  }
}


void PeakDetect::eliminateKinks()
{
  if (peaks.empty())
    THROW(ERR_NO_PEAKS, "Peak list is empty");

  for (auto peak = next(peaks.begin(), 2); 
      peak != prev(peaks.end()); peak++)
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
        ratioPrev * ratioNext > KINK_RATIO)
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
    THROW(ERR_ALGO_MANY_FRONT_PEAKS, "More than two large front peaks");
  
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
  for (auto peak = next(peaks.begin()); peak != peaks.end(); peak++)
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


void PeakDetect::pickStartingClusters(
  vector<PeakCluster>& clusters,
  const unsigned n) const
{
  clusters.resize(n);

  // TODO Pick these more intelligently.

  unsigned c = 0;
  for (auto& peak: peaks)
  {
    if (peak.isCandidate())
    {
      clusters[c].centroid = peak;
      c++;
      if (c == n)
        break;
    }
  }
}


void PeakDetect::pickStartingClustersSharp(
  const list<Sharp>& sharps,
  vector<SharpCluster>& clusters,
  const unsigned n) const
{
  clusters.resize(n);
  const unsigned lp = peaks.size();
  if (n >= lp)
    THROW(ERR_NO_PEAKS, "Not enough peaks for clusters");

  const unsigned start = (lp - n) / 2;
  auto sharp = sharps.begin();
  for (unsigned i = 0; i < start; i++)
    sharp++;

  for (unsigned i = 0; i < n; i++)
  {
    clusters[i].centroid = * sharp;
    sharp++;
  }
}


void PeakDetect::pickStartingClustersQuiet(
  list<Period>& quiets,
  vector<PeriodCluster>& clusters,
  const unsigned n) const
{
  clusters.resize(n);

  // TODO Pick these more intelligently.

  unsigned c = 0;
  for (auto& quiet: quiets)
  {
    clusters[c].centroid = quiet;
    c++;
    if (c == n)
      break;
  }
}



void PeakDetect::runKmeansOnce(vector<PeakCluster>& clusters)
{
  // Pick centroids from the data points.
  PeakDetect::pickStartingClusters(clusters, KMEANS_CLUSTERS);

  float distance = numeric_limits<float>::max();

  for (unsigned iter = 0; iter < KMEANS_ITERATIONS; iter++)
  {
    vector<PeakCluster> clustersNew(clusters.size());
    vector<unsigned> counts(clusters.size(), 0);
    float distGlobal = 0.f;

    for (auto& peak: peaks)
    {
      if (! peak.isCandidate())
        continue;

      float distBest = numeric_limits<float>::max();
      unsigned bestCluster = 0;
      for (unsigned cno = 0; cno < clusters.size(); cno++)
      {
        const float dist = 
          peak.distance(clusters[cno].centroid, scalesList);
        if (dist < distBest)
        {
          distBest = dist;
          bestCluster = cno;
        }
      }

      peak.logCluster(bestCluster);
      clustersNew[bestCluster].centroid += peak;
      counts[bestCluster]++;
      distGlobal += distBest;
    }

    for (unsigned cno = 0; cno < clusters.size(); cno++)
    {
      clusters[cno].centroid = clustersNew[cno].centroid;
      clusters[cno].centroid /= counts[cno];
    }

    if (distGlobal / distance >= KMEANS_CONVERGENCE)
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


void PeakDetect::runKmeansOnceSharp(
  list<Sharp>& sharps,
  const Sharp& sharpScale,
  vector<SharpCluster>& clusters)
{
  // Pick centroids from the data points.
  PeakDetect::pickStartingClustersSharp(sharps, clusters, KMEANS_CLUSTERS);

  float distance = numeric_limits<float>::max();

  for (unsigned iter = 0; iter < KMEANS_ITERATIONS; iter++)
  {
    vector<SharpCluster> clustersNew(clusters.size());
    vector<unsigned> counts(clusters.size(), 0);
    float distGlobal = 0.f;

    for (auto& sharp: sharps)
    {
      float distBest = numeric_limits<float>::max();
      unsigned bestCluster = 0;
      for (unsigned cno = 0; cno < clusters.size(); cno++)
      {
        const float dist = 
          sharp.distance(clusters[cno].centroid, sharpScale);
        if (dist < distBest)
        {
          distBest = dist;
          bestCluster = cno;
        }
      }

      sharp.clusterNo = bestCluster;
      clustersNew[bestCluster].centroid += sharp;
      counts[bestCluster]++;
      distGlobal += distBest;
    }

    for (unsigned cno = 0; cno < clusters.size(); cno++)
    {
      clusters[cno].centroid = clustersNew[cno].centroid;
      clusters[cno].centroid /= counts[cno];
    }

    if (distGlobal / distance >= KMEANS_CONVERGENCE)
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


void PeakDetect::runKmeansOnceQuiet(
  list<Period>& quiets,
  vector<PeriodCluster>& clusters)
{
  // Pick centroids from the data points.
  PeakDetect::pickStartingClustersQuiet(quiets, clusters, KMEANS_CLUSTERS);

  float distance = numeric_limits<float>::max();

  for (unsigned iter = 0; iter < KMEANS_ITERATIONS; iter++)
  {
    vector<PeriodCluster> clustersNew(clusters.size());
    vector<unsigned> counts(clusters.size(), 0);
    float distGlobal = 0.f;

    for (auto& quiet: quiets)
    {
      float distBest = numeric_limits<float>::max();
      unsigned bestCluster = 0;
      for (unsigned cno = 0; cno < clusters.size(); cno++)
      {
        const float dist = 
          quiet.distance(clusters[cno].centroid);
        if (dist < distBest)
        {
          distBest = dist;
          bestCluster = cno;
        }
      }

      quiet.clusterNo = bestCluster;
      clustersNew[bestCluster].centroid += quiet;
      counts[bestCluster]++;
      distGlobal += distBest;
    }

    for (unsigned cno = 0; cno < clusters.size(); cno++)
    {
      clusters[cno].centroid = clustersNew[cno].centroid;
      clusters[cno].centroid /= counts[cno];
    }

    if (distGlobal / distance >= KMEANS_CONVERGENCE)
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


void PeakDetect::pos2time(
  const vector<PeakPos>& posTrue, 
  const double speed,
  vector<PeakTime>& timesTrue) const
{
  for (unsigned i = 0; i < posTrue.size(); i++)
  {
    timesTrue[i].time = posTrue[i].pos / speed;
    timesTrue[i].value = posTrue[i].value;
  }
}


bool PeakDetect::advance(list<Peak>::iterator& peak) const
{
  while (peak != peaks.end() && ! peak->isSelected())
    peak++;
  return (peak != peaks.end());
}


double PeakDetect::simpleScore(
  const vector<PeakTime>& timesTrue,
  const double offsetScore,
  const bool logFlag,
  double& shift)
{
  // This is similar to Align::simpleScore.
  
  list<Peak>::iterator peak = peaks.begin();
  if (! PeakDetect::advance(peak))
    THROW(ERR_NO_PEAKS, "No peaks present or selected");

  const auto peakFirst = peak;
  list<Peak>::iterator peakBest, peakPrev;
  double score = 0.;
  unsigned scoring = 0;

  for (unsigned tno = 0; tno < timesTrue.size(); tno++)
  {
    double d = TIME_PROXIMITY, dabs = TIME_PROXIMITY;
    const double timeTrue = timesTrue[tno].time;
    double timeSeen = peak->getTime() - offsetScore;

    d = timeSeen - timeTrue;
    if (d >= 0.)
    {
      dabs = d;
      peakBest = peak;
    }
    else
    {
      double dleft = numeric_limits<double>::max();
      double dright = numeric_limits<double>::max();
      while (PeakDetect::advance(peak))
      {
        timeSeen = peak->getTime() - offsetScore;
        dright = timeSeen - timeTrue;
        if (dright >= 0.)
          break;
        else
        {
          peakPrev = peak;
          peak++;
        }
      }

      if (dright < 0.)
      {
        // We reached the end.
        dabs = -dright;
        d = dright;
        peakBest = peakPrev;
      }
      else 
      {
        dleft = timeTrue - (peakPrev->getTime() - offsetScore);
        if (dleft <= dright)
        {
          dabs = dleft;
          d = -dleft;
          peakBest = peakPrev;
        }
        else
        {
          dabs = dright;
          d = dright;
          peakBest = peak;
        }
      }
    }

    if (dabs <= TIME_PROXIMITY)
    {
      score += (TIME_PROXIMITY - dabs) / TIME_PROXIMITY;
      shift += d;
      scoring++;
      if (logFlag)
        peakBest->logMatch(tno);
    }
  }

  if (scoring)
    shift /= scoring;

  return score;
}


void PeakDetect::setOffsets(
  const vector<PeakTime>& timesTrue,
  vector<double>& offsetList) const
{
  const unsigned lp = peaks.size();
  const unsigned lt = timesTrue.size();

  // Probably too many.
  const unsigned maxShiftSeen = 3;
  const unsigned maxShiftTrue = 3;
  if (lp <= maxShiftSeen || lt <= maxShiftTrue)
    THROW(ERR_NO_PEAKS, "Not enough peaks");

  offsetList.resize(maxShiftSeen + maxShiftTrue + 1);

  // Offsets are subtracted from the seen peaks.
  list<Peak>::const_iterator peak = peaks.end();
  for (unsigned i = 0; i <= maxShiftSeen; i++)
  {
    do
    {
      peak = prev(peak);
    }
    while (peak != peaks.begin() && ! peak->isSelected());

    offsetList[i] = peak->getTime() - timesTrue[lt-1].time;
  }

  peak = peaks.end();
  do
  {
    peak = prev(peak);
  }
  while (peak != peaks.begin() && ! peak->isSelected());
  const double lastTime = peak->getTime();

  for (unsigned i = 1; i <= maxShiftTrue; i++)
    offsetList[maxShiftSeen+i] = timesTrue[lt-1-i].time - lastTime;
}


bool PeakDetect::findMatch(
  const vector<PeakTime>& timesTrue,
  double& shift)
{
  vector<double> offsetList;
  PeakDetect::setOffsets(timesTrue, offsetList);

  double score = 0.;
  unsigned ino = 0;
  shift = 0.;
  for (unsigned i = 0; i < offsetList.size(); i++)
  {
    double shiftNew = 0.;
    double scoreNew = 
      PeakDetect::simpleScore(timesTrue, offsetList[i], false, shiftNew);
cout << "i " << i << ": " << scoreNew << endl;
    if (scoreNew > score)
    {
cout << "*\n";
      score = scoreNew;
      shift = shiftNew;
      ino = i;
    }
  }

  // This extra run could be eliminated at the cost of some copying
  // and intermediate storage.
  double tmp;
  shift += offsetList[ino];
  score = PeakDetect::simpleScore(timesTrue, shift, true, tmp);
cout << "Final score " << score << " vs. " <<
  timesTrue.size() << " real and " << 
  numTentatives << " tentatively detected peaks" << endl << endl;

  if (score >= SCORE_CUTOFF * timesTrue.size())
    return true;
  else if (numTentatives >= 0.5 * timesTrue.size() &&
      score >= SCORE_CUTOFF * numTentatives)
    return true;
  else
    return false;
}
  

PeakType PeakDetect::findCandidate(
  const double time,
  const double shift) const
{
  double dist = numeric_limits<double>::max();
  PeakType type = PEAK_TRUE_MISSING;

  for (auto& peak: peaks)
  {
    if (! peak.isCandidate() || peak.isSelected())
      continue;
    
    const double pt = peak.getTime() - shift;
    const double distNew = time - pt;
    if (abs(distNew) < dist)
    {
      dist = abs(distNew);
      if (dist < TIME_PROXIMITY)
        type = peak.getType();
    }

    if (distNew < 0.)
      break;
  }
  return type;
}


PeakType PeakDetect::classifyPeak(
  const Peak& peak,
  const vector<PeakCluster>& clusters) const
{
  const unsigned c = peak.getCluster();
  const float m = peak.measure();

  if (clusters[c].good)
  {
    if (m <= MEASURE_CUTOFF)
      return PEAK_TENTATIVE;
    else if (m <= 2.f)
      return PEAK_POSSIBLE;
    else if (m <= 6.f)
      return PEAK_CONCEIVABLE;
    else
      return PEAK_REJECTED;
  }
  else
  {
    if (m <= 2.f)
      return PEAK_POSSIBLE;
    else if (m <= 6.f)
      return PEAK_CONCEIVABLE;
    else
      return PEAK_REJECTED;
  }
}


void PeakDetect::countClusters(vector<PeakCluster>& clusters)
{
  for (auto& peak: peaks)
  {
    if (! peak.isCandidate())
      continue;
    
    const unsigned c = peak.getCluster();
    const float m = peak.measure(scalesList);
    clusters[c].len++;
    if (m <= MEASURE_CUTOFF)
      clusters[c].numConvincing++;
  }
}


unsigned PeakDetect::getConvincingClusters(vector<PeakCluster>& clusters)
{
  unsigned num = 0;
  for (unsigned i = 0; i < clusters.size(); i++)
  {
    // TODO Algorithmic define's.
    if (clusters[i].len >= 3 && 
        clusters[i].numConvincing >= 0.8 * clusters[i].len)
    {
      clusters[i].good = true;
      num++;
    }
    else
      clusters[i].good = false;
  }
  return num;
}


void PeakDetect::setOrigPointers()
{
  for (auto peak = peaks.begin(), peakNew = peaksNew.begin(); 
      peak != peaks.end(); peak++, peakNew++)
    peakNew->logOrigPointer(&*peak);
}


void PeakDetect::eliminatePositiveMinima()
{
  auto peak = peaksNew.begin();
  while (peak != peaksNew.end())
  {
    if (peak->getValue() >= 0. && peak->getMaxFlag() == false)
      peak = PeakDetect::collapsePeaksNew(peak, next(peak));
    else
      peak++;
  }
}


void PeakDetect::eliminateNegativeMaxima()
{
  auto peak = peaksNew.begin();
  while (peak != peaksNew.end())
  {
    if (peak->getValue() < 0. && peak->getMaxFlag() == true)
      peak = PeakDetect::collapsePeaksNew(peak, next(peak));
    else
      peak++;
  }
}


void PeakDetect::reducePositiveMaxima()
{
  auto peak = peaksNew.begin();
  while (peak != peaksNew.end())
  {
    const float vmid = peak->getValue();
    if (vmid < 0. || peak->getMaxFlag() == false)
    {
      peak++;
      continue;
    }

    if (peak == peaksNew.begin())
    {
      peak++;
      continue;
    }
    const auto peakPrev = prev(peak);
    const float v0 = peakPrev->getValue();

    const auto peakNext = next(peak);
    if (peakNext == peaksNew.end())
      break;
    const float v1 = peakNext->getValue();

    // Want the middle one and at least one other to be positive.
    // Want the the three to be monotonic.
    if (v0 < 0. && v1 < 0.)
    {
      peak++;
      continue;
    }
    if (vmid > v0 && vmid > v1)
    {
      peak++;
      continue;
    }
    else if (vmid < v0 && vmid < v1)
    {
      peak++;
      continue;
    }

    // So now we have three consecutive positive maxima.
    // If they are reasonably aligned, cut out the middle one.

    const float grad1 = peak->getGradient();
    const float grad2 = peakNext->getGradient();
    const float ratio = grad2 / grad1;

    if (ratio >= 0.5f && ratio <= 2.0f)
      peak = PeakDetect::collapsePeaksNew(peak, peakNext);
    else
      peak++;
  }
}


void PeakDetect::reduceNegativeMinima()
{
  auto peak = peaksNew.begin();
  while (peak != peaksNew.end())
  {
    const float vmid = peak->getValue();
    if (vmid >= 0. || peak->getMaxFlag() == true)
    {
      peak++;
      continue;
    }

    if (peak == peaksNew.begin())
    {
      peak++;
      continue;
    }

    const auto peakPrev = prev(peak);
    const float v0 = peakPrev->getValue();

    const auto peakNext = next(peak);
    if (peakNext == peaksNew.end())
      break;
    const float v1 = peakNext->getValue();

    // Want the middle one and at least one other to be negative.
    // Want the the three to be monotonic.
    if (v0 >= 0. && v1 >= 0.)
    {
      peak++;
      continue;
    }
    if (vmid > v0 && vmid > v1)
    {
      peak++;
      continue;
    }
    else if (vmid < v0 && vmid < v1)
    {
      peak++;
      continue;
    }

    // So now we have three consecutive negative minima.
    // If they are reasonably aligned, cut out the middle one.

    const float grad1 = peak->getGradient();
    const float grad2 = peakNext->getGradient();
    const float ratio = grad2 / grad1;

    if (ratio >= 0.5f && ratio <= 2.0f)
      peak = PeakDetect::collapsePeaksNew(peak, peakNext);
    else
      peak++;
  }
}


void PeakDetect::markSharpPeaksPos(
  list<Sharp>& sharps,
  Sharp sharpScale,
  vector<SharpCluster>& clusters,
  const bool debug)
{
  if (debug)
  {
    cout << "Positive sharp peaks\n";
    cout << sharpScale.strHeader();
  }

  unsigned count = 0;
  for (auto peak = next(peaksNew.begin()); 
      peak != prev(peaksNew.end()); peak++)
  {
    auto peakPrev = prev(peak);
    auto peakNext = next(peak);

    if (peak->getMaxFlag())
    {
      // Both flanks must go negative for the current purpose.
      if (peakPrev->getMaxFlag() || peakNext->getMaxFlag())
        continue;

      sharps.emplace_back(Sharp());
      Sharp& s = sharps.back();

      count++;
      s.index = peak->getIndex();
      s.level = peak->getValue();
      s.range1 = peak->getRange();
      s.rangeRatio = peakNext->getRange() / s.range1;
      s.grad1 = peak->getGradient();
      s.gradRatio = peakNext->getGradient() / s.grad1;
      s.fill1 = peak->getFill();
      s.fillRatio = peakNext->getFill() / s.fill1;

      if (debug)
        cout << s.str(offset);

      sharpScale += s;
    }
  }
  cout << endl;

  if (count == 0)
    return;

  sharpScale /= count;

  runKmeansOnceSharp(sharps, sharpScale, clusters);

  PeakDetect::printSharpClusters(sharps, sharpScale,
    clusters, true);
}


void PeakDetect::markSharpPeaksNeg(
  list<Sharp>& sharps,
  Sharp sharpScale,
  vector<SharpCluster>& clusters,
  const bool debug)
{

  if (debug)
  {
    cout << "Negative sharp peaks\n";
    cout << sharpScale.strHeader();
  }

  unsigned count = 0;
  for (auto peak = next(peaksNew.begin()); 
      peak != prev(peaksNew.end()); peak++)
  {
    auto peakPrev = prev(peak);
    auto peakNext = next(peak);

    if (! peak->getMaxFlag())
    {
      // Both flanks must go positive for the current purpose.
      if (! peakPrev->getMaxFlag() || ! peakNext->getMaxFlag())
        continue;

      sharps.emplace_back(Sharp());
      Sharp& s = sharps.back();

      count++;
      s.index = peak->getIndex();
      s.level = peak->getValue();
      s.range1 = peak->getRange();
      s.rangeRatio = peakNext->getRange() / s.range1;
      s.grad1 = peak->getGradient();
      s.gradRatio = peakNext->getGradient() / s.grad1;
      s.fill1 = peak->getFill();
      s.fillRatio = peakNext->getFill() / s.fill1;

      if (debug)
        cout << s.str(offset);

      sharpScale += s;
    }
  }
  cout << endl;

  if (count == 0)
    return;

  sharpScale /= count;

  runKmeansOnceSharp(sharps, sharpScale, clusters);

  PeakDetect::printSharpClusters(sharps, sharpScale,
    clusters, true);
}


void PeakDetect::markSharpPeaks()
{
  if (peaksNew.size() < 2)
    THROW(ERR_NO_PEAKS, "No peaks");

  const bool debug = true;

  list<Sharp> sharpsPos, sharpsNeg;
  Sharp sharpScalePos, sharpScaleNeg;
  vector<SharpCluster> clustersPos, clustersNeg;

  PeakDetect::markSharpPeaksPos(sharpsPos, sharpScalePos,
    clustersPos, debug);
  PeakDetect::markSharpPeaksNeg(sharpsNeg, sharpScaleNeg,
    clustersNeg, debug);
}


void PeakDetect::markZeroCrossings()
{
  unsigned nextZCindex = numeric_limits<unsigned>::max();
  float currentZCmax = 0.;
  for (auto peak = peaksNew.rbegin(); peak != peaksNew.rend(); peak++)
  {
    const float v = peak->getValue();
    if (abs(v) > currentZCmax)
      currentZCmax = abs(v);

    // So this is earlier in time.
    auto peakNext = next(peak);
    if (peakNext == peaksNew.rend())
      continue;
    const float vnext = peakNext->getValue();
    if ((v >= 0.f && vnext < 0.f) || (v < 0.f && vnext >= 0.f))
    {
      if (nextZCindex != numeric_limits<unsigned>::max())
        peak->logZeroCrossing(nextZCindex, currentZCmax);
      nextZCindex = peak->getIndex();
      currentZCmax = abs(vnext);
    }
  }
}


list<Peak>::iterator PeakDetect::advanceZC(
  const list<Peak>::iterator peak,
  const unsigned indexNext) const
{
  list<Peak>::iterator peakNext = peak;
  while (peakNext != peaksNew.end() && peakNext->getIndex() != indexNext)
    peakNext++;
  return peakNext;
}


void PeakDetect::markQuiet()
{
  list<Peak>::iterator peak;
  for (peak = peaksNew.begin(); peak != peaksNew.end(); peak++)
  {
    if (peak->getValueZC() != 0.f && peak->getValue() >= 0.f)
      break;
  }

  if (peak == peaksNew.end())
    THROW(ERR_NO_PEAKS, "No positive zero-crossing");

  while (true)
  {
    // Go QUIET_FACTOR of the way from the zero-crossing to the level.
    float floor = abs((1.f - QUIET_FACTOR) * peak->getValueZC() + 
      QUIET_FACTOR * (peak->getValue() - peak->getRange()));
cout << "floor: from " << peak->getValueZC() << " to " <<
  (peak->getValue() - peak->getRange()) << " = " << floor << endl;

    bool foundQuiet = false;
    auto peakNew = peak;
    while (true)
    {
      peakNew = PeakDetect::advanceZC(peakNew, peakNew->getIndexNextZC());
      if (peakNew == peaksNew.end())
        break;

      // This is a zero-crossing that heads negative.
      float valueNew = peakNew->getValueZC();
cout << "Loop: " << peak->getIndex()+offset << " to " <<
  peakNew->getIndex()+offset << ", vnew " << valueNew << endl;
      if (abs(valueNew) <= floor)
      {
        foundQuiet = true;
        // Get the next, positive-heading zero-crossing.
        peakNew = PeakDetect::advanceZC(peakNew, peakNew->getIndexNextZC());
        if (peakNew == peaksNew.end())
          break;
      }
      else
      {
cout << "value " << valueNew << " not in range\n";
        break;
      }
    }

    if (! foundQuiet)
    {
      peak = PeakDetect::advanceZC(peakNew, peakNew->getIndexNextZC());
      if (peak == peaksNew.end())
        break;
      continue;
    }

    if (peakNew == peaksNew.end())
    {
      peak->logQuietBegin();
      peakNew = prev(peakNew);
cout << "Marking " << peak->getIndex()+offset << " to final " << 
peakNew->getIndex()+offset << endl;
      peakNew->logQuietEnd();
      break;
    }
    else
    {
      peak->logQuietBegin();
cout << "Marking " << peak->getIndex()+offset << " to final ";
      peak = PeakDetect::advanceZC(peakNew, peakNew->getIndexNextZC());
      peakNew = prev(peakNew);
      peakNew->logQuietEnd();
cout << peakNew->getIndex()+offset << endl;
      if (peak == peaksNew.end())
        break;
    }
  }
}


void PeakDetect::countPositiveRuns() const
{
  unsigned run = 0;
  for (auto peak = peaksNew.begin(); peak != peaksNew.end(); peak++)
  {
    const float vmid = peak->getValue();
    if (vmid < 0. || peak->getMaxFlag() == false)
    {
      if (run >= 3)
        cout << "RUN ended " << peak->getIndex() + offset <<
          ", run " << run << endl;
      run = 0;
      continue;
    }

    run++;
  }

  if (run >= 3)
    cout << "RUN last " << run << endl;
}


unsigned PeakDetect::countDuplicateUses(
  const list<Period>& quiets,
  const unsigned cno) const
{
  unsigned top = 0;
  for (auto& qc: quiets)
  {
    if (qc.clusterNo == cno && qc.start + qc.len >= top)
      top = qc.start + qc.len;
  }

  vector<unsigned> hash(top+1);
  for (auto& qc: quiets)
  {
    if (qc.clusterNo == cno)
    {
      hash[qc.start]++;
      hash[qc.start + qc.len]++;
    }
  }

  unsigned count = 0;
  for (auto h: hash)
  {
    if (h > 0)
      count += h-1;
  }
  return count;
}


void PeakDetect::estimatePeriodicity(
  vector<unsigned>& spacings,
  unsigned& period,
  unsigned& numMatches) const
{
  const unsigned ls = spacings.size();
  if (ls == 0)
  {
    period = 0;
    numMatches = 0;
    return;
  }

  sort(spacings.begin(), spacings.end());

  vector<unsigned> hist(spacings[ls-1]+1);
  for (auto l: spacings)
    hist[l]++;

  vector<unsigned> matches(ls);
  unsigned bestIndex = 0;
  unsigned bestCount = 0;

  for (unsigned i = 0; i < ls; i++)
  {
    // Look for smallish multiples of the spacing.
    const unsigned sp = spacings[i];
    for (unsigned mult = 1; mult <= 4; mult++)
    {
      const unsigned spmult = sp * mult;
      unsigned lower = static_cast<unsigned>(0.9 * spmult);
      unsigned upper = static_cast<unsigned>(1.1 * spmult);
      if (upper >= spacings[ls-1])
        upper = spacings[ls-1];

      for (unsigned j = lower; j <= upper; j++)
        matches[i] += hist[j];
    }

    if (matches[i] > bestCount)
    {
      bestIndex = i;
      bestCount = matches[i];
    }
  }

  // If there is a streak of best counts, pick the middle one.
  unsigned i = bestIndex;
  while (i < ls && matches[i] == bestCount)
    i++;

  period = spacings[(i+bestIndex)/2];
  numMatches = bestCount;
}


bool PeakDetect::clustersCompatible(
  const vector<Period *>& intervals1,
  const vector<Period *>& intervals2) const
{
  vector<Period *> intervalsCombined = intervals1;
  intervalsCombined.insert(intervalsCombined.end(), intervals2.begin(),
    intervals2.end());

  sort(intervalsCombined.begin(), intervalsCombined.end(),
    [](const Period * p1, const Period * p2) -> bool
    {
      return p1->start < p2->start;
    });

  for (unsigned i = 0; i+1 < intervalsCombined.size(); i++)
  {
    if (intervalsCombined[i]->start + intervalsCombined[i]->len >=
        intervalsCombined[i+1]->start)
      return false;
  }

  return true;
}


void PeakDetect::findCompatibles(
  const vector<vector<Period *>>& intervals,
  vector<vector<unsigned>>& compatibles) const
{
  const unsigned lc = intervals.size();
  for (unsigned cno = 0; cno < lc; cno++)
  {
    for (unsigned dno = cno+1; dno < lc; dno++)
    {
      if (PeakDetect::clustersCompatible(intervals[cno], intervals[dno]))
        compatibles[cno].push_back(dno);
    }
  }
}


void PeakDetect::setQuietMedians(
  list<Period>& quiets,
  vector<PeriodCluster>& clusters,
  unsigned& period)
{
  const unsigned lc = clusters.size();

  vector<vector<unsigned>> lengths;
  lengths.resize(lc);
  vector<vector<unsigned>> spacings(lc);
  vector<Period *> qprev(lc, nullptr);
  vector<vector<Period *>> intervals;
  intervals.resize(lc);

  for (unsigned cno = 0; cno < lc; cno++)
  {
    clusters[cno].lenCloseCount = 0;
    clusters[cno].count = 0;
  }

  for (auto qc = quiets.begin(); qc != quiets.end(); qc++)
  {
    const unsigned cno = qc->clusterNo;
    clusters[cno].count++;
    lengths[cno].push_back(qc->len);
    intervals[cno].push_back(&*qc);

    if (qprev[cno] != nullptr)
    {
      if (qc->start > qprev[cno]->start)
        spacings[cno].push_back(qc->start - qprev[cno]->start);
    }

    qprev[cno] = &*qc;
  }

  // Compatible clusters have no overlaps.
  vector<vector<unsigned>> compatibles;
  compatibles.resize(lc);

  PeakDetect::findCompatibles(intervals, compatibles);

  for (unsigned cno = 0; cno < lc; cno++)
  {
    cout << cno << ":";
    for (auto i: compatibles[cno])
      cout << " " << i;
    cout << "\n";
  }
  cout << endl;


  // Count instances where an end point occurs twice.  This is a bad
  // sign for the quality of a cluster.
  // TODO Use intervals for speed.
  for (unsigned cno = 0; cno < clusters.size(); cno++)
    clusters[cno].duplicates = 
      PeakDetect::countDuplicateUses(quiets, cno);

  // Each cluster on its own.
  vector<unsigned> periods(lc), matches(lc);
  for (unsigned cno = 0; cno < clusters.size(); cno++)
    PeakDetect::estimatePeriodicity(spacings[cno], 
      clusters[cno].periodDominant, clusters[cno].numPeriodics);

  // Pool the results of each cluster.
  vector<unsigned> spacingsAll;
  for (unsigned cno = 0; cno < clusters.size(); cno++)
  {
    spacingsAll.insert(spacingsAll.end(),
      spacings[cno].begin(), spacings[cno].end());
  }

  unsigned numPer;
  PeakDetect::estimatePeriodicity(spacingsAll, period, numPer);
  cout << "Synthetic period: " << period << endl;

  for (unsigned cno = 0; cno < clusters.size(); cno++)
  {
    unsigned n = lengths[cno].size() / 2;
    nth_element(lengths[cno].begin(), lengths[cno].begin()+n, 
      lengths[cno].end());
    clusters[cno].lenMedian = lengths[cno][n];

    // TODO: #define.  Just a rough number.
    for (unsigned l: lengths[cno])
    {
      if (l > static_cast<unsigned>(0.9 * clusters[cno].lenMedian) &&
          l < static_cast<unsigned>(1.1 * clusters[cno].lenMedian))
      {
        clusters[cno].lenCloseCount++;
      }
    }
  }
}


void PeakDetect::markPossibleQuiet()
{
  vector<unsigned> lengths;
  quietCandidates.clear();

  for (auto peak = peaksNew.begin(); peak != peaksNew.end(); peak++)
  {
    // Get a negative minimum.
    const float value = peak->getValue();
    if (value >= 0. || peak->getMaxFlag())
      continue;

    // Find the previous negative minimum that was deeper.
    auto peakPrev = peak;
    bool posFlag = false;
    bool maxPrevFlag;
    float vPrev = 0.;
    float lowerMinValue = 0.f;
    do
    {
      if (peakPrev == peaksNew.begin())
        break;

      peakPrev = prev(peakPrev);
      maxPrevFlag = peakPrev->getMaxFlag();
      vPrev = peakPrev->getValue();

      if (! maxPrevFlag && vPrev > value)
        lowerMinValue = min(vPrev, lowerMinValue);

      if (vPrev >= 0.f)
        posFlag = true;
    }
    while (vPrev >= value || maxPrevFlag);

    if (posFlag && lowerMinValue < 0.f &&
        peakPrev != peaksNew.begin() && vPrev < value)
    {
      quietCandidates.emplace_back(Period());
      Period& p = quietCandidates.back();
      p.start = peakPrev->getIndex();
      p.len = peak->getIndex() - p.start;
      p.depth = (lowerMinValue - vPrev) / scalesList.getRange();
      p.minLevel = lowerMinValue;
      lengths.push_back(p.len);
    }

    // Same thing forwards.
    auto peakNext = peak;
    posFlag = false;
    bool maxNextFlag;
    float vNext = 0.;
    lowerMinValue = 0.f;
    do
    {
      if (peakNext == peaksNew.end())
        break;

      peakNext = next(peakNext);
      maxNextFlag = peakNext->getMaxFlag();
      vNext = peakNext->getValue();

      if (! maxNextFlag && vNext > value)
        lowerMinValue = min(vNext, lowerMinValue);

      if (vNext >= 0.f)
        posFlag = true;
    }
    while (vNext >= value || maxNextFlag);

    if (posFlag && lowerMinValue < 0.f &&
        peakNext != peaksNew.end() && vNext < value)
    {
      quietCandidates.emplace_back(Period());
      Period& p = quietCandidates.back();
      p.start = peak->getIndex();
      p.len = peakNext->getIndex() - p.start;
      p.depth = (lowerMinValue - vNext) / scalesList.getRange();
      p.minLevel = lowerMinValue;
      lengths.push_back(p.len);
    }
  }

  const unsigned n = lengths.size() / 2;
  nth_element(lengths.begin(), lengths.begin()+n, lengths.end());
  const float median = static_cast<float>(lengths[n]);

  /*
  for (auto& qc: quietCandidates)
  {
    qc.lenScaled = qc.len / median;
    cout << qc.start + offset << 
      " len " << qc.len << " to " << qc.start + offset + qc.len <<
      " depth " << qc.depth << endl;
  }
  */

  vector<PeriodCluster> clusters;
  PeakDetect::runKmeansOnceQuiet(quietCandidates, clusters);
  PeakDetect::printQuietClusters(quietCandidates, clusters, false);

  float maxDepth = 0.f;
  unsigned maxIndex = 0;

  for (unsigned i = 0; i < clusters.size(); i++)
  {
    if (clusters[i].centroid.depth > maxDepth)
    {
      maxIndex = i;
      maxDepth = clusters[i].centroid.depth;
    }
  }

  if (maxDepth == 0.f)
    THROW(ERR_NO_PEAKS, "No quiet candidates");

  // TODO Could check for periodicity and no gaps between quiet periods

  // Should maybe be a class variable.
  unsigned period;
  PeakDetect::setQuietMedians(quietCandidates, clusters, period);

  cout << "Cluster select scores: period " << period << "\n";
  cout << setw(2) << "No" << 
    setw(6) << "Len" <<
    setw(6) << "Count" <<
    setw(6) << "Lno" <<
    setw(6) << "Per" <<
    setw(6) << "#per" <<
    setw(6) << "Dupes" << endl;

  for (unsigned i = 0; i < clusters.size(); i++)
  {
    const PeriodCluster& c = clusters[i];

    cout << setw(2) << right << i <<
      setw(6) << c.lenMedian <<
      setw(6) << c.count <<
      setw(6) << c.lenCloseCount <<
      setw(6) << c.periodDominant <<
      setw(6) << c.numPeriodics <<
      setw(6) << c.duplicates << endl;
  }

  quietFavorite = maxIndex;
}


void PeakDetect::reduceNew()
{
  peaksNew = peaks;
  PeakDetect::setOrigPointers();

PeakDetect::reduceSmallRanges(scalesList.getRange() / 10.f);

PeakDetect::markPossibleQuiet();

  /*
  PeakDetect::eliminatePositiveMinima();
  PeakDetect::eliminateNegativeMaxima();

  PeakDetect::reducePositiveMaxima();
  PeakDetect::reduceNegativeMinima();

  PeakDetect::markSharpPeaks();
  */

  // PeakDetect::markZeroCrossings();
  // PeakDetect::markQuiet();

  // PeakDetect::countPositiveRuns();
}


void PeakDetect::reduce()
{
  const bool debug = true;
  const bool debugDetails = false;

  if (debugDetails)
  {
    cout << "Original peaks: " << peaks.size() << "\n";
    PeakDetect::print();
  }

  PeakDetect::reduceSmallAreas(0.1f);
  if (debugDetails)
  {
    cout << "Non-tiny list peaks: " << peaks.size() << "\n";
    PeakDetect::print();
  }


  PeakDetect::eliminateKinks();
  if (debugDetails)
  {
    cout << "Non-kinky list peaks: " << peaks.size() << "\n";
    PeakDetect::print();
  }

  PeakDetect::estimateScales();
  if (debug)
  {
    cout << "Scale list\n";
    cout << scalesList.str(0) << endl;
  }

reduceNew();

  const float scaledCutoffList = AREA_CUTOFF * scalesList.getArea();
  if (debug)
    cout << "Area list cutoff " << scaledCutoffList << endl;

  PeakDetect::reduceSmallAreas(scaledCutoffList);
  if (debugDetails)
  {
    cout << "Reasonable list peaks: " << peaks.size() <<  endl;
    PeakDetect::print();
    cout << endl;
  }

  vector<PeakCluster> clusters;
  PeakDetect::runKmeansOnce(clusters);
  PeakDetect::countClusters(clusters);

  if (! PeakDetect::getConvincingClusters(clusters))
  {
    PeakDetect::printClusters(clusters, debugDetails);
    cout << "No convincing clusters found\n";
    return;
  }

  if (debug)
    PeakDetect::printClusters(clusters, debugDetails);

  bool firstSeen = false;
  unsigned firstTentativeIndex = peaks.front().getIndex();
  unsigned lastTentativeIndex = peaks.back().getIndex();
  numCandidates = 0;
  numTentatives = 0;

  for (auto& peak: peaks)
  {
    if (! peak.isCandidate())
      continue;

    numCandidates++;

    PeakType ptype = PeakDetect::classifyPeak(peak, clusters);

    if (ptype == PEAK_TENTATIVE)
    {
      numTentatives++;
      peak.select();
      if (! firstSeen)
      {
        firstTentativeIndex = peak.getIndex();
        firstSeen = true;
      }
      lastTentativeIndex = peak.getIndex();
    }

    peak.logType(ptype);
  }

  // Fix the early and late peaks to different types.
  for (auto& peak: peaks)
  {
    if (! peak.isCandidate() || 
        peak.getType() == PEAK_TENTATIVE)
      continue;

    if (peak.getIndex() < firstTentativeIndex)
      peak.logType(PEAK_TRANS_FRONT);
    else if (peak.getIndex() > lastTentativeIndex)
      peak.logType(PEAK_TRANS_BACK);
  }

  cout << numCandidates << " candidate peaks" << endl;
  if (numTentatives == 0)
    THROW(ERR_NO_PEAKS, "No tentative peaks");

}


void PeakDetect::logPeakStats(
  const vector<PeakPos>& posTrue,
  const string& trainTrue,
  const double speedTrue,
  PeakStats& peakStats)
{
  const bool debug = true;
  const bool debugDetails = false;

  const unsigned lt = posTrue.size();

  // Scale true positions to true times.
  vector<PeakTime> timesTrue;
  timesTrue.resize(lt);
  PeakDetect::pos2time(posTrue, speedTrue, timesTrue);

  if (debugDetails)
    PeakDetect::printPeaks(timesTrue);

  // Find a good line-up.
  double shift = 0.;
  if (! PeakDetect::findMatch(timesTrue, shift))
  {
    if (debug)
    {
      cout << "No good match to real " << posTrue.size() << " peaks.\n";
      cout << "\nTrue train " << trainTrue << " at " << 
        fixed << setprecision(2) << speedTrue << " m/s" << endl << endl;
    }
    return;
  }

  // Make statistics.
  vector<unsigned> seenTrue(posTrue.size(), 0);
  unsigned seen = 0;
  for (auto& peak: peaks)
  {
    if (peak.isCandidate())
    {
      const int m = peak.getMatch();
      const PeakType pt = peak.getType();
      if (m >= 0)
      {
        peakStats.logSeenMatch(static_cast<unsigned>(m), lt, pt);
        if (seenTrue[m] && debug)
          cout << "Already saw true peak " << m << endl;
        seenTrue[m]++;
        seen++;
      }
      else
        peakStats.logSeenMiss(pt);
    }
  }

  if (debug)
  {
    cout << "True train " << trainTrue << " at " << 
      fixed << setprecision(2) << speedTrue << " m/s\n\n";

    cout << seen << " of the observed peaks are close to true ones (" << 
      posTrue.size() << " true peaks)" << endl;
  }

  for (unsigned m = 0; m < posTrue.size(); m++)
  {
    if (seenTrue[m])
      continue;

    // Look for posTrue[m].time vs (peaks time - shift),
    // must be within TIME_PROXIMITY.
    PeakType ctype = PeakDetect::findCandidate(timesTrue[m].time, shift);
    if (ctype == PEAK_TENTATIVE)
    {
      // TODO Should either not happen, or be thrown.
      cout << "Odd: Tentative matched again\n";
    }

    // The missed ones are detected as early/late/missing.
    if (ctype == PEAK_TRUE_MISSING)
      peakStats.logTrueReverseMiss(m, lt);
    else
      peakStats.logTrueReverseMatch(m, lt, ctype);
  }
}


void PeakDetect::makeSynthPeaks(vector<float>& synthPeaks) const
{
  for (unsigned i = 0; i < synthPeaks.size(); i++)
    synthPeaks[i] = 0;

  // PeakDetect::makeSynthPeaksSharp(synthPeaks);

  PeakDetect::makeSynthPeaksQuietNew(synthPeaks);

  // PeakDetect::makeSynthPeaksLines(synthPeaks);

  // PeakDetect::makeSynthPeaksQuiet(synthPeaks);

  // PeakDetect::makeSynthPeaksLines(synthPeaks);

  // PeakDetect::makeSynthPeaksClassical(synthPeaks);
}


void PeakDetect::makeSynthPeaksQuietNew(vector<float>& synthPeaks) const
{
  for (auto& qc: quietCandidates)
  {
    if (qc.clusterNo != quietFavorite)
      continue;

    for (unsigned i = qc.start; i <= qc.start + qc.len; i++)
      synthPeaks[i] = qc.minLevel;
  }
}


void PeakDetect::makeSynthPeaksQuiet(vector<float>& synthPeaks) const
{
  unsigned index1 = 0;
  float value1 = 0.f;
  bool activeFlag = false;

  for (auto peak = peaksNew.begin(); peak != peaksNew.end(); peak++)
  {
    if (activeFlag)
    {
      if (peak->getQuietBegin())
        THROW(ERR_ALGO_PEAK_INTERVALS, "Two begins");
      else if (peak->getQuietEnd())
      {
        activeFlag = false;

        const unsigned index2 = peak->getIndex();
        const float value2 = peak->getValue();
        const float grad = (value2 - value1) / (index2 - index1);

        for (unsigned i = index1; i <= index2; i++)
          synthPeaks[i] = value1 + grad * (i-index1);
      }
    }
    else if (peak->getQuietEnd())
        THROW(ERR_ALGO_PEAK_INTERVALS, "Two ends");
    else if (peak->getQuietBegin())
    {
      activeFlag = true;
      index1 = peak->getIndex();
      value1 = peak->getValue();
    }
  }
}


void PeakDetect::makeSynthPeaksSharp(vector<float>& synthPeaks) const
{
  // Draw lines from sharp peaks.

  for (auto peak = peaksNew.begin(); peak != peaksNew.end(); peak++)
  {
    if (! peak->getSharp())
      continue;

    // Otherwise it will be an interior peak.
    auto peakPrev = prev(peak);
    auto peakNext = next(peak);

    unsigned index1 = peak->getIndex();
    unsigned index2 = peakNext->getIndex();
    float value1 = peak->getValue();
    float value2 = peakNext->getValue();
    float grad = (value2 - value1) / (index2 - index1);

    // Put zeroes at the peaks themselves.
    for (unsigned i = index1+1; i < index2; i++)
      synthPeaks[i] = value1 + grad * (i-index1);

    index1 = peakPrev->getIndex();
    index2 = peak->getIndex();
    value1 = peakPrev->getValue();
    value2 = peak->getValue();
    grad = (value2 - value1) / (index2 - index1);

    // Put zeroes at the peaks themselves.
    for (unsigned i = index1+1; i < index2; i++)
      synthPeaks[i] = value1 + grad * (i-index1);
  }
}


void PeakDetect::makeSynthPeaksLines(vector<float>& synthPeaks) const
{
  // Draw lines between all peaks.

  for (auto peak = peaksNew.begin(); peak != peaksNew.end(); peak++)
  {
    const auto peakNext = next(peak);
    if (peakNext == peaksNew.end())
     break;

    const float value1 = peak->getValue();
    const float value2 = peakNext->getValue();

    unsigned index1 = peak->getIndex();
    unsigned index2 = peakNext->getIndex();
    const float grad = (value2 - value1) / (index2 - index1);

    // Put zeroes at the peaks themselves.
    for (unsigned i = index1+1; i < index2; i++)
      synthPeaks[i] = value1 + grad * (i-index1);
  }
}


void PeakDetect::makeSynthPeaksPosLines(vector<float>& synthPeaks) const
{
  // Show lines whenever a positive maximum is involved.

  for (auto peak = peaksNew.begin(); peak != peaksNew.end(); peak++)
  {
    const auto peakNext = next(peak);
    if (peakNext == peaksNew.end())
     break;

    const float value1 = peak->getValue();
    const float value2 = peakNext->getValue();
    if (value1 < 0.f && value2 < 0.f)
      continue;

    unsigned index1 = peak->getIndex();
    unsigned index2 = peakNext->getIndex();
    const float grad = (value2 - value1) / (index2 - index1);

    // Only put zeroes at positive maxima.
    unsigned i1 = (peak->getValue() >= 0.f ? index1+1 : index1);
    if (peakNext->getValue() >= 0.f)
      index2--;

    for (unsigned i = i1; i <= index2; i++)
      synthPeaks[i] = value1 + grad * (i-index1);
  }
}


void PeakDetect::makeSynthPeaksClassical(vector<float>& synthPeaks) const
{
  for (auto& peak: peaks)
  {
    if (peak.isSelected())
      synthPeaks[peak.getIndex()] = peak.getValue();
  }
}


float PeakDetect::getFirstPeakTime() const
{
  for (auto& peak: peaks)
  {
    if (peak.isSelected())
      return peak.getIndex() / static_cast<float>(SAMPLE_RATE);
  }

  THROW(ERR_NO_PEAKS, "No output peaks");
}


void PeakDetect::getPeakTimes(vector<PeakTime>& times) const
{
  times.clear();
  const float t0 = PeakDetect::getFirstPeakTime();

  for (auto& peak: peaks)
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
  cout << peaks.front().strHeader();

  for (auto& peak: peaks)
    cout << peak.str(offset);
  cout << "\n";
}


void PeakDetect::printPeaks(const vector<PeakTime>& timesTrue) const
{
  cout << "true\n";
  for (unsigned i = 0; i < timesTrue.size(); i++)
    cout << i << ";" << 
      fixed << setprecision(6) << timesTrue[i].time << "\n";

  cout << "\nseen\n";
  unsigned pp = 0;
  for (auto& peak: peaks)
  {
    if (peak.isCandidate() && peak.getType() == PEAK_TENTATIVE)
    {
      cout << pp << ";" << 
        fixed << setprecision(6) << peak.getTime() << endl;
      pp++;
    }
  }
  cout << "\n";
}


void PeakDetect::printClustersDetail(
  const vector<PeakCluster>& clusters) const
{
  // Graded content of each cluster.
  vector<string> ctext(clusters.size());
  vector<unsigned> ccount(clusters.size());

  for (auto& peak: peaks)
  {
    if (! peak.isCandidate())
      continue;
    
    double m = peak.measure();

    stringstream ss;
    ss << setw(6) << right << peak.getIndex() + offset <<
      setw(8) << fixed << setprecision(2) << m;
    if (m <= MEASURE_CUTOFF)
      ss << " +";
    ss << endl;

    const unsigned c = peak.getCluster();
    ccount[c]++;
    ctext[c] += ss.str();
  }

  for (unsigned i = 0; i < clusters.size(); i++)
  {
    cout << "Cluster " << i << ": " << ccount[i];
    if (clusters[i].good)
      cout << " (good)";
    cout << "\n" << ctext[i] << endl;
  }
}


void PeakDetect::printClusters(
  const vector<PeakCluster>& clusters,
  const bool debugDetails) const
{
  // The actual cluster content.
  cout << "clusters" << endl;
  cout << clusters.front().centroid.strHeader();
  for (auto& c: clusters)
    cout << c.centroid.str(offset);
  cout << endl;

  // The peak indices in each cluster.
  for (unsigned i = 0; i < clusters.size(); i++)
  {
    cout << i << ":";
    for (auto& peak: peaks)
    {
      if (peak.isCluster(i))
        cout << " " << peak.getIndex() + offset;
    }
    cout << "\n";
  }
  cout << endl;

  if (debugDetails)
    PeakDetect::printClustersDetail(clusters);
}


void PeakDetect::printClustersSharpDetail(
  const list<Sharp>& sharps,
  const Sharp& sharpScale,
  const vector<SharpCluster>& clusters) const
{
  // Graded content of each cluster.
  vector<string> ctext(clusters.size());
  vector<unsigned> ccount(clusters.size());

  for (auto& sharp: sharps)
  {
    double m = 
      sharp.distance(clusters[sharp.clusterNo].centroid, sharpScale);

    stringstream ss;
    ss << setw(6) << right << sharp.index + offset <<
      setw(8) << fixed << setprecision(2) << m;
    if (m <= SHARP_CUTOFF)
      ss << " +";
    ss << endl;

    const unsigned c = sharp.clusterNo;
    ccount[c]++;
    ctext[c] += ss.str();
  }

  for (unsigned i = 0; i < clusters.size(); i++)
  {
    cout << "Cluster " << i << ": " << ccount[i];
    if (clusters[i].good)
      cout << " (good)";
    cout << "\n" << ctext[i] << endl;
  }
}


void PeakDetect::printSharpClusters(
  const list<Sharp>& sharps,
  const Sharp& sharpScale,
  const vector<SharpCluster>& clusters,
  const bool debugDetails) const
{
  // The actual cluster content.
  cout << "clusters" << endl;
  cout << clusters.front().centroid.strHeader();
  for (auto& c: clusters)
    cout << c.centroid.str(offset);
  cout << endl;

  // The peak indices in each cluster.
  for (unsigned i = 0; i < clusters.size(); i++)
  {
    cout << i << ":";
    for (auto& sharp: sharps)
    {
      if (sharp.clusterNo == i)
        cout << " " << sharp.index + offset;
    }
    cout << "\n";
  }
  cout << endl;

  if (debugDetails)
    PeakDetect::printClustersSharpDetail(sharps, sharpScale, clusters);
}



void PeakDetect::printClustersQuietDetail(
  const list<Period>& quiets,
  const vector<PeriodCluster>& clusters) const
{
  // Graded content of each cluster.
  vector<string> ctext(clusters.size());
  vector<unsigned> ccount(clusters.size());

  for (auto& quiet: quiets)
  {
    double m = 
      quiet.distance(clusters[quiet.clusterNo].centroid);

    stringstream ss;
    ss << setw(6) << right << quiet.start + offset <<
      setw(8) << fixed << setprecision(2) << m;
    if (m <= SHARP_CUTOFF)
      ss << " +";
    ss << endl;

    const unsigned c = quiet.clusterNo;
    ccount[c]++;
    ctext[c] += ss.str();
  }

  for (unsigned i = 0; i < clusters.size(); i++)
  {
    cout << "Cluster " << i << ": " << ccount[i];
    cout << "\n" << ctext[i] << endl;
  }
}


void PeakDetect::printQuietClusters(
  const list<Period>& quiets,
  const vector<PeriodCluster>& clusters,
  const bool debugDetails) const
{
  // The actual cluster content.
  cout << "clusters" << endl;
  cout << clusters.front().centroid.strHeader();
  for (auto& c: clusters)
    cout << c.centroid.str(offset);
  cout << endl;

  // The peak indices in each cluster.
  for (unsigned i = 0; i < clusters.size(); i++)
  {
    cout << i << ":";
    for (auto& quiet: quiets)
    {
      if (quiet.clusterNo == i)
        cout << " " << quiet.start + offset << "-" <<
          quiet.start + offset + quiet.len;
    }
    cout << "\n";
  }
  cout << endl;

  if (debugDetails)
    PeakDetect::printClustersQuietDetail(quiets, clusters);
}

