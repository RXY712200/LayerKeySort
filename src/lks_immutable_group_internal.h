#ifndef LKS_IMMUTABLE_GROUP_INTERNAL_H
#define LKS_IMMUTABLE_GROUP_INTERNAL_H
#include "layerkeysort.h"
typedef struct LksSourceMarker LksSourceMarker;
struct LksGroup {
    void **items;
    size_t count;
    LksSourceMarker *marker;
    int busy;
};
struct LksGroupBatch {
    LksGroup **groups;
    size_t count, group_count, group_size;
    int busy;
};
#endif
