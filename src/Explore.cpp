#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "Explore.h"
#include "Peak.h"
#include "PeakPtrs.h"
#include "AccelDetect.h"

#include "const.h"

#define WINDOW_LOBE 4
#define CORR_LOBE 20
#define COARSE_LOBE 100


Explore::Explore()
{
  Explore::reset();
}


Explore::~Explore()
{
}


void Explore::reset()
{
  filtered.clear();
  data.clear();
}


void Explore::log(
  const list<QuietInterval>& actives,
  const PeakPtrs& candidates)
{
  auto pi = candidates.cbegin();
  auto pe = candidates.cend();

  for (auto& act: actives)
  {
    // Would no doubt be cleaner to avoid this in Quiet.cpp
    if (act.len == 0)
      continue;

    data.push_back(Datum());
    Datum& datum = data.back();
    datum.qint = &act;

/*
    while (pi != pe && (* pi)->getIndex() < act.first)
     pi++;
     if (pi == pe)
       continue;

    while (pi != pe && (* pi)->getIndex() <= act.first + act.len)
    {
      datum.peaksOrig.push_back(* pi);
      pi++;
    }
*/
  }
}


float Explore::filterEdge(
  const vector<float>& accel,
  const unsigned pos,
  const unsigned lobe) const
{
  float f = accel[pos];
  for (unsigned j = 1; j <= lobe; j++)
    f += accel[pos-j] + accel[pos+j];
  return f / (2*lobe+1);
}


void Explore::filter(
  const vector<float>& integrand,
  vector<float>& result)
{
  // For now this is a simple moving average.
  result.resize(integrand.size());

  // Front and back.
  const unsigned al = integrand.size();
  for (unsigned i = 0; i <= WINDOW_LOBE; i++)
  {
    result[i] = Explore::filterEdge(integrand, i, i);
    result[al-i-1] = Explore::filterEdge(integrand, al-i-1, i);
  }

  float cum = 0;
  for (unsigned i = 0; i < 2*WINDOW_LOBE + 1; i++)
    cum += integrand[i];
  const float factor = 1.f / (2.f*WINDOW_LOBE + 1.f);

  for (unsigned i = WINDOW_LOBE+1; i < al-WINDOW_LOBE-1; i++)
  {
    cum += integrand[i+WINDOW_LOBE] - integrand[i-WINDOW_LOBE-1];
    result[i] = factor * cum;
  }
}


void Explore::setupGaussian(const float& sigma)
{
  const unsigned lobe = static_cast<unsigned>(3.f * sigma);

  gaussian.clear();
  gaussian.resize(lobe+1);

  const float factor = 1.f / (2.f * sigma * sigma);
  gaussian[0] = 1.f;
  float sum = 1.f;
  for (unsigned i = 1; i <= lobe; i++)
  {
    gaussian[i] = exp(- (i*i * factor));
    sum += 2.f * gaussian[i]; // Positive and negative lobe
  }

  for (unsigned i = 0; i <= lobe; i++)
    gaussian[i] /= sum;
}


float Explore::filterEdgeGaussian(
  const vector<float>& accel,
  const unsigned pos,
  const unsigned lobe) const
{
  float f = gaussian[0] * accel[pos];
  float coeffSum = gaussian[0];

  for (unsigned j = 1; j <= lobe; j++)
  {
    f += gaussian[j] * (accel[pos-j] + accel[pos+j]);
    coeffSum += 2.f * gaussian[j];
  }
  return f / coeffSum;
}


void Explore::filterGaussian(
  const vector<float>& integrand,
  vector<float>& result)
{
  result.resize(integrand.size());

  // Front and back.
  const unsigned al = integrand.size();
  const unsigned lobe = gaussian.size()-1;
  for (unsigned i = 0; i <= lobe; i++)
  {
    result[i] = Explore::filterEdgeGaussian(integrand, i, i);
    result[al-i-1] = Explore::filterEdgeGaussian(integrand, al-i-1, i);
  }

  // Center.
  for (unsigned i = lobe+1; i < al-lobe-1; i++)
  {
    result[i] = gaussian[0] * integrand[i];
    for (unsigned j = 1; j <= lobe; j++)
      result[i] += gaussian[j] * (integrand[i+j] + integrand[i-j]);
  }
}


