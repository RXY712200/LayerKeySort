#include "history_probe.h"
#ifdef RESEARCH_PRIVATE
#include "lks_snapshot_internal.h"
HistoryStorage history_storage(const LksSnapshot *s)
{
    HistoryStorage r={0};
    if(s){r.header=sizeof(*s);r.rows=s->count*sizeof(LksSnapshotRow);
        r.useful=s->association_size;r.capacity=s->association_capacity;r.domain=s->namespace_size;}
    return r;
}
#else
HistoryStorage history_storage(const LksSnapshot *s)
{HistoryStorage r={0};(void)s;return r;}
#endif
