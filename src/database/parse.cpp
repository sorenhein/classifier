#include "parse.h"


void tokenize(
  const string& text,
  vector<string>& tokens,
  const string& delimiters)
{
  // tokenize splits a string into tokens separated by delimiter.
  // http://stackoverflow.com/questions/236129/split-a-string-in-c

  string::size_type pos, lastPos = 0;

  while (true)
  {
    pos = text.find_first_of(delimiters, lastPos);
    if (pos == std::string::npos)
    {
      pos = text.length();
      tokens.push_back(string(text.data()+lastPos,
        static_cast<string::size_type>(pos - lastPos)));
      break;
    }
    else
    {
      tokens.push_back(string(text.data()+lastPos,
        static_cast<string::size_type>(pos - lastPos)));
    }
    lastPos = pos + 1;
  }
}


unsigned countDelimiters(
  const string& text,
  const string& delimiters)
{
  int c = 0;
  for (unsigned i = 0; i < delimiters.length(); i++)
    c += static_cast<int>
      (count(text.begin(), text.end(), delimiters.at(i)));
  return static_cast<unsigned>(c);
}


bool parseInt(
  const string& text,
  int& value)
{
  if (text == "")
    return false;

  int i;
  size_t pos;
  try
  {
    i = stoi(text, &pos);
    if (pos != text.size())
      return false;
  }
  catch (...)
  {
    return false;
  }

  value = i;
  return true;
}


bool parseDouble(
  const string& text,
  double& value)
{
  if (text == "")
    return false;

  double f;
  size_t pos;
  try
  {
    f = stod(text, &pos);
    if (pos != text.size())
      return false;
  }
  catch (...)
  {
    return false;
  }

  value = f;
  return true;
}


bool parseBool(
  const string& text,
  bool& value)
{
  if (text == "yes")
  {
    value = true;
    return true;
  }
  else if (text == "no")
  {
    value = false;
    return true;
  }
  else
    return false;
}


void parseCommaString(
  const string& text,
  vector<string>& fields)
{
  const size_t c = countDelimiters(text, ",");
  fields.resize(c+1);
  fields.clear();
  tokenize(text, fields, ",");
}


bool parseCarSpecifier(
  const string& text,
  string& offName,
  bool& reverseFlag,
  unsigned& count)
{
  count = 1;
  offName = text;
  reverseFlag = false;

  if (offName.back() == ')')
  {
    auto pos = offName.find("(");
    if (pos == string::npos)
      return false;

    string bracketed = offName.substr(pos+1);
    bracketed.pop_back();

    int countI;
    if (! parseInt(bracketed, countI))
      return false;

    count = static_cast<unsigned>(countI);
    offName = offName.substr(0, pos);
  }

  if (offName.back() == ']')
  {
    // Car is "backwards".
    auto pos = offName.find("[");
    if (pos == string::npos)
      return false;

    string bracketed = offName.substr(pos+1);
    if (bracketed != "R]")
      return false;

    offName = offName.substr(0, pos);
    reverseFlag =  true;
  }
  return true;
}

