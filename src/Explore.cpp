#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "Explore.h"
#include "Peak.h"
#include "PeakPtrs.h"

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

    while (pi != pe && (* pi)->getIndex() < act.first)
     pi++;
     if (pi == pe)
       continue;

    while (pi != pe && (* pi)->getIndex() <= act.first + act.len)
    {
      datum.peaksOrig.push_back(* pi);
      pi++;
    }
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


void Explore::filter(const vector<float>& accel)
{
  // For now this is a simple moving average.
  filtered.resize(accel.size());

  // Front and back.
  const unsigned al = accel.size();
  for (unsigned i = 0; i <= WINDOW_LOBE; i++)
  {
    filtered[i] = Explore::filterEdge(accel, i, i);
    filtered[al-i-1] = Explore::filterEdge(accel, al-i-1, i);
  }

  float cum = 0;
  for (unsigned i = 0; i < 2*WINDOW_LOBE + 1; i++)
    cum += accel[i];
  const float factor = 1.f / (2.f*WINDOW_LOBE + 1.f);

  for (unsigned i = WINDOW_LOBE+1; i < al-WINDOW_LOBE-1; i++)
  {
    cum += accel[i+WINDOW_LOBE] - accel[i-WINDOW_LOBE-1];
    filtered[i] = factor * cum;
  }
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


#include "FFT.h"

void Explore::correlate(const vector<float>& accel)
{
UNUSED(accel);
/*
  Explore::filter(accel);

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

  // Just to try the FFT.
/*
  vector<float> accelPad;
  accelPad.resize(16384);
  const unsigned m = (filtered.size() > 16384 ? 16384 : filtered.size());
  for (unsigned i = 0; i < m; i++)
    accelPad[i] = filtered[i];

  Datum& bogieLast = data.back();
  const unsigned s = bogieLast.qint->first;
  const unsigned e = s + bogieLast.qint->len;

  vector<float> reverseBogie;
  reverseBogie.resize(16384);
  for (unsigned i = 0; i < bogieLast.qint->len; i++)
    reverseBogie[i] = filtered[e-i-1];

  FFT fft;
  fft.setSize(16384);
  vector<float> out;
  out.resize(16384);
  fft.convolve(accelPad, reverseBogie, out);

  cout << "Convolution" << endl;
  for (unsigned i = 0; i < 16384; i++)
    cout << i << ";" << out[i] << endl;
  cout << endl;
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

