/*
 * This a generic list suitable for holding trains, cars etc.
 * It is only used in its derived forms.
 */

#ifndef TRAIN_COLLECTION_H
#define TRAIN_COLLECTION_H

#include "Entity.h"

using namespace std;

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


class Collection
{
  protected:

    list<CorrespondenceEntry> fields;

  private:

    vector<unsigned> fieldCounts;

    vector<Entity> entries;


    virtual void complete(Entity& entry){UNUSED(entry);};


  public:

    Collection();

    virtual ~Collection();

    void reset();

    virtual void configure(){};

    bool readFile(const string& fname);

    Entity const * getEntity(const unsigned no);

};

#endif