void Explore::integrate(
  const vector<float>& accel,
  const unsigned start,
  const unsigned end,
  const float sampleRate,
  vector<float>& speed) const
{
  // Speed is a simple integral, uncompensated for drift.  It is zero-based
  // no matter where the acceleration segment starts.
  
  // Speed is in 0.01 m/s.
  const float factor = 100.f * G_FORCE / sampleRate;

  speed[0] = factor * accel[start];

  for (unsigned i = start+1; i < end; i++)
    speed[i-start] = speed[i-start-1] + factor * accel[i];
}


void Explore::differentiate(
  const vector<float>& position,
  const float sampleRate,
  vector<float>& speed) const
{
  // As speed is in 0.01 m/s, we need this factor to get back from
  // the position to speed.
  const float factor = sampleRate / 100.f;

  speed[0] = 0.f;
  for (unsigned i = 0; i < position.size(); i++)
    speed[i] = factor * (position[i] - position[i-1]);
}


void Explore::correlateFixed(
  const QuietInterval& first,
  const QuietInterval& second,
  const unsigned offset,
  Correlate& corr) const
{
  // The first interval is before the second one (or equal to it), 
  // and the offset is therefore always non-negative.

  const unsigned lenMin = min(first.len, second.len);

  const unsigned lower1 = first.first + offset;
  const unsigned upper1 = lower1 + lenMin;
  const unsigned lower = second.first;
  const unsigned upper = min(lower + lenMin, filtered.size()-1);
  
  corr.shift = offset;
  corr.overlap = upper - lower;
  
// cout << "correlating (" << lower-offset << ", " << 
  // upper-offset << ") against (" <<
    // lower << ", " << upper << ")\n";
  corr.value = 0.;
  for (unsigned i = lower; i < upper; i++)
    corr.value += filtered[i-offset] * filtered[i];
}


unsigned Explore::guessShiftSameLength(
  vector<Peak const *>& first,
  vector<Peak const *>& second,
  unsigned& devSq) const
{
  int sum1 = 0, sum2 = 0;
  for (unsigned i = 0; i < first.size(); i++)
  {
    sum1 += static_cast<int>(first[i]->getIndex());
    sum2 += static_cast<int>(second[i]->getIndex());
  }

  const unsigned shift = (sum2-sum1) / first.size();


  devSq = 0;
  for (unsigned i = 0; i < first.size(); i++)
  {
    const int fi = static_cast<int>(first[i]->getIndex()) + shift;
    const int si = static_cast<int>(second[i]->getIndex());
    devSq += static_cast<unsigned>((si-fi) * (si-fi));
  }
  return devSq;
}


unsigned Explore::guessShiftFirstLonger(
  vector<Peak const *>& first,
  vector<Peak const *>& second) const
{
  vector<Peak const *> alt;
  alt.resize(second.size());

  const unsigned d = first.size() - second.size();
  unsigned devSq;
  unsigned devBest = numeric_limits<unsigned>::max();
  unsigned shift, shiftBest = 0;

  for (unsigned i = 0; i < d; i++)
  {
    for (unsigned j = 0; j < second.size(); j++)
      alt[j] = first[i+j];

    shift = Explore::guessShiftSameLength(alt, second, devSq);
    if (devSq < devBest)
    {
      shiftBest = shift;
      devBest = devSq;
    }
  }
  return shiftBest;
}


unsigned Explore::guessShift(
  vector<Peak const *>& first,
  vector<Peak const *>& second) const
{
  unsigned tmp;

  if (first.size() == second.size())
    return Explore::guessShiftSameLength(first, second, tmp);
  else if (first.size() > second.size())
    return Explore::guessShiftFirstLonger(first, second);
  else
    return Explore::guessShiftFirstLonger(second, first);
}


