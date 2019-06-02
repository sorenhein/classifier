#include <iomanip>
#include <random>
#include <ctime>
#include <algorithm>

#include "Disturb.h"
#include "read.h"


Disturb::Disturb()
{
  noiseSdev = 0;
  detectedNoiseSdev = 0;

  Disturb::resetDisturbType(injection);
  Disturb::resetDisturbType(deletion);
  Disturb::resetDisturbType(prunedFront);
  Disturb::resetDisturbType(prunedBack);

  Disturb::resetDisturbType(detInjection);
  Disturb::resetDisturbType(detDeletion);
  Disturb::resetDisturbType(detPrunedFront);
  Disturb::resetDisturbType(detPrunedBack);

  srand(static_cast<unsigned>(time(NULL)));
}


void Disturb::resetDisturbType(DisturbType& d)
{
  d.peaksMin = 0;
  d.peaksMax = 0;
  d.peaks.clear();
}


Disturb::~Disturb()
{
}


bool Disturb::readFile(const string& fname)
{
  ifstream fin;
  fin.open(fname);
  string line;
  while (getline(fin, line))
  {
    line.erase(remove(line.begin(), line.end(), '\r'), line.end());
    if (line == "" || line.front() == '#')
      continue;

    const string err = "File " + fname + ": Bad line '" + line + "'";

    auto sp = line.find(" ");
    if (sp == string::npos || sp == 0 || sp == line.size()-1)
    {
      cout << err << endl;
      fin.close();
      return false;
    }

    const string& field = line.substr(0, sp);
    const string& rest = line.substr(sp+1);

    if (field == "NOISE_SDEV")
    {
      if (! readInt(rest, noiseSdev, err)) break;
    }
    else if (field == "INJECT_AT_LEAST")
    {
      if (! readInt(rest, injection.peaksMin, err)) break;
    }
    else if (field == "INJECT_UP_TO")
    {
      if (! readInt(rest, injection.peaksMax, err)) break;
    }
    else if (field == "INJECT")
    {
      if (! readInt(rest, injection.peaksMin, err)) break;
      injection.peaksMax = injection.peaksMin;
    }
    else if (field == "DELETE_AT_LEAST")
    {
      if (! readInt(rest, deletion.peaksMin, err)) break;
    }
    else if (field == "DELETE_UP_TO")
    {
      if (! readInt(rest, deletion.peaksMax, err)) break;
    }
    else if (field == "DELETE")
    {
      if (! readInt(rest, deletion.peaksMin, err)) break;
      deletion.peaksMax = deletion.peaksMin;
    }
    else if (field == "PRUNE_FRONT_AT_LEAST")
    {
      if (! readInt(rest, prunedFront.peaksMin, err)) break;
    }
    else if (field == "PRUNE_FRONT_UP_TO")
    {
      if (! readInt(rest, prunedFront.peaksMax, err)) break;
    }
    else if (field == "PRUNE_FRONT")
    {
      if (! readInt(rest, prunedFront.peaksMin, err)) break;
      prunedFront.peaksMax = prunedFront.peaksMin;
    }
    else if (field == "PRUNE_BACK_AT_LEAST")
    {
      if (! readInt(rest, prunedBack.peaksMin, err)) break;
    }
    else if (field == "PRUNE_BACK_UP_TO")
    {
      if (! readInt(rest, prunedBack.peaksMax, err)) break;
    }
    else if (field == "PRUNE_BACK")
    {
      if (! readInt(rest, prunedBack.peaksMin, err)) break;
      prunedBack.peaksMax = prunedBack.peaksMin;
    }
    else
    {
      cout << err << endl;
    }
  }
  return true;
}


int Disturb::getNoiseSdev() const
{
  return noiseSdev;
}


void Disturb::getInjectRange(
  int& injectMin,
  int& injectMax) const
{
  injectMin = injection.peaksMin;
  injectMax = injection.peaksMax;
}


void Disturb::getDeleteRange(
  int& deleteMin,
  int& deleteMax) const
{
  deleteMin = deletion.peaksMin;
  deleteMax = deletion.peaksMax;
}


