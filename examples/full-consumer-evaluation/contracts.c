/* Consumer-facing integration cases, not source-inclusion/private-state tests. */
#include <layerkeysort.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"contract line %d: %s\n",__LINE__,#x); return 1; } } while (0)
typedef struct Item { int key; int identity; } Item;
typedef struct Context { LksManagedOrder *managed; size_t calls; int violations; } Context;
static int compare(const void *a, const void *b, void *context)
{
    const Item *x = a, *y = b; Context *c = context;
    if (c) {
        ++c->calls;
        if (c->managed) {
            const LksOrderHandle *p = NULL, *e = NULL, *s = NULL;
            if (lks_managed_order_locate(c->managed, a, &p, &e, &s) != LKS_STATUS_REENTRANT ||
                lks_managed_order_size(c->managed) != 0) c->violations++;
        }
    }
    return (x->key > y->key) - (x->key < y->key);
}
static LksStatus assoc(void *item, uint64_t row, void *ctx, const void **p, size_t *n)
{
    (void)row; (void)ctx; *p = &((Item *)item)->identity; *n = sizeof(int); return LKS_STATUS_OK;
}
static LksStatus missing(const void *p, size_t n, uint64_t row, void *ctx, void **out)
{ (void)p; (void)n; (void)row; (void)ctx; *out = NULL; return LKS_STATUS_NOT_FOUND; }
static LksStatus reject_capture(void *item,uint64_t row,void *ctx,const void **p,size_t *n)
{
    LksOrder *order=ctx;const LksOrderHandle *output=NULL;
    (void)row;(void)p;(void)n;
    if(lks_order_insert_back(order,item,&output)!=LKS_STATUS_REENTRANT||output||lks_order_size(order)!=0)
        return LKS_STATUS_INTERNAL_ERROR;
    return LKS_STATUS_NOT_FOUND;
}
static int persistence(void)
{
    Item items[3] = {{1,11},{2,22},{3,33}}; LksOrder *o = lks_order_create(), *restored = NULL;
    const LksOrderHandle *h[3], *got = NULL; LksOrderCursor *cursor = NULL;
    LksSnapshot *s = NULL, *loaded = NULL; LksSnapshotOptions opt = {"eval",4,assoc,NULL};
    unsigned char *wire, *mutated; size_t bytes, i; char key[64], other[64]; int c = 9;
    CHECK(o);
    for (i=0;i<3;i++) CHECK(lks_order_insert_back(o,&items[i],&h[i])==LKS_STATUS_OK);
    CHECK(lks_order_cursor_create(o,0,&cursor)==LKS_STATUS_OK);
    CHECK(lks_order_snapshot_capture(o,&opt,&s)==LKS_STATUS_OK);
    CHECK(lks_order_snapshot_is_current(o,s));
    {LksSnapshot *failed=NULL;LksSnapshotOptions rejection={"reject",6,reject_capture,o};
     CHECK(lks_order_snapshot_capture(o,&rejection,&failed)==LKS_STATUS_NOT_FOUND&&!failed);
     CHECK(lks_order_snapshot_is_current(o,s)&&lks_order_size(o)==3);}
    CHECK(lks_order_move_before(o,h[1],h[2])==LKS_STATUS_OK); /* Adjacent no-op. */
    CHECK(lks_order_cursor_next(cursor,&got)==LKS_STATUS_OK && got==h[0]);
    CHECK(lks_order_snapshot_is_current(o,s));
    CHECK(lks_order_remove(o,h[1],NULL)==LKS_STATUS_OK); h[1]=NULL;
    CHECK(lks_order_cursor_next(cursor,&got)==LKS_STATUS_INVALIDATED && !got);
    CHECK(!lks_order_snapshot_is_current(o,s));
    lks_order_cursor_destroy(cursor);
    CHECK(lks_snapshot_key_format(s,0,key,sizeof(key))==LKS_STATUS_OK);
    CHECK(!strcmp(key,"LS1.6576616c.0000000000000000"));
    CHECK(lks_snapshot_key_format(s,2,other,sizeof(other))==LKS_STATUS_OK);
    CHECK(strcmp(key,other)<0 && lks_snapshot_key_compare(key,other,&c)==LKS_STATUS_OK && c<0);
    CHECK(lks_snapshot_key_compare(key,"LS1.61.0000000000000000",&c)==LKS_STATUS_DOMAIN_MISMATCH && c==0);
    CHECK(lks_snapshot_key_validate("LS1.6A.0000000000000000")==LKS_STATUS_INVALID_ARGUMENT);
    bytes=lks_snapshot_serialized_size(s); wire=malloc(bytes+1); mutated=malloc(bytes+1); CHECK(wire&&mutated);
    memset(wire,0xA5,bytes+1);
    CHECK(lks_snapshot_serialize(s,wire,bytes-1)==LKS_STATUS_BUFFER_TOO_SMALL);
    for(i=0;i<bytes+1;i++) CHECK(wire[i]==0xA5);
    CHECK(lks_snapshot_serialize(s,wire,bytes)==LKS_STATUS_OK);
    for(i=0;i<bytes;i++) {
        CHECK(lks_snapshot_deserialize(wire,i,&loaded)==LKS_STATUS_INVALID_ARGUMENT && !loaded);
    }
    memcpy(mutated,wire,bytes); mutated[8]=1;
    CHECK(lks_snapshot_deserialize(mutated,bytes,&loaded)==LKS_STATUS_INVALID_ARGUMENT&&!loaded);
    wire[bytes]=0;
    CHECK(lks_snapshot_deserialize(wire,bytes+1,&loaded)==LKS_STATUS_INVALID_ARGUMENT&&!loaded);
    CHECK(lks_snapshot_deserialize(wire,bytes,&loaded)==LKS_STATUS_OK);
    {uint32_t random=712200;size_t trial,accepted=0,rejected=0;
     for(trial=0;trial<20000;trial++){
         LksSnapshot *probe=NULL;size_t position,length;unsigned char *roundtrip;LksStatus result;
         random=random*1664525u+1013904223u;position=(size_t)random%bytes;
         memcpy(mutated,wire,bytes);mutated[position]^=(unsigned char)(1u+(trial%255));
         result=lks_snapshot_deserialize(mutated,bytes,&probe);
         if(result==LKS_STATUS_OK){
             length=lks_snapshot_serialized_size(probe);CHECK(length==bytes);
             roundtrip=malloc(length);CHECK(roundtrip);
             CHECK(lks_snapshot_serialize(probe,roundtrip,length)==LKS_STATUS_OK&&!memcmp(roundtrip,mutated,length));
             free(roundtrip);lks_snapshot_destroy(probe);++accepted;
         }else{CHECK(result==LKS_STATUS_INVALID_ARGUMENT&&!probe);++rejected;}
     }
     printf("external wire mutations accepted=%zu rejected=%zu canonical_reserialize=true\n",accepted,rejected);}
    CHECK(!lks_order_snapshot_is_current(o,loaded));
    CHECK(lks_snapshot_restore_order(loaded,missing,NULL,&restored)==LKS_STATUS_NOT_FOUND&&!restored);
    items[0].identity=999; /* Copied association remains 11, borrowed Group payload would not. */
    lks_order_destroy(o);
    {const void *p=NULL;size_t n=0; CHECK(lks_snapshot_association(s,0,&p,&n)==LKS_STATUS_OK&&n==sizeof(int)&&*(const int *)p==11);}
    CHECK(lks_snapshot_serialize(loaded,mutated,bytes)==LKS_STATUS_OK&&!memcmp(mutated,wire,bytes));
    lks_snapshot_destroy(s);lks_snapshot_destroy(loaded);free(wire);free(mutated);
    return 0;
}
int main(void)
{
    Item items[8]={{2,0},{1,1},{2,2},{1,3},{2,4},{0,5},{2,6},{1,7}};
    void *input[8], *sorted[8]; int expected[8]={5,1,3,7,0,2,4,6}; size_t i;
    Context ctx={NULL,0,0};LksComparator cmp={compare,&ctx}; LksManagedOrder *m=lks_managed_order_create(&cmp);
    const LksOrderHandle *handles[8],*h,*p=NULL,*e=NULL,*s=NULL; Item query={1,99}; void *removed=NULL;
    LksGroup *g=NULL,*merged=NULL,*batchmerged=NULL;LksGroupBatch *batch=NULL;LksSnapshot *snap=NULL;
    LksSnapshotOptions options={"group",5,assoc,NULL};
    CHECK(m); ctx.managed=m;
    for(i=0;i<8;i++){input[i]=&items[i];sorted[i]=input[i];CHECK(lks_managed_order_insert(m,input[i],&handles[i])==LKS_STATUS_OK);}
    CHECK(ctx.violations==0&&ctx.calls>0);h=lks_managed_order_first(m);
    for(i=0;i<8;i++){CHECK(h==handles[expected[i]]);h=lks_order_next(h);}CHECK(!h);
    CHECK(lks_managed_order_locate(m,&query,&p,&e,&s)==LKS_STATUS_OK);
    CHECK(p==handles[5]&&e==handles[1]&&s==handles[0]);
    query.key=3; CHECK(lks_managed_order_locate(m,&query,&p,&e,&s)==LKS_STATUS_OK&&!e&&!s&&p==handles[6]);
    CHECK(lks_managed_order_remove(m,handles[0],&removed)==LKS_STATUS_OK&&removed==&items[0]); handles[0]=NULL;
    items[0].key=-1;CHECK(lks_managed_order_insert(m,&items[0],&handles[0])==LKS_STATUS_OK);
    CHECK(lks_managed_order_first(m)==handles[0]);
    {LksOrderCursor *cursor=NULL;const LksOrderHandle *got=NULL;
     CHECK(lks_managed_order_cursor_create(m,1,&cursor)==LKS_STATUS_OK);
     CHECK(lks_order_cursor_next(cursor,&got)==LKS_STATUS_OK&&got==lks_managed_order_last(m));
     lks_order_cursor_destroy(cursor);
     CHECK(lks_managed_order_compare(m,handles[0],handles[6],&(int){0})==LKS_STATUS_OK);
     CHECK(lks_managed_order_snapshot_capture(m,&options,&snap)==LKS_STATUS_OK);
     CHECK(lks_managed_order_snapshot_is_current(m,snap));
     lks_snapshot_destroy(snap);snap=NULL;}
    lks_managed_order_destroy(m);ctx.managed=NULL;items[0].key=2;
    CHECK(lks_sort(sorted,8,compare,&ctx)==LKS_STATUS_OK);
    CHECK(lks_group_build(input,8,&cmp,&g)==LKS_STATUS_OK);
    for(i=0;i<8;i++){CHECK(input[i]==&items[i]);CHECK(lks_group_item_at(g,i)==&items[expected[i]]);CHECK(sorted[i]==&items[expected[i]]);}
    CHECK(lks_group_merge(g,g,&cmp,&merged)==LKS_STATUS_OK&&lks_group_size(merged)==16);
    {int self_expected[16]={5,5,1,3,7,1,3,7,0,2,4,6,0,2,4,6};for(i=0;i<16;i++)CHECK(lks_group_item_at(merged,i)==&items[self_expected[i]]);}
    CHECK(lks_group_batch_build(input,8,3,&cmp,&batch)==LKS_STATUS_OK);
    CHECK(lks_group_batch_size(batch)==8&&lks_group_batch_group_count(batch)==3&&lks_group_batch_group_size(batch)==3);
    CHECK(lks_group_size(lks_group_batch_group_at(batch,0))==3);
    CHECK(lks_group_batch_merge_all(batch,&cmp,&batchmerged)==LKS_STATUS_OK);
    lks_group_batch_destroy(batch);
    for(i=0;i<8;i++)CHECK(lks_group_item_at(batchmerged,i)==&items[expected[i]]);
    items[1].identity=101; /* Immutable sequence owns pointers, not payload copies. */
    CHECK(((Item *)lks_group_item_at(batchmerged,1))->identity==101);
    items[1].identity=1;
    CHECK(lks_group_snapshot_capture(g,&options,&snap)==LKS_STATUS_OK&&lks_group_snapshot_is_current(g,snap));
    lks_group_destroy(g);lks_group_destroy(merged);
    CHECK(lks_snapshot_count(snap)==8);lks_snapshot_destroy(snap);lks_group_destroy(batchmerged);
    {LksV3Lk1ImportEntry entries[3]={{"LK1:201FF0000!",&items[2]},{"LK1:1!",&items[1]},{"LK1:001FF0000!",&items[0]}};LksOrder *o=NULL;
     CHECK(lks_order_import_v3_lk1(entries,3,&o)==LKS_STATUS_OK);h=lks_order_first(o);
     CHECK(lks_order_item(h)==&items[0]);h=lks_order_next(h);CHECK(lks_order_item(h)==&items[1]);h=lks_order_next(h);CHECK(lks_order_item(h)==&items[2]);lks_order_destroy(o);
     entries[2].key=entries[0].key;CHECK(lks_order_import_v3_lk1(entries,3,&o)==LKS_STATUS_ALREADY_EXISTS&&!o);}
    {LksOrder *o=lks_order_create(),*foreign=lks_order_create();const LksOrderHandle *a=NULL,*b=NULL;int c=42;void *out=&items[0];CHECK(o&&foreign);
     CHECK(lks_order_insert_back(o,NULL,&a)==LKS_STATUS_INVALID_ARGUMENT&&!a);
     CHECK(lks_order_insert_back(o,&items[0],&a)==LKS_STATUS_OK);CHECK(lks_order_insert_back(foreign,&items[1],&b)==LKS_STATUS_OK);
     CHECK(lks_order_remove(o,b,&out)==LKS_STATUS_INVALID_ARGUMENT&&!out);
     CHECK(lks_order_compare(o,a,b,&c)==LKS_STATUS_INVALID_ARGUMENT&&c==0);
     CHECK(lks_order_move_before(o,a,b)==LKS_STATUS_INVALID_ARGUMENT&&lks_order_first(o)==a);
     CHECK(lks_order_remove(o,a,&out)==LKS_STATUS_OK&&out==&items[0]);a=NULL;
     CHECK(!lks_order_first(o)&&!lks_order_last(o)&&!lks_order_size(o));lks_order_destroy(o);lks_order_destroy(foreign);}
    CHECK(persistence()==0);
    puts("contracts: managed equality/context/reentry/edit, Group/Batch/sort, cursor, LS1/wire/history, legacy import, live foreign errors PASS");
    return 0;
}
