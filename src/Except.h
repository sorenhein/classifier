#ifndef TRAIN_EXCEPT_H
#define TRAIN_EXCEPT_H

#include <string>
#include "errors.h"

using namespace std;


#define THROW(code, msg) throw Except(__FILE__, __LINE__, __FUNCTION__, code, msg)


class Except: public exception
{
  private:

    string file;
    int line;
    string function;
    Code code;
    string message;


  public:

    Except(
      const char fileArg[],
      const int lineArg,
      const char functionArg[],
      const Code codeArg,
      const string messageArg);

    void print(ostream& flog) const;

    string getMessage() const;

    Code getCode() const;

};

#endif

