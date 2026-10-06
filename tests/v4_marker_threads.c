/* Normal production allocator: diagnostic counters are intentionally not used
 * concurrently. Each worker owns distinct snapshots; common read lifetime is held. */
#include "layerkeysort.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif
#define THREADS 8
#define EACH 8
typedef struct Worker { LksSnapshot *owned[EACH]; const LksSnapshot *shared; int failed; } Worker;
static void thread_work(Worker *w)
{
    size_t i; char key[64];
    for(i=0;i<10000;++i) {
        const void *data; size_t size;
        if(lks_snapshot_key_format(w->shared,i%128,key,sizeof(key))!=LKS_STATUS_OK ||
            lks_snapshot_key_validate(key)!=LKS_STATUS_OK ||
            lks_snapshot_namespace(w->shared,&data,&size)!=LKS_STATUS_OK || size!=2) w->failed=1;
    }
    for(i=0;i<EACH;++i) lks_snapshot_destroy(w->owned[i]);
}
#ifdef _WIN32
static DWORD WINAPI entry(LPVOID w) { thread_work((Worker *)w);return 0; }
#else
static void *entry(void *w) { thread_work((Worker *)w);return NULL; }
#endif
static int equal_compare(const void *a,const void *b,void *ctx)
{ (void)a;(void)b;(void)ctx;return 0; }
static int run(int group_source)
{
    size_t i,j,started=0; int item=1,result=0; void *items[128]; Worker workers[THREADS];
    LksImmutableGroup *g=NULL; LksSnapshot *shared=NULL; unsigned char ns[]={0,255};
    LksSnapshotOptions opt={ns,2,NULL,NULL};
    LksOrder *order=lks_order_create(); const LksOrderHandle *h;
#ifdef _WIN32
    HANDLE threads[THREADS];
#else
    pthread_t threads[THREADS];
#endif
    LksComparator cmp = {equal_compare, NULL};
    for(i=0;i<128;++i) items[i]=&item;
    if(!order) return 1;
    for(i=0;i<128;++i) if(lks_order_insert_back(order,items[i],&h)!=LKS_STATUS_OK) return 2;
    memset(workers,0,sizeof(workers));
    if(group_source && lks_immutable_group_build(items,128,&cmp,&g)!=LKS_STATUS_OK) return 3;
    if((group_source ? lks_immutable_group_snapshot_capture(g,&opt,&shared) :
        lks_order_snapshot_capture(order,&opt,&shared))!=LKS_STATUS_OK) return 3;
    for(i=0;i<THREADS;++i) {
        workers[i].shared=shared;
        for(j=0;j<EACH;++j) if((group_source ? lks_immutable_group_snapshot_capture(g,&opt,&workers[i].owned[j]) : lks_order_snapshot_capture(order,&opt,&workers[i].owned[j]))!=LKS_STATUS_OK) return 4;
    }
    lks_order_destroy(order); lks_immutable_group_destroy(g);
    for(i=0;i<THREADS;++i) {
#ifdef _WIN32
        threads[i]=CreateThread(NULL,0,entry,&workers[i],0,NULL); if(!threads[i]) { result=5;break; }
#else
        if(pthread_create(&threads[i],NULL,entry,&workers[i])) { result=5;break; }
#endif
        ++started;
    }
    for(i=0;i<started;++i) {
#ifdef _WIN32
        if(WaitForSingleObject(threads[i],INFINITE)!=WAIT_OBJECT_0) return 6;
        CloseHandle(threads[i]);
#else
        if(pthread_join(threads[i],NULL)) return 6;
#endif
        if(workers[i].failed)result=7;
    }
    for(i=started;i<THREADS;++i)for(j=0;j<EACH;++j)lks_snapshot_destroy(workers[i].owned[j]);
    lks_snapshot_destroy(shared); if(result)return result;puts("Concurrent marker releases and protected immutable reads PASS"); return 0;
}

int main(void) { int status=run(0);return status ? status : run(1); }
