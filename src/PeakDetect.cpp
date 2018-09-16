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


#define KINK_RATIO 100.f

#define SAMPLE_RATE 2000.

#define TIME_PROXIMITY 0.03 // s

#define SCORE_CUTOFF 0.75

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
  peakList.emplace_back(Peak());

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
  peakList.back().logSentinel(samples[0], maxFlag);
}


void PeakDetect::logLast(const vector<float>& samples)
{
  const auto& peakPrev = peakList.back();
  const float areaFull = 
    PeakDetect::integrate(samples, peakPrev.getIndex(), len-1);
  const float areaCumPrev = peakPrev.getAreaCum();

  peakList.emplace_back(Peak());
  peakList.back().log(
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
  peakList.clear();

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

    const auto& peakPrev = peakList.back();
    const float areaFull = PeakDetect::integrate(samples, 
      peakPrev.getIndex(), i);
    const float areaCumPrev = peakPrev.getAreaCum();

    // The peak contains data for the interval preceding it.
    peakList.emplace_back(Peak());
    peakList.back().log(i, samples[i], areaCumPrev + areaFull, maxFlag);
  }

  // The last peak is a dummy extremum at the last sample.
  PeakDetect::logFirst(samples);

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
    (peak1 == peakList.begin() ? &*peak1 : &*prev(peak1));
  Peak * peakN = 
    (next(peak2) == peakList.end() ? nullptr : &*next(peak2));

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
      if (peakCurrent != peakList.end())
        peakList.erase(peakCurrent, peakList.end());
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


void PeakDetect::pickStartingClusters(
  vector<Peak>& clusters,
  const unsigned n) const
{
  clusters.resize(n);

  // TODO Pick these more intelligently.

  unsigned c = 0;
  for (auto peak = peakList.begin(); peak != peakList.end(); peak++)
  {
    if (peak->isCandidate())
    {
      clusters[c] = * peak;
      c++;
      if (c == n)
        break;
    }
  }
}


void PeakDetect::runKmeansOnce(
  const Koptions& koptions,
  vector<Peak>& clusters,
  float& distance)
{
  // Pick centroids from the data points.
  PeakDetect::pickStartingClusters(clusters, koptions.numClusters);

  distance = numeric_limits<float>::max();

  for (unsigned iter = 0; iter < koptions.numIterations; iter++)
  {
    vector<Peak> clustersNew(koptions.numClusters);
    vector<unsigned> counts(koptions.numClusters, 0);
    float distGlobal = 0.f;

    for (auto peak = peakList.begin(); peak != peakList.end(); peak++)
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
  const bool logFlag,
  double& shift)
{
  // This is similar to Align::simpleScore.
  
  list<Peak>::iterator peak = peakList.begin();
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
  const unsigned lp = peakList.size();
  const unsigned lt = timesTrue.size();

  // Probably too many.
  const unsigned maxShiftSeen = 3;
  const unsigned maxShiftTrue = 3;
  if (lp <= maxShiftSeen || lt <= maxShiftTrue)
    THROW(ERR_NO_PEAKS, "Not enough peaks");

  offsetList.resize(maxShiftSeen + maxShiftTrue + 1);

  // Offsets are subtracted from the seen peaks (peakList).
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
cout << "Final score " << score << endl << endl;

  return (score >= SCORE_CUTOFF * timesTrue.size());
}
  

PeakType PeakDetect::findCandidate(
  const double time,
  const double shift) const
{
  double dist = numeric_limits<double>::max();
  PeakType type = PEAK_TRUE_MISSING;

  for (auto& peak: peakList)
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


void PeakDetect::reduce()
{
  PeakDetect::reduceSmallAreas(0.1f);

// cout << "Non-tiny list peaks: " << peakList.size() << "\n";
// PeakDetect::print();

  PeakDetect::eliminateKinks();

// cout << "Non-kinky list peaks: " << peakList.size() << "\n";
// PeakDetect::print();

  PeakDetect::estimateScales();

// cout << "Scale list\n";
// cout << scalesList.str(0) << "\n";

  const float scaledCutoffList = 0.05f * scalesList.getArea();

cout << "Area list cutoff " << scaledCutoffList << endl;

  PeakDetect::reduceSmallAreas(scaledCutoffList);

// cout << "Reasonable list peaks: " << peakList.size() <<  endl;
//
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

  // A "good" cluster has at least 3 entries with a low measure,
  // and at least 80% such entries.
  vector<unsigned> cplus(koptions.numClusters);
  vector<bool> cgood(koptions.numClusters);

  for (auto peak = next(peakList.begin()); peak != peakList.end(); peak++)
  {
    if (! peak->isCandidate())
      continue;
    
    const unsigned c = peak->getCluster();
    const float m = peak->measure(scalesList);
    ccount[c]++;
    
stringstream ss;
ss << setw(6) << right << peak->getIndex() + offset <<
  setw(8) << fixed << setprecision(2) << m;
    if (m <= 1.f)
    {
      cplus[c]++;
ss << " +";
    }
ss << endl;
ctext[c] += ss.str();

  }

  for (unsigned i = 0; i < koptions.numClusters; i++)
  {
    if (ccount[i] >= 3 && cplus[i] >= 0.8 * ccount[i])
      cgood[i] = true;
    else
      cgood[i] = false;
cout << "cgood[" << i << "] = " << cgood[i] << endl;
  }

  unsigned nraw = 0;
  for (auto peak = next(peakList.begin());
      peak != peakList.end(); peak++)
  {
    if (! peak->isCandidate())
      continue;

    const unsigned c = peak->getCluster();
    const float m = peak->measure(scalesList);
    nraw++;

    PeakType ptype;
    if (cgood[c])
    {
      if (m <= 1.f)
      {
        ptype = PEAK_TENTATIVE;
        peak->select();
      }
      else if (m <= 2.f)
        ptype = PEAK_POSSIBLE;
      else if (m <= 6.f)
        ptype = PEAK_CONCEIVABLE;
      else
        ptype = PEAK_REJECTED;
    }
    else
    {
      if (m <= 2.f)
        ptype = PEAK_POSSIBLE;
      else if (m <= 6.f)
        ptype = PEAK_CONCEIVABLE;
      else
        ptype = PEAK_REJECTED;
    }
    peak->logType(ptype);
  }

cout << nraw << " candidate peaks\n";


  for (unsigned i = 0; i < koptions.numClusters; i++)
  {
    cout << "Cluster " << i << ": " << ccount[i] << "\n" << 
      ctext[i] << endl;
  }

}


void PeakDetect::logPeakStats(
  const vector<PeakPos>& posTrue,
  const string& trainTrue,
  const double speedTrue,
  PeakStats& peakStats)
{
  const unsigned lt = posTrue.size();

  // Scale true positions to true times.
  vector<PeakTime> timesTrue;
  timesTrue.resize(lt);
  PeakDetect::pos2time(posTrue, speedTrue, timesTrue);

/*
cout << "true\n";
for (unsigned i = 0; i < lt; i++)
  cout << i << ";" << fixed << setprecision(6) << timesTrue[i].time << "\n";cout << "\nseen\n";
unsigned pp = 0;
for (auto peak = peakList.begin(); peak != peakList.end(); peak++)
{
  if (peak->isCandidate() && peak->getType() == PEAK_TENTATIVE)
  {
    cout << pp << ";" << fixed << setprecision(6) << peak->getTime() << endl;
    pp++;
  }
}
cout << "\n";
*/

  // Find a good line-up.
  double shift = 0.;
  if (! PeakDetect::findMatch(timesTrue, shift))
  {
    cout << "No good match to real " << posTrue.size() << " peaks.\n";
cout << "\nTrue train " << trainTrue << " at " << 
  fixed << setprecision(2) << speedTrue << " m/s" << endl << endl;
    return;
  }

  // Make statistics.
unsigned seen = 0;
  vector<unsigned> seenTrue(posTrue.size(), 0);
  for (auto peak = peakList.begin(); peak != peakList.end(); peak++)
  {
    if (peak->isCandidate())
    {
      const int m = peak->getMatch();
      if (m >= 0)
      {
        peakStats.logSeenMatch(static_cast<unsigned>(m), lt,
          peak->getType());
      }
      else
      {
        peakStats.logSeenMiss(peak->getType());
      }

      if (m != -1)
      {
if (seenTrue[m])
  cout << "Already saw true peak " << m << endl;
        seenTrue[m]++;
seen++;
      }
    }
  }

cout << "True train " << trainTrue << " at " << 
  fixed << setprecision(2) << speedTrue << " m/s\n\n";

cout << seen << " of the observed peaks are close to true ones (" << posTrue.size() << " true peaks)" << endl;

  for (unsigned m = 0; m < posTrue.size(); m++)
  {
    if (seenTrue[m])
      continue;

    // Look for posTrue[m].time vs (peakList time - shift),
    // must be within TIME_PROXIMITY.
    PeakType ctype = PeakDetect::findCandidate(timesTrue[m].time, shift);
    if (ctype == PEAK_TENTATIVE)
    {
      cout << "Odd: Tentative matched again\n";
    }

    // -1 is right for a missing peak.
    // The peak numbers for seen peaks are not meaningful,
    // should be changed.
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

