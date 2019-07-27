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


string Motion::strLine(
  const string& text,
  const double act,
  const double est) const
{
  double dev;
  if (act == 0.)
    dev = 0.;
  else
  {
    const double f = (act - est) / act;
    dev = 100. * (f >= 0. ? f : -f);
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

  ss <<
    setw(16) << left << "Acceleration" <<
    setw(8) << right << fixed << setprecision(2) << estimate[2] << 
      " m/s^2 = " <<
    setw(8) << estimate[2] / G_FORCE << " g\n";

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

