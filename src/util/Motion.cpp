#include <iomanip>
#include <sstream>

#include "Motion.h"

#include "../const.h"


Motion::Motion()
{
  order = 2;
  Motion::reset();
}


Motion::~Motion()
{
}


void Motion::reset()
{
  actual.resize(order+1);
  estimate.resize(order+1);
}


float Motion::time2pos(const float time) const
{
  float res = 0.;
  float pow = 1.;
  for (unsigned c = 0; c < estimate.size(); c++)
  {
    res += estimate[c] * pow;
    pow *= time;
  }
  return res;
}


bool Motion::pos2time(
  const float pos,
  const float sampleRate,
  unsigned& n) const
{
  const float s0 = estimate[0];
  const float v = estimate[1];
  const float c = estimate[2]; // Equals 0.5 * accel, so a = 2c

  if (s0 > pos)
    return false;

  const float t0 = (pos - s0) / v;

  if (abs(c) < 0.0001f)
  {
    // s = s0 + v * t
    // Numerically probably better to ignore the acceleration.
    n = static_cast<unsigned>(sampleRate * t0);
  }
  else
  {
    // s = s0 + v * t + c * t^2
    n = static_cast<unsigned>(sampleRate *
       (v / (2.f * c)) * (sqrt(1.f + 4.f * c * t0/ v) - 1.f));
  }
  return true;
}


string Motion::strLine(
  const string& text,
  const float act,
  const float est) const
{
  float dev;
  if (act == 0.)
    dev = 0.;
  else
  {
    const float f = (act - est) / act;
    dev = 100.f * (f >= 0. ? f : -f);
  }

  stringstream ss;
  ss << setw(8) << left << text <<
    setw(10) << right << fixed << setprecision(2) << act <<
    setw(10) << right << fixed << setprecision(2) << est <<
    setw(9) << right << fixed << setprecision(1) << dev << "%" << endl;
  return ss.str();
}


string Motion::strEstimate(const string& title) const
{
  stringstream ss;
  ss << title << "\n";
  ss <<
    setw(16) << left << "Speed" <<
    setw(8) << right << fixed << setprecision(2) << estimate[1] << 
      " m/s   = " <<
    setw(8) << MS_TO_KMH * estimate[1] << " km/h\n";

  // The regression coefficients are of the form:
  //   position = c2 * time^2 + c1 * time + c0.
  //
  // In the physics formula, we have
  //   position = 0.5 * a * time^2 + v * time + pos0.
  // 
  // As we store the regression coefficients here, a = 2 * c2.

  ss <<
    setw(16) << left << "Acceleration" <<
    setw(8) << right << fixed << setprecision(2) << 2.f * estimate[2] << 
      " m/s^2 = " <<
    setw(8) << 2.f * estimate[2] / G_FORCE << " g\n";

  return ss.str() + "\n";
}


string Motion::strBothHeader() const
{
  stringstream ss;
  ss << setw(8) << left << "" <<
    setw(10) << right << "Actual" <<
    setw(10) << right << "Est" <<
    setw(10) << right << "Error" << endl;
  return ss.str();
}


string Motion::strBoth() const
{
  stringstream ss;
  ss << Motion::strBothHeader();
  ss << Motion::strLine("Offset", actual[0], estimate[0]);
  ss << Motion::strLine("Speed", actual[1], estimate[1]);
  ss << Motion::strLine("Accel", actual[2], estimate[2]);
  return ss.str();
}

