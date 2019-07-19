#ifndef TRAIN_ENTITY_H
#define TRAIN_ENTITY_H

#include <vector>
#include <list>
#include <deque>
#include <string>

using namespace std;


enum CorrespondenceType
{
  CORRESPONDENCE_STRING = 0,
  CORRESPONDENCE_STRING_VECTOR = 1,
  CORRESPONDENCE_INT_VECTOR = 2,
  CORRESPONDENCE_INT = 3,
  CORRESPONDENCE_BOOL = 4,
  CORRESPONDENCE_SIZE = 5
};


struct CorrespondenceEntry
{
  string tag;
  CorrespondenceType corrType;
  unsigned no;
};


class Entity
{
  private:

    vector<string> strings;
    vector<vector<string>> stringVectors;
    vector<vector<int>> intVectors;
    vector<int> ints;
    deque<bool> bools;

    void init(const vector<unsigned>& fieldCounts);


  public:

    Entity();

    ~Entity();

    void clear();

    bool readFile(
      const string& fname,
      const list<CorrespondenceEntry>& fields,
      const vector<unsigned>& fieldCounts);

    int& operator [](const unsigned no); // Only the ints vector
    const int& operator [](const unsigned no) const;

    const string getString(const unsigned no);

    const vector<string> getStringVector(const unsigned no);

    int getInt(const unsigned no) const;

    bool getBool(const unsigned no) const;

    unsigned sizeString(const unsigned no) const;

    unsigned sizeInt(const unsigned no) const;

    void setString(
      const unsigned no,
      const string& s);

    void setStringVector(
      const unsigned no,
      const vector<string>& v);

    void setInt(
      const unsigned no,
      const int i);

    void setBool(
      const unsigned no,
      const bool b);
};

#endif
