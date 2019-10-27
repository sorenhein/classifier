// https://www.nayuki.io/page/free-small-fft-in-multiple-languages

/* 
 * Free FFT and convolution (C++)
 * 
 * Copyright (c) 2019 Project Nayuki. (MIT License)
 * https://www.nayuki.io/page/free-small-fft-in-multiple-languages
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the 
 * "Software"), to deal in the Software without restriction, including 
 * without limitation the rights to use, copy, modify, merge, publish, 
 * distribute, sublicense, and/or sell copies of the Software, and to 
 * permit persons to whom the Software is furnished to do so, subject to 
 * the following conditions:
 * - The above copyright notice and this permission notice shall be 
 *   included in all copies or substantial portions of the Software.
 * - The Software is provided "as is", without warranty of any kind, 
 *   express or implied, including but not limited to the warranties of 
 *   merchantability, fitness for a particular purpose and noninfringement.
 *   In no event shall the authors or copyright holders be liable for any 
 *   claim, damages or other liability, whether in an action of contract, 
 *   tort or otherwise, arising from, out of or in connection with the 
 *   Software or the use or other dealings in the Software.
 */

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "FFT.h"

#ifndef M_PI
  #define M_PI 3.14159265358979323846
#endif


void FFT::setSize(const size_t n)
{
  len = n;

  // Compute levels = floor(log2(n))
  levels = 0;  
  for (size_t temp = n; temp > 1U; temp >>= 1)
    levels++;

  if (static_cast<size_t>(1U) << levels != n)
    throw domain_error("Length is not a power of 2");

  // Trigonometric tables
  cosTable.resize(len/2);
  sinTable.resize(len/2);
  for (size_t i = 0; i < len / 2; i++) 
  {
    cosTable[i] = static_cast<float>(cos(2 * M_PI * i / n));
    sinTable[i] = static_cast<float>(sin(2 * M_PI * i / n));
  }
}


size_t FFT::reverseBits(
  size_t x, 
  int n) 
{
  size_t result = 0;
  for (int i = 0; i < n; i++, x >>= 1)
    result = (result << 1) | (x & 1U);
  return result;
}


void FFT::transform(
  vector<float>& real, 
  vector<float>& imag) 
{
  if (len != real.size() || len != imag.size())
    throw invalid_argument("Mismatched lengths");
  if (len == 0)
    return;
  else if ((len & (len - 1)) == 0)  // Is power of 2
    FFT::transformRadix2(real, imag);
  else
    throw invalid_argument("Not power of 2");
}


void FFT::inverseTransform(
  vector<float>& real, 
  vector<float>& imag) 
{
  FFT::transform(imag, real);
}


void FFT::transformRadix2(
  vector<float>& real, 
  vector<float>& imag) 
{
  // Bit-reversed addressing permutation
  for (size_t i = 0; i < len; i++) 
  {
    size_t j = reverseBits(i, levels);
    if (j > i) 
    {
      swap(real[i], real[j]);
      swap(imag[i], imag[j]);
    }
  }
	
  // Cooley-Tukey decimation-in-time radix-2 FFT
  for (size_t size = 2; size <= len; size *= 2) 
  {
    size_t halfsize = size / 2;
    size_t tablestep = len / size;
    for (size_t i = 0; i < len; i += size) 
    {
      for (size_t j = i, k = 0; j < i + halfsize; j++, k += tablestep) 
      {
        size_t l = j + halfsize;
        const float tpre =  real[l] * cosTable[k] + imag[l] * sinTable[k];
        const float tpim = -real[l] * sinTable[k] + imag[l] * cosTable[k];
        real[l] = real[j] - tpre;
        imag[l] = imag[j] - tpim;
        real[j] += tpre;
        imag[j] += tpim;
      }
    }
    if (size == len)  // Prevent overflow in 'size *= 2'
      break;
  }
}


void FFT::convolve(
  const vector<float>& x, 
  const vector<float>& y, 
  vector<float>& out) 
{
  size_t n = x.size();
  if (n != y.size() || n != out.size())
    throw std::invalid_argument("Mismatched lengths");
  vector<float> outimag(n);
  convolve(x, vector<float>(n), y, vector<float>(n), out, outimag);
}


void FFT::convolve(
  const vector<float>& xreal, 
  const vector<float>& ximag,
  const vector<float>& yreal, 
  const vector<float>& yimag,
  vector<float>& outreal, 
  vector<float>& outimag) 
{
  size_t n = xreal.size();
  if (n != ximag.size() || 
    n != yreal.size() || 
    n != yimag.size() || 
    n != outreal.size() || 
    n != outimag.size())
      throw std::invalid_argument("Mismatched lengths");
	
  vector<float> xr = xreal;
  vector<float> xi = ximag;
  vector<float> yr = yreal;
  vector<float> yi = yimag;
  transform(xr, xi);
  transform(yr, yi);
	
  for (size_t i = 0; i < n; i++) 
  {
    float temp = xr[i] * yr[i] - xi[i] * yi[i];
    xi[i] = xi[i] * yr[i] + xr[i] * yi[i];
    xr[i] = temp;
  }
  inverseTransform(xr, xi);
	
  for (size_t i = 0; i < n; i++) 
  {  
    // Scaling (because this FFT implementation omits it)
    outreal[i] = xr[i] / n;
    outimag[i] = xi[i] / n;
  }
}

