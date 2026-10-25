#ifndef HISTORY_PROBE_H
#define HISTORY_PROBE_H
#include <layerkeysort.h>
typedef struct HistoryStorage { size_t header, rows, useful, capacity, domain; } HistoryStorage;
HistoryStorage history_storage(const LksSnapshot *snapshot);
#endif