#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))
void Explore::correlateBest(
  const QuietInterval& first,
  const QuietInterval& second,
  const unsigned guess,
  const float refValue,
  Correlate& corr) const
{
  Correlate tmp;
  Explore::correlateFixed(first, second, guess, corr);
cout << " base correlate " << corr.value << endl;

  UNUSED(refValue);
  // unsigned guessImproved = guess;
  // if (corr.value < refValue / 3.)
  // {
    // Take some larger steps.
  // }

  // Pretty basic search, one step at a time, only as much as needed.
  unsigned iopt = 0;
  for (unsigned i = 1; i < CORR_LOBE; i++)
  {
    Explore::correlateFixed(first, second, guess+i, tmp);
cout << "  forward by " << i << ": " << tmp.value << "\n";
    if (tmp.value > corr.value)
    {
      iopt = i;
      corr = tmp;
    }
    else
      break;
  }

  if (iopt > 0)
    return;

  for (unsigned i = 1; i < CORR_LOBE; i++)
  {
    Explore::correlateFixed(first, second, guess-i, tmp);
cout << "  backward by " << i << ": " << tmp.value << "\n";
    if (tmp.value > corr.value)
    {
      iopt = i;
      corr = tmp;
    }
    else
      break;
  }
}


unsigned Explore::powerOfTwo(const unsigned len) const
{
  if (len > 32768)
    return 0;
  else if (len > 16384)
    return 32768;
  else if (len > 8192)
    return 16384;
  else if (len > 4096)
    return 8192;
  else
    return 4096;
}


void Explore::setCorrelands(
  const vector<float>& accel,
  const unsigned len,
  const unsigned lower,
  const unsigned upper,
  vector<float>& bogieRev,
  vector<float>& wheel1Rev,
  vector<float>& wheel2Rev) const
{
  bogieRev.resize(len);
  wheel1Rev.resize(len);
  wheel2Rev.resize(len);

  // Reversed in preparation for convolution.
  for (unsigned i = lower; i < upper; i++)
    bogieRev[upper-1-i] = accel[i];

  const unsigned mid = (lower + upper) / 2;
  for (unsigned i = mid; i < upper; i++)
    wheel2Rev[upper-1-i] = accel[i];

  for (unsigned i = lower; i < mid; i++)
    wheel1Rev[upper-1-i] = accel[i];

/*
  cout << "Correlands\n";
  for (unsigned i = 0; i < upper-lower; i++)
    cout << i << ";" << bogieRev[i] << ";" << wheel1Rev[i] << ";" <<
      wheel2Rev[i] << endl;
  cout << "\n";
*/
}


void Explore::findCorrelationPeaks(
  const Peaks& corrPeaks,
  const float ampl,
  const unsigned spacing,
  list<unsigned>& bogieEnds)
{
  // ampl and spacing are estimates, but are derived from the signal, so
  // presumably they are not too far off.

  bogieEnds.clear();

  Pciterator pit0 = corrPeaks.cbegin();
  if (pit0->isMinimum())
    pit0 = next(pit0);
  if (pit0 == corrPeaks.cend())
    return;

  Pciterator pit1 = corrPeaks.next(pit0, &Peak::isMaximum, true);
  if (pit1 == corrPeaks.cend())
    return;

  Pciterator pit2 = corrPeaks.next(pit1, &Peak::isMaximum, true);
  if (pit2 == corrPeaks.cend())
    return;

  const unsigned spacingLo = static_cast<unsigned>(spacing * 0.75f);
  const unsigned spacingHi = static_cast<unsigned>(spacing * 1.25f);

cout << "Spacing " << spacing << endl;

  while (true)
  {
    const float level0 = pit0->getValue();
    const float level1 = pit1->getValue();
    const float level2 = pit2->getValue();
    const unsigned pos0 = pit0->getIndex();
    const unsigned pos1 = pit1->getIndex();
    const unsigned pos2 = pit2->getIndex();
    const unsigned diff10 = pos1 - pos0;
    const unsigned diff21 = pos2 - pos1;

cout << "pos1 " << pos1 << ", levels " << level0 << ", " << level1 << ", " << level2 << ", " <<
  diff10 << ", " << diff21 << endl;
  
    if (level0 > 0.2f * level1 &&
        level1 > 0.4f * ampl &&
        level2 > 0.2f * level1 &&
        level0 < level1 &&
        level2 < level1)
        // diff10 >= spacingLo &&
        // diff21 >= spacingLo &&
        // diff21 <= spacingHi &&
        // diff10 <= spacingHi)
    {
cout << "HIT " << pos1 << "\n";
      // Plausible correlation peak.
      bogieEnds.push_back(pos1);
    }

    pit0 = pit1;
    pit1 = pit2;
    pit2 = corrPeaks.next(pit1, &Peak::isMaximum, true);
    if (pit2 == corrPeaks.cend())
      return;
  }
}


