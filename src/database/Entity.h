#ifndef TRAIN_ENTITY_H
#define TRAIN_ENTITY_H

#include <vector>
#include <list>
#include <map>
#include <deque>
#include <string>

using namespace std;


enum CorrespondenceType
{
  CORRESPONDENCE_STRING = 0,
  CORRESPONDENCE_STRING_VECTOR = 1,
  CORRESPONDENCE_STRING_MAP = 2,
  CORRESPONDENCE_INT_VECTOR = 3,
  CORRESPONDENCE_INT = 4,
  CORRESPONDENCE_BOOL = 5,
  CORRESPONDENCE_DOUBLE = 6,
  CORRESPONDENCE_SIZE = 7
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
    map<string, vector<string>> stringMap;
    vector<vector<int>> intVectors;
    vector<int> ints;
    deque<bool> bools;
    vector<double> doubles;

    void init(const vector<unsigned>& fieldCounts);

    bool parseField(
      const list<CorrespondenceEntry>& fields,
      const string& tag,
      const string& value);


  public:

    Entity();

    ~Entity();

    void clear();

    // Tag-type file.
    bool readTagFile(
      const string& fname,
      const list<CorrespondenceEntry>& fields,
      const vector<unsigned>& fieldCounts);

    // One header followed by ints.
    bool readSeriesFile(
      const string& fname,
      const list<CorrespondenceEntry>& fields,
      const vector<unsigned>& fieldCounts,
      const unsigned no);

    // Regular lines of comma-separated data.
    bool readCommaFile(
      const string& fname,
      const vector<unsigned>& fieldCounts,
      const unsigned count);

    bool readCommaLine(
      ifstream& fin,
      bool& errFlag,
      const unsigned count);

    int& operator [](const unsigned no); // Only the ints vector
    const int& operator [](const unsigned no) const;

    string& getString(const unsigned no);
    const string& getString(const unsigned no) const;

    vector<string>& getStringVector(const unsigned no);
    const vector<string>& getStringVector(const unsigned no) const;

    const string& getMap(
      const string& str,
      const unsigned no) const;

    vector<int>& getIntVector(const unsigned no);
    const vector<int>& getIntVector(const unsigned no) const;

    int getInt(const unsigned no) const;

    bool getBool(const unsigned no) const;

    double& getDouble(const unsigned no);
    const double& getDouble(const unsigned no) const;

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

    void setDouble(
      const unsigned no,
      const double d);

    void reverseIntVector(const unsigned no);
};

#endif
