#include <iomanip>
#include <sstream>

#include "StatEntry.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


StatEntry::StatEntry()
{
}


StatEntry::~StatEntry()
{
}


void StatEntry::setTitle(const string& text)
{
  title = text;
}


void StatEntry::log(
  const vector<double>& actual,
  const vector<double>& estimate,
  const double residuals)
{
  UNUSED(actual);
  UNUSED(estimate);
  UNUSED(residuals);
}


void StatEntry::printHeader(ofstream& fout) const
{
  UNUSED(fout);
}


void StatEntry::print(ofstream& fout) const
{
  UNUSED(fout);
}