void Explore::findSpeedBumps(
  const vector<float>& position, 
  const unsigned offset,
  const list<unsigned>& bogieEnds, 
  const unsigned activeLen,
  list<BogieTimes>& speedBumps)
{
  // Add 20%
  const unsigned tol = static_cast<unsigned>(0.2f * activeLen);
  unsigned lowerExt, upperExt;

  for (unsigned bend: bogieEnds)
  {
    if (bend < activeLen + tol)
      lowerExt = 0;
    else
      lowerExt = bend - activeLen - tol;
    
    if (bend + tol >= position.size())
      upperExt = position.size();
    else
      upperExt = bend + tol;

cout << "Trying bogie end " << bend << ", range " <<
  lowerExt << " to " << upperExt << endl;
cout << "--------------------------\n\n";

  unsigned min1index, min2index;
  if (Explore::getMinima(position, offset, lowerExt, upperExt,
      min1index, min2index))
  {
    cout << "Minima " << min1index << " and " << min2index << endl;
      speedBumps.emplace_back(BogieTimes());
      BogieTimes& bp = speedBumps.back();
      bp.first = min1index;
      bp.second = min2index;
  }
  else
  {
    cout << "No two minima\n";
    return;
  }
/*
    // Integrate the bogie into a rough speed segment.
    vector<float> speed;
    speed.resize(upperExt - lowerExt);
    Explore::integrate(accel, lowerExt, upperExt, sampleRate, speed);
      
    // Find the main peaks in this narrow speed segment.
    AccelDetect speedDetect;
    speedDetect.log(speed, lowerExt, upperExt - lowerExt);
    speedDetect.extract("speed");

    unsigned lower, upper, spacing;
    if (speedDetect.getLimits(lower, upper, spacing))
    {
      speedBumps.emplace_back(BogieTimes());
      BogieTimes& bp = speedBumps.back();
      bp.first = lowerExt + lower;
      bp.second = lowerExt + lower + spacing;

      cout << "Got limits " << lowerExt+lower << 
        " and " << lowerExt+upper << 
        ", spacing " << spacing << endl;
    }
    else
      cout << "Bad limits\n";
*/
  }
}

#define REGRESS_HALF_SIZE 4
#include "align/PolynomialRegression.h"

void Explore::findExactZC(
  const vector<float>& position,
  const unsigned i,
  ZCinterval& zc) const
{
  vector<float> x, y;
  x.resize(2*REGRESS_HALF_SIZE+1);
  y.resize(2*REGRESS_HALF_SIZE+1);

  for (unsigned j = i-REGRESS_HALF_SIZE, k = 0; 
      j <= i+REGRESS_HALF_SIZE; j++, k++)
  {
    x[k] = static_cast<float>(j);
    y[k] = position[j];
  }

  PolynomialRegression regr;
  vector<float> coeffs;
  regr.fitIt(x, y, 1, coeffs);

  zc.endExact = - coeffs[0] / coeffs[1];
  zc.slopeEnd = 2000.f * coeffs[1];
}
  

