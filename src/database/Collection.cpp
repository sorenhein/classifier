#include "Collection.h"


Collection::Collection()
{
  Collection::reset();
}


Collection::~Collection()
{
}


void Collection::reset()
{
  fields.clear();
  fieldCounts.clear();
  entries.clear();
}


bool Collection::readFile(const string& fname)
{
  Entity entry;
  if (! entry.readFile(fname, fields, fieldCounts))
    return false;

  Collection::complete(entry);
  return true;
}


Entity const * Collection::getEntity(const unsigned no)
{
  if (no < entries.size())
    return &entries[no];
  else
    return nullptr;
}

