#include <iostream>

#include "stats/CountStats.h"

extern CountStats overallStats;
extern CountStats exceptStats;

#include "Except.h"


Except::Except(
  const char fileArg[],
  const int lineArg,
  const char functionArg[],
  const Code codeArg,
  const string messageArg):
    file(string(fileArg)),
    line(lineArg),
    function(string(functionArg)),
    code(codeArg),
    message(messageArg)
{
  overallStats.log("except");

  if (! exceptStats.log(message))
    cout << "Unknown exception text" << endl;
}


void Except::print(ostream& flog) const
{
  flog << "Exception thrown in " << file << 
      ", function " << function << 
      ", line number " << line << endl;
  flog << "Code " << static_cast<unsigned>(code) << endl;
  flog << message << endl;
}


string Except::getMessage() const
{
  return message;
}


Code Except::getCode() const
{
  return code;
}