void Explore::makeZCintervals(
  const vector<float>& position,
  const unsigned posStart,
  const unsigned posEnd,
  const float level,
  list<ZCinterval>& ZCmaxima,
  list<ZCinterval>& ZCminima) const
{
  unsigned pStart = posStart;
  unsigned iMax = numeric_limits<unsigned>::max();
  float vMax = 0.f;
  bool posFlag = (position[posStart] >= level);
  unsigned countMax = 0;
  unsigned countMin = 0;

  for (unsigned i = posStart; i < posEnd-1; i++)
  {
    if ((posFlag && position[i] > vMax) ||
        (! posFlag && position[i] < vMax))
    {
      iMax = i;
      vMax = position[i];
    }

    if (posFlag && (position[i+1] < level || i+2 == posEnd))
    {
      // Crossing downward, coming from a maximum.
      ZCmaxima.emplace_back(ZCinterval());
      ZCinterval& zc = ZCmaxima.back();
      zc.start = pStart;
      zc.end = i;
      zc.index = iMax;
      zc.value = vMax;
      zc.count = countMax++;
      pStart = i+1;
      vMax = 0.f;
      posFlag = false;

      Explore::findExactZC(position, i, zc);
    }
    else if (! posFlag && (position[i+1] >= level || i+2 == posEnd))
    {
      ZCminima.emplace_back(ZCinterval());
      ZCinterval& zc = ZCminima.back();
      zc.start = pStart;
      zc.end = i;
      zc.index = iMax;
      zc.value = vMax;
      zc.count = countMin++;
      pStart = i+1;
      posFlag = true;

      Explore::findExactZC(position, i, zc);

      if (! ZCmaxima.empty())
      {
        zc.startExact = ZCmaxima.back().endExact;
        zc.slopeStart = ZCmaxima.back().slopeEnd;
      }
      else
      {
        zc.startExact = 0.f;
        zc.slopeStart = 0.f;
      }
    }
  }
}


bool Explore::getMinima(
  const vector<float>& position, 
  const unsigned offset, 
  const unsigned start, 
  const unsigned end, 
  unsigned& min1index,
  unsigned& min2index) const
{
  if (start < offset)
  {
    cout << "Explore::getMinima: Odd start\n";
    return false;
  }

  // ZCminima is an upward zero crossing, coming from a minimum.
  list<ZCinterval> ZCmaxima;
  list<ZCinterval> ZCminima;
  Explore::makeZCintervals(position, start-offset, end-offset, 0.f,
    ZCmaxima, ZCminima);

  if (ZCminima.size() < 2)
  {
    cout << "Explore::getLimits: Too few ZC minima\n";
    return false;
  }

  list<ZCinterval> ZCminimaSorted = ZCminima;
  ZCminimaSorted.sort([](const ZCinterval& zc1, const ZCinterval& zc2)
  {
    return (zc1.value < zc2.value);
  });

  auto zcit1 = ZCminimaSorted.begin();
  auto zcit2 = next(zcit1);

  if (zcit1->count+1 == zcit2->count)
  {
    // Order is 1, 2
  }
  else if (zcit2->count+1 == zcit1->count)
  {
    // Swap the order
    auto tmp = zcit1;
    zcit1 = zcit2;
    zcit2 = tmp;
  }
  else
  {
    cout << "Explore::getLimits: Leading minima are not neighbors\n";
    return false;
  }

  const float ratio = zcit1->value / zcit2->value;
  if (ratio < 0.5f || ratio > 2.0f)
  {
    cout << "Explore::getLimits: Leading minima are different heights\n";
    return false;
  }

  min1index = (zcit1->start + zcit1->end)/2 + offset;
  min2index = (zcit2->start + zcit2->end)/2 + offset;

cout << "ZC1 " << zcit1->start+offset << " to " << zcit1->end+offset << ", avg " <<
  (zcit1->start + zcit1->end)/2+offset << "\n";
cout << "  " << fixed << setprecision(2) << zcit1->startExact + offset <<
  ", " << zcit1->slopeStart << "\n";
cout << "  " << fixed << setprecision(2) << zcit1->endExact + offset <<
  ", " << zcit1->slopeEnd << "\n";

cout << "ZC2 " << zcit2->start+offset << " to " << zcit2->end+offset << ", avg " <<
  (zcit2->start + zcit2->end)/2+offset << "\n";
cout << "  " << fixed << setprecision(2) << zcit2->startExact + offset <<
  ", " << zcit2->slopeStart << "\n";
cout << "  " << fixed << setprecision(2) << zcit2->endExact + offset <<
  ", " << zcit2->slopeEnd << "\n";

  return true;
}


