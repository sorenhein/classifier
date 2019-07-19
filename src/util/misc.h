#ifndef TRAIN_MISC_H
#define TRAIN_MISC_H

#include <vector>
#include <string>

using namespace std;


void getFilenames(
  const string& dirName,
  vector<string>& textfiles,
  const string& terminateMatch = "");

void tokenize(
  const string& text,
  vector<string>& tokens,
  const string& delimiters);

unsigned countDelimiters(
  const string& text,
  const string& delimiters);

bool parseInt(
  const string& text,
  int& value,
  const string& err);

bool parseDouble(
  const string& text,
  double& value,
  const string& err);

bool parseBool(
  const string& text,
  bool& value,
  const string& err);

void parseCommaString(
  const string& text,
  vector<string>& fields);

bool parseCarSpecifier(
  const string& text,
  const string& err,
  string& offName,
  bool& reverseFlag,
  unsigned& count);

void toUpper(string& text);

bool readBinaryTrace(
  const string& filename,
  vector<float>& samples);

float ratioCappedUnsigned(
  const unsigned num,
  const unsigned denom,
  const float rMax);

float ratioCappedFloat(
  const float num,
  const float denom,
  const float rMax);

#endif
