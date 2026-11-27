#define _POSIX_C_SOURCE 200809L
#include <glib.h>
#include <layerkeysort.h>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <time.h>
#include <sys/resource.h>
#include <unistd.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { uint64_t key, id; } Item;
typedef struct { unsigned type; size_t a, b; } Op;
typedef struct { size_t p,e,s; } Answer;
static uint64_t state=UINT64_C(0x91e10da5c79e7b1d);
static uint64_t rng(void){state^=state<<13;state^=state>>7;state^=state<<17;return state;}
static void fail(const char *s){fprintf(stderr,"FAIL: %s\n",s);exit(2);}
#define REQUIRE(x) do {if(!(x)) fail(#x);} while(0)
static void *allocate(size_t n,size_t s){void *p=calloc(n,s);REQUIRE(p!=NULL);return p;}
static int keycmp(const void *a,const void *b,void *ctx){const Item *x=a,*y=b;(void)ctx;return (x->key>y->key)-(x->key<y->key);}
static int arraycmp(const void *a,const void *b,void *ctx){return keycmp(*(void *const *)a,*(void *const *)b,ctx);}
static int referencecmp(const void *a,const void *b){const Item *x=*(Item *const *)a,*y=*(Item *const *)b;int c=keycmp(x,y,NULL);return c?c:(x->id>y->id)-(x->id<y->id);}
static uint64_t hash_id(uint64_t h,size_t id){return (h^(uint64_t)id)*UINT64_C(1099511628211);}
/* Both implementations use the same clock and whole-process memory sampler. */
static double clock_ms(void){
#ifdef _WIN32
 LARGE_INTEGER x,f;QueryPerformanceCounter(&x);QueryPerformanceFrequency(&f);return 1000.0*(double)x.QuadPart/(double)f.QuadPart;
#else
 struct timespec t;REQUIRE(clock_gettime(CLOCK_MONOTONIC,&t)==0);return 1000.0*t.tv_sec+t.tv_nsec/1e6;
#endif
}
static void memory_sample(uint64_t *resident,uint64_t *peak,uint64_t *commit){
#ifdef _WIN32
 PROCESS_MEMORY_COUNTERS_EX m={0};REQUIRE(GetProcessMemoryInfo(GetCurrentProcess(),(PROCESS_MEMORY_COUNTERS*)&m,sizeof(m)));
 *resident=m.WorkingSetSize;*peak=m.PeakWorkingSetSize;*commit=m.PeakPagefileUsage;
#else
 struct rusage u;REQUIRE(getrusage(RUSAGE_SELF,&u)==0);*peak=(uint64_t)u.ru_maxrss*1024;*commit=0;
 FILE *f=fopen("/proc/self/statm","r");unsigned long long total=0,pages=0;REQUIRE(f!=NULL);REQUIRE(fscanf(f,"%llu %llu",&total,&pages)==2);fclose(f);*resident=pages*(uint64_t)sysconf(_SC_PAGESIZE);
#endif
}
static size_t item_id(void *p){return p?((Item*)p)->id:SIZE_MAX;}
static size_t *previous,*next_id,head=SIZE_MAX,tail=SIZE_MAX;
static void unlink_id(size_t id){size_t p=previous[id],q=next_id[id];if(p!=SIZE_MAX)next_id[p]=q;else head=q;if(q!=SIZE_MAX)previous[q]=p;else tail=p;}
static void insert_id(size_t id,size_t before){size_t p=before==SIZE_MAX?tail:previous[before];previous[id]=p;next_id[id]=before;if(p!=SIZE_MAX)next_id[p]=id;else head=id;if(before!=SIZE_MAX)previous[before]=id;else tail=id;}
/* Array-based implicit treap: independent rank oracle for arbitrary live pairs.
   Linked-list reference updates independently validate its complete final order. */