bool Explore::getLimits(
  const vector<float>& position, 
  const unsigned offset, 
  const unsigned start, 
  const unsigned end, 
  unsigned& lower,
  unsigned& upper,
  unsigned& spacing,
  unsigned& min1index,
  unsigned& min2index) const
{
  if (start < offset)
  {
    cout << "Explore::getLimits: Odd start\n";
    return false;
  }

  // ZCmaxima is a downward zero crossing, coming from a maximum.
  list<ZCinterval> ZCmaxima;
  list<ZCinterval> ZCminima;
  Explore::makeZCintervals(position, start-offset, end-offset, 0.f,
    ZCmaxima, ZCminima);

  if (ZCminima.size() < 2)
  {
    cout << "Explore::getLimits: Too few ZC minima\n";
    return false;
  }

  list<ZCinterval> ZCminimaSorted = ZCminima;
  ZCminimaSorted.sort([](const ZCinterval& zc1, const ZCinterval& zc2)
  {
    return (zc1.value < zc2.value);
  });

  auto zcit1 = ZCminimaSorted.begin();
  auto zcit2 = next(zcit1);

  if (zcit1->count+1 == zcit2->count)
  {
    // Order is 1, 2
  }
  else if (zcit2->count+1 == zcit1->count)
  {
    // Swap the order
    auto tmp = zcit1;
    zcit1 = zcit2;
    zcit2 = tmp;
  }
  else
  {
    cout << "Explore::getLimits: Leading minima are not neighbors\n";
    return false;
  }

  const float ratio = zcit1->value / zcit2->value;
  if (ratio < 0.5f || ratio > 2.0f)
  {
    cout << "Explore::getLimits: Leading minima are different heights\n";
    return false;
  }

  const unsigned i1 = zcit1->index;
  auto zcmaxit = ZCmaxima.begin();
  if (zcmaxit->index > i1)
  {
    cout << "Explore::getLimits: No leading maximum for first minimum\n";
    return false;
  }

  while (zcmaxit != ZCmaxima.end() && next(zcmaxit)->index < i1)
    zcmaxit++;

  if (zcmaxit == ZCmaxima.end())
  {
    cout << "Explore::getLimits: No leading maximum for first minimum\n";
    return false;
  }

  lower = zcmaxit->index + offset;

  const unsigned i2 = zcit2->index;
  while (zcmaxit != ZCmaxima.end() && next(zcmaxit)->index < i2)
    zcmaxit++;

  if (zcmaxit == ZCmaxima.end())
  {
    cout << "Explore::getLimits: No trailing maximum for first minimum\n";
    return false;
  }

  upper = next(zcmaxit)->index + offset;

  spacing = zcit2->index - zcit1->index;

  min1index = (zcit1->start + zcit1->end)/2 + offset;
  min2index = (zcit2->start + zcit2->end)/2 + offset;

cout << "ZC1 " << zcit1->start+offset << " to " << zcit1->end+offset << ", avg " <<
  (zcit1->start + zcit1->end)/2+offset << "\n";
cout << "ZC2 " << zcit2->start+offset << " to " << zcit2->end+offset << ", avg " <<
  (zcit2->start + zcit2->end)/2+offset << "\n";

  cout << "Returning " << lower << ", " << upper << ", " << spacing << "\n";
  return true;
}


