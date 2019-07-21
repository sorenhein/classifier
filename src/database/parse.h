#ifndef TRAIN_PARSE_H
#define TRAIN_PARSE_H

#include <vector>
#include <string>

using namespace std;


bool parseInt(
  const string& text,
  int& value);

bool parseDouble(
  const string& text,
  double& value);

bool parseBool(
  const string& text,
  bool& value);

void parseDelimitedString(
  const string& text,
  const string& delimiter,
  vector<string>& fields);

bool parseCarSpecifier(
  const string& text,
  string& offName,
  bool& reverseFlag,
  unsigned& count);

string parseBasename(const string& text);

#endif
