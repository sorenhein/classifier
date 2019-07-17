#include "Collection.h"


#include <iostream>

Collection::Collection()
{
}


bool Collection::readFile(const string& fname)
{
  Entity entry;
  if (! entry.readFile(fname, fields, fieldCounts))
    return false;

  Collection::complete(entry);
  entries.push_back(entry);
  return true;
}


Entity const * Collection::getEntity(const unsigned no)
{
  if (no < entries.size())
    return &entries[no];
  else
    return nullptr;
}