#include "FFT.h"

void Explore::correlate(
  const vector<float>& accel,
  const vector<float>& position,
  const unsigned offset,
  PeaksInfo& peaksInfo,
  list<BogieTimes>& bogieTimes)
{
  // Manual deconvolution.
  vector<float> response;
  response.resize(2048, 0.f);
  for (unsigned i = 1500; i <= 2300; i++)
    response[i-1000] = accel[i];

  // Make the ends taper off more nicely.
  for (unsigned i = 400; i < 500; i++)
    response[i] = response[500] * (i-400.f) / 100.f;
  for (unsigned i = 1300; i < 1400; i++)
    response[i] = response[1300] * (1400.f-i) / 100.f;

  vector<float> zeroes;
  zeroes.resize(2048, 0.f);

  vector<float> imprespRe, imprespIm;
  imprespRe.resize(2048, 0.f);
  imprespIm.resize(2048, 0.f);

  FFT fftTemp;
  fftTemp.setSize(2048);

  vector<float> stimulus;
  stimulus.resize(2048, 0.f);
  stimulus[816] = 1.f;

  cout << "Deconvolution\n";
  for (unsigned stim = 953; stim < 954; stim++)
  {
  stimulus[stim-1] = 0.f;
  stimulus[stim] = 1.f;

  fftTemp.deconvolve(response, zeroes, stimulus, zeroes, 
    imprespRe, imprespIm);

  float power = 0.f;
  for (unsigned i = 0; i < 2048; i++)
    power += imprespRe[i] * imprespRe[i];

  cout << stim << " " << power << endl;
  }

  for (unsigned i = 0; i < 2048; i++)
  {
    cout << i << ";" << stimulus[i] << ";" << response[i] << ";" <<
      imprespRe[i] << ";" << imprespIm[i] << endl;
  }
  cout << endl;



  // Explore::setupGaussian(5.f);

/*
cout << "Gfilter\n";
for (unsigned i = 0; i < gaussian.size(); i++)
  cout << i << ";" << gaussian[i] << endl;
cout << "\n";
*/

  // Explore::filterGaussian(accel, filtered);
  // Explore::filter(accel, filtered);
  // If OK, then we don't need filtered.
  filtered = accel;
  // Explore::filter(accel, filtered);

  if (data.empty())
    return;

  // Just look at the last bogie for now.
  Datum& bogieLast = data.back();
  const unsigned len = bogieLast.qint->len;
  const unsigned s = bogieLast.qint->first;
  const unsigned e = s + len;

  // Integrate the bogie into a rough speed segment.
  // vector<float> speedAll;
  // speedAll.resize(position.size());
  // Explore::differentiate(position, 2000.f, speedAll);

  /*
  vector<float> speed;
  speed.resize(len);
  for (unsigned i = 0; i < len; i++)
    speed[i] = speedAll[i+s];
    */

  // Explore::integrate(filtered, s, e, 2000.f, speed);
  // Explore::differentiate(position, s, e, 2000.f, speed);

cout << "\n";
cout << "accel " << accel.size() << endl;
// for (unsigned i = 0; i < accel.size(); i++)
  // cout << i << ";" << accel[i] << endl;
// cout << endl;
// cout << "speedAll " << speedAll.size() << endl;
cout << "position " << position.size() << endl << endl;
for (unsigned i = 0; i < position.size(); i++)
  cout << i+offset << ";" << position[i] << endl;
cout << endl;

// cout << "speed " << speed.size() << endl;


  // New approach, 2019-12-08:
  // - Segment position into zero-crossings.
  // - Go from the tops before and after the bogie.
  unsigned lower, upper, spacing;
  unsigned min1index, min2index;
  if (Explore::getLimits(position, offset, s, e, lower, upper, spacing,
      min1index, min2index))
  {
    cout << "Got pos limits " << lower << " and " << upper << endl;
    cout << "Minima " << min1index << " and " << min2index << endl;
  }
  else
  {
    cout << "Bad pos limits\n";
    return;
  }

  const unsigned fftlen = Explore::powerOfTwo(filtered.size());
  if (fftlen == 0)
  {
    cout << "FFT limited to 32768 for now\n";
    return;
  }

  // Pad out the the acceleration trace.
  vector<float> accelPad;
  accelPad.resize(fftlen);
  for (unsigned i = 0; i < filtered.size(); i++)
    accelPad[i] = filtered[i];

cout << "Setting correlands" << endl;
  vector<float> bogieRev, wheel1Rev, wheel2Rev;
  Explore::setCorrelands(accelPad, fftlen, lower, upper,
    bogieRev, wheel1Rev, wheel2Rev);

  FFT fft;
  fft.setSize(fftlen);

  vector<float> bogieConv;
  // vector<float> wheel1Conv;
  // vector<float> wheel2Conv;
  bogieConv.resize(fftlen);
  // wheel1Conv.resize(fftlen);
  // wheel2Conv.resize(fftlen);

  fft.convolve(accelPad, bogieRev, bogieConv);
  // fft.convolve(accelPad, wheel1Rev, wheel1Conv);
  // fft.convolve(accelPad, wheel2Rev, wheel2Conv);

cout << "Filtering the bogie correlation" << endl;
  vector<float> bogieFilt;
  Explore::filter(bogieConv, bogieFilt);

/*
cout << "bogie conv\n";
for (unsigned i = 0; i < accel.size(); i++)
  cout << i << ";" << bogieConv[i] << ";" << bogieFilt[i] << "\n";
cout << endl;
*/

  AccelDetect accelDetect;
  cout << "Logging the filtered signal" << endl;
  accelDetect.log(bogieFilt, 0, accel.size());
  cout << "Extracting correlation peaks, " << bogieFilt[upper] << endl;
  accelDetect.extractCorr(bogieFilt[upper], "accel");
  cout << "Extracted correlation peaks" << endl;

  list<unsigned> bogieEnds;
  Explore::findCorrelationPeaks(accelDetect.getPeaks(), 
      bogieFilt[upper], spacing, bogieEnds);

  cout << "Bogie end positions\n";
  for (auto b: bogieEnds)
    cout << b << "\n";
  cout << endl;

  Explore::findSpeedBumps(position, offset, bogieEnds, upper-lower, bogieTimes);

  cout << "Speed bumps\n";
  unsigned prev = 0;
  for (auto b: bogieTimes)
  {
    cout << setw(4) << b.first << setw(6) << b.first-prev << "\n";
    prev = b.first;
    cout << setw(4) << b.second << setw(6) << b.second-prev << "\n";
    prev = b.second;
  }
  cout << endl;

  UNUSED(peaksInfo);
}


string Explore::strHeader() const
{
  stringstream ss;
  ss << setw(4) << "";
  for (unsigned j = 0; j < data.size(); j++)
    ss << right << setw(6) << j;
  return ss.str() + "\n";
}


string Explore::str() const
{
  stringstream ss;
  ss << Explore::strHeader();
  for (unsigned i = 0; i < data.size(); i++)
  {
    ss << left << setw(4) << i;
    for (unsigned j = 0; j < i; j++)
      ss << right << setw(6) << "*";
    for (unsigned j = i; j < data.size(); j++)
      ss << setw(6) << right << fixed << setprecision(2) << 
        data[i].correlates[j].value;
    ss << "\n";
  }
  ss << "\n";

  for (unsigned i = 0; i < data.size(); i++)
  {
    ss << left << setw(4) << i;
    for (unsigned j = 0; j < i; j++)
      ss << right << setw(6) << "*";
    for (unsigned j = i; j < data.size(); j++)
      ss << setw(6) << right << 
        data[i].correlates[j].shift;
    ss << "\n";
  }

  return ss.str() + "\n";
}

