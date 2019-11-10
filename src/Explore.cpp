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
  const unsigned start,
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
  for (unsigned i = start+lower; i < start+upper; i++)
    bogieRev[start+upper-1-i] = accel[i];

  const unsigned mid = (start+lower + start+upper) / 2;
  for (unsigned i = mid; i < start+upper; i++)
    wheel2Rev[start+upper-1-i] = accel[i];

  for (unsigned i = start+lower; i < mid; i++)
    wheel1Rev[start+upper-1-i] = accel[i];

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

    if (level0 > 0.2f * level1 &&
        level1 > 0.5f * ampl &&
        level2 > 0.2f * level1 &&
        level0 < level1 &&
        level2 < level1 &&
        diff10 >= spacingLo &&
        diff10 >= spacingLo &&
        diff21 <= spacingHi &&
        diff10 <= spacingHi)
    {
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
  const vector<float>& accel, 
  const list<unsigned>& bogieEnds, 
  const unsigned activeLen,
  const float sampleRate,
  list<BogieTimes>& speedBumps)
{
  // Add 20%
  const unsigned tol = static_cast<unsigned>(0.2f * activeLen);
  unsigned lowerExt, upperExt;

  for (unsigned bend: bogieEnds)
  {
    if (bend - activeLen < tol)
      lowerExt = 0;
    else
      lowerExt = bend - activeLen - tol;
    
    if (bend + tol >= accel.size())
      upperExt = accel.size();
    else
      upperExt = bend + tol;

cout << "Trying bogie end " << bend << ", range " <<
  lowerExt << " to " << upperExt << endl;
cout << "--------------------------\n\n";

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
  }
}

#include "FFT.h"

void Explore::correlate(
  const vector<float>& accel,
  PeaksInfo& peaksInfo,
  list<BogieTimes>& bogieTimes)
{
  Explore::filter(accel, filtered);

  if (bogieTimes.empty())
    return;

  // Just look at the last bogie for now.
  Datum& bogieLast = data.back();
  const unsigned len = bogieLast.qint->len;
  const unsigned s = bogieLast.qint->first;
  const unsigned e = s + len;

  // Integrate the bogie into a rough speed segment.
  vector<float> speed;
  speed.resize(len);
  Explore::integrate(filtered, s, e, 2000.f, speed);

/*
  cout << "Speed\n";
  for (unsigned i = 0; i < e-s; i++)
    cout << s+i << ";" << speed[i] << endl;
*/

  // Find the main peaks in this narrow speed segment.
  AccelDetect speedDetect;
  speedDetect.log(speed, s, len);
  speedDetect.extract("speed");

  unsigned lower, upper, spacing;
  if (speedDetect.getLimits(lower, upper, spacing))
    cout << "Got limits " << s+lower << " and " << s+upper << endl;
  else
  {
    cout << "Bad limits\n";
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
  Explore::setCorrelands(accelPad, fftlen, s, lower, upper,
    bogieRev, wheel1Rev, wheel2Rev);

  FFT fft;
  fft.setSize(fftlen);

  vector<float> bogieConv;
  vector<float> wheel1Conv;
  vector<float> wheel2Conv;
  bogieConv.resize(fftlen);
  wheel1Conv.resize(fftlen);
  wheel2Conv.resize(fftlen);

  fft.convolve(accelPad, bogieRev, bogieConv);
  fft.convolve(accelPad, wheel1Rev, wheel1Conv);
  fft.convolve(accelPad, wheel2Rev, wheel2Conv);

cout << "Filtering the bogie correlation" << endl;
  vector<float> bogieFilt;
  Explore::filter(bogieConv, bogieFilt);

  AccelDetect accelDetect;
cout << "Logging the filtered signal" << endl;
  accelDetect.log(bogieFilt, 0, accel.size());
cout << "Extracting correlation peaks, " << bogieFilt[s+upper] << endl;
  accelDetect.extractCorr(bogieFilt[s+upper], "accel");
cout << "Extracted correlation peaks" << endl;

  list<unsigned> bogieEnds;
  Explore::findCorrelationPeaks(accelDetect.getPeaks(), 
    bogieFilt[s+upper], spacing, bogieEnds);

cout << "Bogie end positions\n";
for (auto b: bogieEnds)
  cout << b << "\n";
cout << endl;

  Explore::findSpeedBumps(filtered, bogieEnds, upper-lower, 2000.f, bogieTimes);

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

