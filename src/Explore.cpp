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

  cout << "Correlands\n";
  for (unsigned i = 0; i < upper-lower; i++)
    cout << i << ";" << bogieRev[i] << ";" << wheel1Rev[i] << ";" <<
      wheel2Rev[i] << endl;
  cout << "\n";
}


#include "FFT.h"

void Explore::correlate(const vector<float>& accel)
{
  Explore::filter(accel, filtered);

  // Just look at the last bogie for now.
  Datum& bogieLast = data.back();
  const unsigned len = bogieLast.qint->len;
  const unsigned s = bogieLast.qint->first;
  const unsigned e = s + len;

  // Integrate the bogie into a rough speed segment.
  vector<float> speed;
  speed.resize(len);
  Explore::integrate(filtered, s, e, 2000.f, speed);

  cout << "Speed\n";
  for (unsigned i = 0; i < e-s; i++)
    cout << s+i << ";" << speed[i] << endl;

  // Find the main peaks in this narrow speed segment.
  AccelDetect speedDetect;
  speedDetect.log(speed, s, len);
  speedDetect.extract("speed");

  unsigned lower, upper;
  if (speedDetect.getLimits(lower, upper))
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

  /*
  cout << "Convolutions" << endl;
  for (unsigned i = 0; i < fftlen; i++)
    cout << i << ";" << bogieConv[i] << ";" << 
      wheel1Conv[i] << ";" << 
      wheel2Conv[i] << endl;
  cout << endl;
  */


/*
  for (unsigned i = 0; i < data.size(); i++)
  {
    data[i].correlates.resize(data.size());

    Explore::correlateFixed(* data[i].qint, * data[i].qint, 0,
      data[i].correlates[i]);

    for (unsigned j = i+1; j < data.size(); j++)
    {
cout << "MAIN Correlating " << i << ", (" <<
  data[i].qint->first << ", " << data[i].qint->first + data[i].qint->len <<
  ") (" <<
  data[i].peaksOrig.size() << ") and " << j << ", (" << 
  data[j].qint->first << ", " << data[j].qint->first + data[j].qint->len <<
  ") (" <<
  "(" << data[j].peaksOrig.size() << ")" << endl;
      const unsigned guess = Explore::guessShift(
        data[i].peaksOrig, data[j].peaksOrig);
cout << "guess " << guess << endl;
if (guess > 100)
{
  cout << "ERROR\n";
  data[i].correlates[j].value = 0.;
}
else
      Explore::correlateBest(* data[i].qint, * data[j].qint, guess, 
        data[i].correlates[i].value,
        data[i].correlates[j]);
    } 
  }
*/

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

