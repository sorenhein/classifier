#include <list>
#include <iostream>
#include <iomanip>
#include <functional>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <random>

#include "PeakDetect.h"
#include "PeakStats.h"
#include "Except.h"

extern PeakStats peakStats;


#define SMALL_AREA_FACTOR 100.f

// Only a check limit, not algorithmic parameters.
#define ABS_RANGE_LIMIT 1.e-4
#define ABS_AREA_LIMIT 1.e-4

#define SAMPLE_RATE 2000.

#define TIME_PROXIMITY 0.03 // s

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
  while (peak != peakList.end() && ! peak->isSelected())
    peak++;
  return (peak != peakList.end());
}


double PeakDetect::simpleScore(
  const vector<PeakTime>& timesTrue,
  const double offsetScore,
  const bool logFlag)
{
  // This is similar to Align::simpleScore.
  
/*
cout << "offsetScore " << offsetScore << endl << endl;
cout << "true\n";
for (unsigned tno = 0; tno < timesTrue.size(); tno++)
{
  cout << tno << ";" << fixed << setprecision(4) << timesTrue[tno].time << "\n";
}
cout << "\nseen\n";
unsigned pp = 0;
for (auto& peak: peakList)
{
  if (peak.isSelected())
  {
    cout << pp << "," << fixed << setprecision(4) << peak.getTime() << "\n";

  pp++;
  }
}
*/

  list<Peak>::iterator peak = peakList.begin();
  if (! PeakDetect::advance(peak))
    THROW(ERR_NO_PEAKS, "No peaks present or selected");
  const auto peakFirst = peak;

  list<Peak>::iterator peakBest, peakPrev;

  double score = 0.;
  for (unsigned tno = 0; tno < timesTrue.size(); tno++)
  {
    double d = TIME_PROXIMITY;
    const double timeTrue = timesTrue[tno].time;
    double timeSeen = peak->getTime() - offsetScore;

    d = timeSeen - timeTrue;
    if (d >= 0.)
      peakBest = peak;
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
        d = -dright;
        peakBest = peakPrev;
      }
      else 
      {
        dleft = timeTrue - (peakPrev->getTime() - offsetScore);
        if (dleft <= dright)
        {
          d = dleft;
          peakBest = peakPrev;
        }
        else
        {
          d = dright;
          peakBest = peak;
        }
      }
    }

    if (d <= TIME_PROXIMITY)
    {
      score += (TIME_PROXIMITY - d) / TIME_PROXIMITY;
      if (logFlag)
        peakBest->logMatch(tno);
    }
  }

  return score;
}


bool PeakDetect::findMatch(const vector<PeakTime>& timesTrue)
{
  const unsigned lp = peakList.size();
  const unsigned lt = timesTrue.size();

  // Way too many, but we have late transients at the moment.
  const unsigned maxShiftSeen = 5;
  const unsigned maxShiftTrue = 3;
  if (lp <= maxShiftSeen || lt <= maxShiftTrue)
    THROW(ERR_NO_PEAKS, "Not enough peaks");

  // Offsets are subtracted from the seen peaks (peakList).
  // TODO Put time as a field of Peak?
  vector<double> offsetList(maxShiftSeen + maxShiftTrue + 1);
  list<Peak>::const_iterator peak = peakList.end();
  for (unsigned i = 0; i <= maxShiftSeen; i++)
  {
    do
    {
      peak = prev(peak);
    }
    while (! peak->isSelected());

    offsetList[i] = peak->getTime() - timesTrue[lt-1].time;
  }

  peak = peakList.end();
  do
  {
    peak = prev(peak);
  }
  while (! peak->isSelected());
  const double lastTime = peak->getTime();

  for (unsigned i = 1; i <= maxShiftTrue; i++)
    offsetList[maxShiftSeen+i] = timesTrue[lt-1-i].time - lastTime;

  double score = 0.;
  unsigned ino = 0;
  for (unsigned i = 0; i < offsetList.size(); i++)
  {
    double scoreNew = 
      PeakDetect::simpleScore(timesTrue, offsetList[i], false);
cout << "i " << i << ": " << scoreNew << endl;
    if (scoreNew > score)
    {
cout << "*\n";
      score = scoreNew;
      ino = i;
    }
  }

  // This extra run could be eliminated at the cost of some copying
  // and intermetidate storage.
  score = PeakDetect::simpleScore(timesTrue, offsetList[ino], true);

  // TODO
  if (score < 0.75 * lt)
    return false;
  else
    return true;
}
  

void PeakDetect::reduce(
  const vector<PeakPos>& posTrue,
  const double speedTrue)
{
  UNUSED(posTrue);
  UNUSED(speedTrue);

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

  /*
  cout << peakList.front().strHeader();
  for (auto peak = next(peakList.begin()); peak != peakList.end(); peak++)
  {
    if (peak->isCandidate())
      cout << peak->str(offset);
  }
  cout << "\n";
  */

  vector<string> ctext(koptions.numClusters);
  vector<unsigned> ccount(koptions.numClusters);

  for (auto peak = next(peakList.begin());
      peak != peakList.end(); peak++)
  {

    if (peak->isCandidate())
    {
stringstream ss;
ss << setw(6) << right << peak->getIndex() + offset <<
  setw(8) << fixed << setprecision(2) << peak->measure(scalesList);
      if (peak->measure(scalesList) <= 1.f)
      {
ss << " +";
        peak->select();

        // TODO Better types.
        peak->logType(PEAK_TENTATIVE);
      }
ss << endl;
ctext[peak->getCluster()] += ss.str();
ccount[peak->getCluster()]++;
    }
  }

  for (unsigned i = 0; i < koptions.numClusters; i++)
  {
    cout << "Cluster " << i << ": " << ccount[i] << "\n" << 
      ctext[i] << endl;
  }


  // Scale true positions to true times.
  vector<PeakTime> timesTrue;
  timesTrue.resize(posTrue.size());
  PeakDetect::pos2time(posTrue, speedTrue, timesTrue);

  // Find a good line-up.
  if (! PeakDetect::findMatch(timesTrue))
  {
    cout << "No good match to real peaks.\n";
    return;
  }

  // Make statistics.
  unsigned pno = 0;
unsigned seen = 0;
  vector<unsigned> seenTrue(posTrue.size(), 0);
  for (auto peak = next(peakList.begin());
      peak != peakList.end(); peak++, pno++)
  {
    if (peak->isCandidate())
    {
      const int m = peak->getMatch();
      peakStats.log(m, pno, peak->getType());
      pno++;

      if (m != -1)
      {
if (seenTrue[m])
  cout << "Already saw true peak " << m << endl;
        seenTrue[m]++;
seen++;
      }
    }
  }
cout << "Saw " << seen << " of the true peaks\n";

  for (unsigned m = 0; m < posTrue.size(); m++)
  {
    if (! seenTrue[m])
    {
      PeakType type;
      if (m <= 2)
        type = PEAK_TOO_EARLY;
      else if (m+2 >= posTrue.size())
        type = PEAK_TOO_LATE;
      else
        type = PEAK_MISSING;

      peakStats.log(m, -1, type);
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