typedef struct {size_t l,r,p,size;uint64_t priority;} Oracle;
static Oracle *oracle;
static size_t root=SIZE_MAX;
static size_t osize(size_t a){return a==SIZE_MAX?0:oracle[a].size;}
static void pull(size_t a){oracle[a].size=1+osize(oracle[a].l)+osize(oracle[a].r);if(oracle[a].l!=SIZE_MAX)oracle[oracle[a].l].p=a;if(oracle[a].r!=SIZE_MAX)oracle[oracle[a].r].p=a;}
static size_t merge(size_t a,size_t b){
 if(a==SIZE_MAX)return b;
 if(b==SIZE_MAX)return a;
 if(oracle[a].priority<oracle[b].priority){oracle[a].r=merge(oracle[a].r,b);pull(a);return a;}
 oracle[b].l=merge(a,oracle[b].l);pull(b);return b;
}
static void split(size_t a,size_t k,size_t *l,size_t *r){
 if(a==SIZE_MAX){*l=*r=SIZE_MAX;return;}
 size_t left=osize(oracle[a].l);
 if(k<=left){split(oracle[a].l,k,l,&oracle[a].l);pull(a);*r=a;}else{split(oracle[a].r,k-left-1,&oracle[a].r,r);pull(a);*l=a;}
 if(*l!=SIZE_MAX)oracle[*l].p=SIZE_MAX;
 if(*r!=SIZE_MAX)oracle[*r].p=SIZE_MAX;
}
static size_t rank_id(size_t a){size_t rank=osize(oracle[a].l);while(oracle[a].p!=SIZE_MAX){size_t p=oracle[a].p;if(oracle[p].r==a)rank+=1+osize(oracle[p].l);a=p;}return rank;}
static void oracle_insert(size_t a,size_t before){size_t k=before==SIZE_MAX?osize(root):rank_id(before),l,r;split(root,k,&l,&r);root=merge(merge(l,a),r);oracle[root].p=SIZE_MAX;}
static void oracle_remove(size_t a){size_t l,r,b;split(root,rank_id(a),&l,&r);split(r,1,&b,&r);REQUIRE(b==a);root=merge(l,r);if(root!=SIZE_MAX)oracle[root].p=SIZE_MAX;oracle[a].l=oracle[a].r=oracle[a].p=SIZE_MAX;oracle[a].size=1;}
static void check_sequence(int glib,GSequence *g,LksOrder *o,LksManagedOrder *m,Item **expected,size_t count){
 GSequenceIter *it=glib?g_sequence_get_begin_iter(g):NULL;
 const LksOrderHandle *h=glib?NULL:(m?lks_managed_order_first(m):lks_order_first(o));
 for(size_t i=0;i<count;i++){


  if(glib){REQUIRE(!g_sequence_iter_is_end(it));REQUIRE(g_sequence_get(it)==expected[i]);it=g_sequence_iter_next(it);}
  else {REQUIRE(h!=NULL);REQUIRE(lks_order_item(h)==expected[i]);h=lks_order_next(h);}
 }
 if(glib)REQUIRE(g_sequence_iter_is_end(it));else REQUIRE(h==NULL);
}
int main(int argc,char **argv){
 if(argc==2 && strcmp(argv[1],"version")==0){printf("%u.%u.%u\n",glib_major_version,glib_minor_version,glib_micro_version);return 0;}
 REQUIRE(argc==4 || argc==5);if(argc==5)state=strtoull(argv[4],NULL,0);int glib=strcmp(argv[1],"glib")==0;const char *scenario=argv[2];size_t n=(size_t)strtoull(argv[3],NULL,10),cap=2*n+1;
 REQUIRE(n>0 && n<=1000000);
 int extended=strncmp(scenario,"mix_",4)==0,lifecycle=strstr(scenario,"lifecycle")!=NULL;
 int sorting=strncmp(scenario,"sort_",5)==0,managed=strncmp(scenario,"managed_",8)==0;
 Item *items=allocate(cap,sizeof(*items));Item **reference=allocate(cap,sizeof(*reference));void **array=allocate(cap,sizeof(*array));void **handles=allocate(cap,sizeof(*handles));
 previous=allocate(cap,sizeof(*previous));next_id=allocate(cap,sizeof(*next_id));Op *ops=allocate(n,sizeof(*ops));Answer *answers=allocate(n,sizeof(*answers));size_t *active=allocate(n,sizeof(*active));
 for(size_t i=0;i<cap;i++){items[i].id=i;items[i].key=i;reference[i]=&items[i];array[i]=&items[i];previous[i]=next_id[i]=SIZE_MAX;}
 int *expected_sign=NULL;
 if(extended){
  oracle=allocate(cap,sizeof(*oracle));expected_sign=allocate(n,sizeof(*expected_sign));
  for(size_t i=0;i<cap;i++){
   uint64_t z=(uint64_t)i+UINT64_C(0x9e3779b97f4a7c15);z=(z^(z>>30))*UINT64_C(0xbf58476d1ce4e5b9);z=(z^(z>>27))*UINT64_C(0x94d049bb133111eb);
   oracle[i]=(Oracle){SIZE_MAX,SIZE_MAX,SIZE_MAX,1,z^(z>>31)};
  }
  for(size_t i=0;i<n;i++)oracle_insert(i,SIZE_MAX);
 }
 uint64_t trace_hash=UINT64_C(1469598103934665603),fingerprint=trace_hash;size_t expected_count=n,op_count=n;
 if(sorting){
  for(size_t i=0;i<n;i++){
   if(strcmp(scenario,"sort_random")==0)items[i].key=rng();
   else if(strcmp(scenario,"sort_reverse")==0)items[i].key=n-i;
   else if(strcmp(scenario,"sort_duplicates")==0)items[i].key=rng()%16;
  }
  if(strcmp(scenario,"sort_nearly")==0)for(size_t i=0;i<n/100;i++){size_t a=rng()%n,b=rng()%n;uint64_t t=items[a].key;items[a].key=items[b].key;items[b].key=t;}
  qsort(reference,n,sizeof(*reference),referencecmp);
 }else if(managed){
  for(size_t i=0;i<n;i++)active[i]=i;
  for(size_t i=n;i>1;i--){size_t j=rng()%i,t=active[i-1];active[i-1]=active[j];active[j]=t;}
  for(size_t i=0;i<n;i++)items[i].key=2*active[i];
  qsort(reference,n,sizeof(*reference),referencecmp);
  for(size_t i=0;i<n;i++){ops[i].a=(size_t)(rng()%(2*n+2));ops[i].b=i;}
  if(strcmp(scenario,"managed_remove")==0){for(size_t i=n;i>1;i--){size_t j=rng()%i,t=ops[i-1].b;ops[i-1].b=ops[j].b;ops[j].b=t;}expected_count=0;}
 }else{
  if(strcmp(scenario,"insert")!=0)for(size_t i=0;i<n;i++){insert_id(i,SIZE_MAX);active[i]=i;}
  for(size_t i=0;i<n;i++){
   if(strcmp(scenario,"insert")==0){ops[i]=(Op){0,i,i?rng()%i:SIZE_MAX};insert_id(i,ops[i].b);}
   else if(strcmp(scenario,"remove")==0){size_t left=n-i,k=rng()%left,a=active[k];active[k]=active[left-1];ops[i]=(Op){1,a,0};unlink_id(a);}
   else if(strcmp(scenario,"move")==0){size_t a=rng()%n,b=rng()%n;ops[i]=(Op){2,a,b};if(a!=b){unlink_id(a);insert_id(a,b);}}
   else if(strcmp(scenario,"compare")==0){size_t a=rng()%n,b=rng()%n;ops[i]=(Op){3,a,b};}
   else if(lifecycle){ /* Population and destruction are measured separately. */ }
   else if(extended){
    unsigned q=strstr(scenario,"query")?90:(strstr(scenario,"mutation")?10:50);
    unsigned roll=(unsigned)(rng()%100);size_t k=rng()%n,a=active[k],b=active[rng()%n];
    if(roll<q){ops[i]=(Op){3,a,b};size_t ra=rank_id(a),rb=rank_id(b);expected_sign[i]=(ra>rb)-(ra<rb);}
    else{
     if(strstr(scenario,"hotspot")){size_t pool=n<64?n:64;k=rng()%pool;a=active[k];b=active[rng()%pool];}
     if(roll<q+(100-q)/2){ops[i]=(Op){2,a,b};if(a!=b){oracle_remove(a);oracle_insert(a,b);unlink_id(a);insert_id(a,b);}}
     else{ops[i]=(Op){4,a,n+i};oracle_remove(a);oracle_insert(n+i,SIZE_MAX);unlink_id(a);insert_id(n+i,SIZE_MAX);active[k]=n+i;}
    }
   }
   else if(strcmp(scenario,"mixed")==0){
    size_t k=rng()%n,a=active[k],b=active[rng()%n];unsigned type=(unsigned)(i%4);
    if(type==0){ops[i]=(Op){1,a,0};unlink_id(a);active[k]=n+i;}
    else if(type==1){ops[i]=(Op){2,a,b};if(a!=b){unlink_id(a);insert_id(a,b);}}
    else if(type==2){ops[i]=(Op){2,a,b};if(a!=b){unlink_id(a);insert_id(a,b);}}
    else {b=next_id[a];ops[i]=(Op){3,a,b==SIZE_MAX?a:b};}
    if(type==0){/* Encode replacement as one timed remove+insert operation, constant live size. */ops[i].type=4;ops[i].b=n+i;insert_id(n+i,SIZE_MAX);}
   }else fail("unknown scenario");
  }
  if(strcmp(scenario,"remove")==0)expected_count=0;
  size_t at=head,c=0;while(at!=SIZE_MAX){REQUIRE(c<cap);reference[c++]=&items[at];at=next_id[at];}REQUIRE(c==expected_count);if(extended)for(size_t j=0;j<c;j++)REQUIRE(rank_id(reference[j]->id)==j);
 }
 for(size_t i=0;i<n;i++){trace_hash=hash_id(trace_hash,items[i].key);trace_hash=hash_id(trace_hash,ops[i].type);trace_hash=hash_id(trace_hash,ops[i].a);trace_hash=hash_id(trace_hash,ops[i].b);}
 GSequence *g=NULL;LksOrder *o=NULL;LksManagedOrder *m=NULL;LksComparator cmp={keycmp,NULL};
 uint64_t baseline,peak_resident,peak_commit,unused;memory_sample(&baseline,&peak_resident,&peak_commit);
 double create_ms=0,populate_ms=0,t=clock_ms();
 if(!sorting){if(glib)g=g_sequence_new(NULL);else if(managed)m=lks_managed_order_create(&cmp);else o=lks_order_create();REQUIRE(glib?g!=NULL:(managed?m!=NULL:o!=NULL));
  create_ms=clock_ms()-t;t=clock_ms();
  if(strcmp(scenario,"insert")!=0 && strcmp(scenario,"managed_insert")!=0)for(size_t i=0;i<n;i++){
   if(glib)handles[i]=managed?g_sequence_insert_sorted(g,&items[i],keycmp,NULL):g_sequence_append(g,&items[i]);
   else {const LksOrderHandle *h=NULL;REQUIRE((managed?lks_managed_order_insert(m,&items[i],&h):lks_order_insert_back(o,&items[i],&h))==LKS_STATUS_OK);handles[i]=(void*)h;}
  }
 }
 populate_ms=clock_ms()-t;
 double start=clock_ms();
 if(lifecycle){ /* Only lifecycle phases are measured for these cells. */ }
 else if(sorting){if(glib)g_sort_array(array,n,sizeof(void*),arraycmp,NULL);else REQUIRE(lks_sort(array,n,keycmp,NULL)==LKS_STATUS_OK);}
 else if(managed){for(size_t i=0;i<n;i++){
  if(strcmp(scenario,"managed_insert")==0){if(glib)handles[i]=g_sequence_insert_sorted(g,&items[i],keycmp,NULL);else {const LksOrderHandle *h=NULL;REQUIRE(lks_managed_order_insert(m,&items[i],&h)==LKS_STATUS_OK);handles[i]=(void*)h;}}
  else if(strcmp(scenario,"managed_remove")==0){size_t a=ops[i].b;if(glib){answers[i].e=item_id(g_sequence_get(handles[a]));g_sequence_remove(handles[a]);}else {void *p=NULL;REQUIRE(lks_managed_order_remove(m,handles[a],&p)==LKS_STATUS_OK);answers[i].e=item_id(p);}}
  else {Item q={ops[i].a,0};if(glib){GSequenceIter *u=g_sequence_search(g,&q,keycmp,NULL),*p=g_sequence_iter_is_begin(u)?NULL:g_sequence_iter_prev(u),*e=NULL;
    if(p && ((Item*)g_sequence_get(p))->key==q.key){e=p;p=g_sequence_iter_is_begin(e)?NULL:g_sequence_iter_prev(e);}
    answers[i]=(Answer){p?item_id(g_sequence_get(p)):SIZE_MAX,e?item_id(g_sequence_get(e)):SIZE_MAX,g_sequence_iter_is_end(u)?SIZE_MAX:item_id(g_sequence_get(u))};
   }else {const LksOrderHandle *p,*e,*s;REQUIRE(lks_managed_order_locate(m,&q,&p,&e,&s)==LKS_STATUS_OK);answers[i]=(Answer){p?item_id(lks_order_item(p)):SIZE_MAX,e?item_id(lks_order_item(e)):SIZE_MAX,s?item_id(lks_order_item(s)):SIZE_MAX};}}
 }}else for(size_t i=0;i<n;i++){
  Op op=ops[i];if(op.type==0){if(glib)handles[op.a]=g_sequence_insert_before(op.b==SIZE_MAX?g_sequence_get_end_iter(g):handles[op.b],&items[op.a]);else {const LksOrderHandle *h=NULL;REQUIRE((op.b==SIZE_MAX?lks_order_insert_back(o,&items[op.a],&h):lks_order_insert_before(o,handles[op.b],&items[op.a],&h))==LKS_STATUS_OK);handles[op.a]=(void*)h;}}
  else if(op.type==1 || op.type==4){if(glib){answers[i].e=item_id(g_sequence_get(handles[op.a]));g_sequence_remove(handles[op.a]);}else {void *p=NULL;REQUIRE(lks_order_remove(o,handles[op.a],&p)==LKS_STATUS_OK);answers[i].e=item_id(p);}
   if(op.type==4){if(glib)handles[op.b]=g_sequence_append(g,&items[op.b]);else {const LksOrderHandle *h=NULL;REQUIRE(lks_order_insert_back(o,&items[op.b],&h)==LKS_STATUS_OK);handles[op.b]=(void*)h;}}}
  else if(op.type==2){if(glib)g_sequence_move(handles[op.a],handles[op.b]);else REQUIRE(lks_order_move_before(o,handles[op.a],handles[op.b])==LKS_STATUS_OK);}
  else {int c=0;if(glib)c=g_sequence_iter_compare(handles[op.a],handles[op.b]);else REQUIRE(lks_order_compare(o,handles[op.a],handles[op.b],&c)==LKS_STATUS_OK);answers[i].e=(size_t)((c>0)-(c<0)+1);}
 }
 double elapsed=clock_ms()-start;
 if(sorting){for(size_t i=0;i<n;i++)REQUIRE(array[i]==reference[i]);}
 else {check_sequence(glib,g,o,m,reference,expected_count);
  for(size_t i=0;i<n;i++){
   if(managed && strcmp(scenario,"managed_lookup")==0){size_t q=ops[i].a,lo=(q+1)/2,s=q/2+1;Answer e={lo?reference[(lo-1<n)?lo-1:n-1]->id:SIZE_MAX,(q%2==0 && q/2<n)?reference[q/2]->id:SIZE_MAX,s<n?reference[s]->id:SIZE_MAX};REQUIRE(answers[i].p==e.p && answers[i].e==e.e && answers[i].s==e.s);}
   else if(managed && strcmp(scenario,"managed_remove")==0)REQUIRE(answers[i].e==ops[i].b);
   else if(!managed && (ops[i].type==1 || ops[i].type==4))REQUIRE(answers[i].e==ops[i].a);
   else if(!managed && ops[i].type==3){int c=extended?expected_sign[i]:(strcmp(scenario,"compare")==0?((ops[i].a>ops[i].b)-(ops[i].a<ops[i].b)):(ops[i].a==ops[i].b?0:-1));REQUIRE(answers[i].e==(size_t)(c+1));}
  }
 }
 for(size_t i=0;i<expected_count;i++)fingerprint=hash_id(fingerprint,reference[i]->id);
 for(size_t i=0;i<n;i++){fingerprint=hash_id(fingerprint,answers[i].p);fingerprint=hash_id(fingerprint,answers[i].e);fingerprint=hash_id(fingerprint,answers[i].s);}
 memory_sample(&unused,&peak_resident,&peak_commit);
 t=clock_ms();
 if(g)g_sequence_free(g);
 if(o)lks_order_destroy(o);
 if(m)lks_managed_order_destroy(m);
 double destroy_ms=clock_ms()-t;
 printf("%s,%s,%zu,%zu,%.9f,%llu,%llu,%llu,%llu,%llu,%llu,OK,%.9f,%.9f,%.9f\n",glib?"glib":"full",scenario,n,op_count,elapsed,(unsigned long long)baseline,(unsigned long long)peak_resident,(unsigned long long)peak_commit,(unsigned long long)trace_hash,(unsigned long long)fingerprint,(unsigned long long)expected_count,create_ms,populate_ms,destroy_ms);
 free(oracle);free(expected_sign);
 free(active);free(answers);free(ops);free(next_id);free(previous);free(handles);free(array);free(reference);free(items);return 0;
}
