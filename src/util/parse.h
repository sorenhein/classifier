#ifndef TRAIN_PARSE_H
#define TRAIN_PARSE_H

#include <vector>
#include <string>

using namespace std;


void tokenize(
  const string& text,
  vector<string>& tokens,
  const string& delimiters);

unsigned countDelimiters(
  const string& text,
  const string& delimiters);

bool parseInt(
  const string& text,
  int& value);

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

#endif