void Disturb::getFrontRange(
  int& frontMin,
  int& frontMax) const
{
  frontMin = prunedFront.peaksMin;
  frontMax = prunedFront.peaksMax;
}


void Disturb::getBackRange(
  int& backMin,
  int& backMax) const
{
  backMin = prunedBack.peaksMin;
  backMax = prunedBack.peaksMax;
}


void Disturb::setInject(const unsigned pos)
{
  injection.peaks.insert(pos);
}


void Disturb::setDelete(const unsigned pos)
{
  deletion.peaks.insert(pos);
}


void Disturb::setPruneFront(const unsigned number)
{
  prunedFront.peaks.insert(number);
}


void Disturb::setPruneBack(const unsigned number)
{
  prunedBack.peaks.insert(number);
}


void Disturb::detectInject(const unsigned pos)
{
  detInjection.peaks.insert(pos);
  detInjection.peaksMin++;
  detInjection.peaksMax++;
}


void Disturb::detectDelete(const unsigned pos)
{
  detDeletion.peaks.insert(pos);
  detDeletion.peaksMin++;
  detDeletion.peaksMax++;
}


void Disturb::detectFront(const unsigned number)
{
  detPrunedFront.peaks.insert(number);
  detPrunedFront.peaksMin++;
  detPrunedFront.peaksMax++;
}


void Disturb::detectBack(const unsigned number)
{
  detPrunedBack.peaks.insert(number);
  detPrunedBack.peaksMin++;
  detPrunedBack.peaksMax++;
}


void Disturb::printLine(
  ofstream& fout,
  const string& text,
  const DisturbType& actual,
  const DisturbType& detected) const
{
  string s1, s2, s3;

  if (actual.peaksMin == actual.peaksMax)
    s1 = to_string(actual.peaksMin);
  else
    s1 = to_string(actual.peaksMin) + "-" +
         to_string(actual.peaksMax);

  if (detected.peaksMin == detected.peaksMax)
    s2 = to_string(detected.peaksMin);
  else
    s2 = to_string(detected.peaksMin) + "-" +
         to_string(detected.peaksMax);

  if (detected.peaksMin >= actual.peaksMin &&
      detected.peaksMax <= actual.peaksMax)
    s3 = "OK";
  else
    s3 = "DIFF";

  set<unsigned> setActual = actual.peaks;
  set<unsigned> setDetected = detected.peaks;

  string s4a, s4b, s4c;
  for (auto& a: setActual)
  {
    if (setDetected.erase(a))
    {
      setActual.erase(a);
       s4a += " " + to_string(a);
    }
  }

  for (auto& a: setActual)
    s4b += " " + to_string(a);

  for (auto& a: setDetected)
    s4c += " " + to_string(a);

  string s4;
  if (s4a != "")
    s4 += "AGREE" + s4a + " ";
  if (s4b != "")
    s4 += "MISSED" + s4b + " ";
  if (s4c != "")
    s4 += "EXCESS" + s4c + " ";

  fout << setw(12) << left << text <<
    setw(8) << right << s1 <<
    setw(8) << right << s2 <<
    setw(8) << right << s3 << "  " << 
    setw(8) << left << s4 << endl;
}


void Disturb::print(const string& fname) const
{
  ofstream fout;
  fout.open(fname);

  fout << setw(12) << "" <<
    setw(8) << right << "Set" <<
    setw(8) << right << "Detect" <<
    setw(8) << right << "Same?" << "  " <<
    left << "Peaks" << endl;

  fout << setw(12) << "Noise (ms)" << 
    setw(8) << right << noiseSdev <<
    setw(8) << right << detectedNoiseSdev << 
    setw(8) << right << "-" << "  " << 
    setw(8) << left << "-" << endl;

  Disturb::printLine(fout, "Inject", injection, detInjection);
  Disturb::printLine(fout, "Detect", deletion, detDeletion);
  Disturb::printLine(fout, "pruneF", prunedFront, detPrunedFront);
  Disturb::printLine(fout, "pruneB", prunedBack, detPrunedBack);
  fout << endl;

  fout.close();
}

