/* Native-layout checks are research observations, never persisted ABI rules. */
#include "lks_snapshot_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"layout failure line %d\n",__LINE__); return 1; } } while (0)
_Static_assert(_Alignof(LksSnapshot) >= _Alignof(LksSnapshotRow),
    "snapshot tail must align row storage");
int main(void)
{
    static const size_t counts[] = {0,1,63,64,65,129};
    size_t k,i; int item=7;
    const unsigned char domain[] = {0,255,1};
    LksSnapshotOptions options = {domain,sizeof(domain),NULL,NULL};
    printf("pointer=%zu size_t=%zu record=%zu block=%zu snapshot=%zu row=%zu row_align=%zu\n",
        sizeof(void *),sizeof(size_t),sizeof(LksOrderHandle),sizeof(LksOrderBlock),
        sizeof(LksSnapshot),sizeof(LksSnapshotRow),(size_t)_Alignof(LksSnapshotRow));
    for(k=0;k<sizeof(counts)/sizeof(counts[0]);++k){
        LksOrder *o=lks_order_create();LksSnapshot *s=NULL,*loaded=NULL;
        const LksOrderHandle *h;unsigned char *wire;size_t n;
        CHECK(o);
        for(i=0;i<counts[k];++i)CHECK(lks_order_insert_back(o,&item,&h)==LKS_STATUS_OK);
        CHECK(lks_order_snapshot_capture(o,&options,&s)==LKS_STATUS_OK);
        CHECK(!counts[k] || (uintptr_t)s->rows % _Alignof(LksSnapshotRow)==0);
        n=lks_snapshot_serialized_size(s);wire=malloc(n);CHECK(wire);
        CHECK(lks_snapshot_serialize(s,wire,n)==LKS_STATUS_OK);
        CHECK(lks_snapshot_deserialize(wire,n,&loaded)==LKS_STATUS_OK);
        lks_order_destroy(o);free(wire);
        for(i=0;i<counts[k];++i){const void *data=(const void *)s;size_t size=1;
            CHECK(lks_snapshot_association(s,i,&data,&size)==LKS_STATUS_OK && !data && !size);
            CHECK(lks_snapshot_association(loaded,i,&data,&size)==LKS_STATUS_OK && !data && !size);
        }
        lks_snapshot_destroy(s);lks_snapshot_destroy(loaded);
    }
    puts("layout/alignment/empty association/source-wire lifetime COMPLETE");return 0;
}
