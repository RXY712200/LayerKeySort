"""Generate lifecycle adapters from the preserved Preview.2 driver."""
from pathlib import Path
import sys
here=Path(__file__).resolve().parent
s=(here.parent/'full-v4.2.0-preview.2/lifecycle.c').read_text(encoding='utf-8')
s=s.replace('#if FLAT\n','#if FLAT\n#include "baseline.h"\n',1)
a=s.index('typedef struct Ref')
b=s.index('static Snapshot *snap_new',a)
old=s[a:b]
snap=old[old.index('typedef struct Snapshot'):old.index('static Order *create')]
s=s[:a]+'''#if INDEXED
typedef Sequence Order;
'''+snap+'''
static Order *create(void){return seq_create();}
static void append(Order *o,Item *x){CHECK(seq_insert(o,seq_last(o),1,x));}
static void move(Order *o){seq_move(o,seq_first(o),seq_last(o),1);}
static void destroy(Order *o){seq_destroy(o);}
#else
'''+old+'''#endif
'''+s[b:]
s=s.replace('    for(Ref *r=o->first;r;r=r->next){','''#if INDEXED
    for(Node *r=seq_first(o);r;r=seq_next(r)){
        Item *item=seq_item(r);
#else
    for(Ref *r=o->first;r;r=r->next){
        Item *item=r->item;
#endif''').replace('association(r->item,','association(item,').replace('snap_new(o->n,domain,d)', 'snap_new(ORDER_SIZE(o),domain,d)')
s=s.replace('#if INDEXED\ntypedef Sequence', '#if INDEXED\n#define ORDER_SIZE(o) seq_size(o)\ntypedef Sequence').replace('#else\ntypedef struct Ref','#else\n#define ORDER_SIZE(o) ((o)->n)\ntypedef struct Ref')
s=s.replace('static Item *first(Order *o){return o->first->item;}','''#if INDEXED
static Item *first(Order *o){return seq_item(seq_first(o));}
#else
static Item *first(Order *o){return o->first->item;}
#endif''')
s=s.replace('Item *items; unsigned char', 'Item *items; size_t *expected; unsigned char')
s=s.replace('id=(row+c->epoch)%c->n','id=c->expected ? c->expected[row] : (row+c->epoch)%c->n')
s=s.replace('int main(int argc,char **argv)', r"""
#if MIXED
static void mixed(Order *o, size_t *model, size_t n, size_t seed)
{
    size_t at=1+seed%(n-2);
#if INDEXED
    Node *a=seq_first(o),*b=a;
    for(size_t i=0;i<at;++i)b=seq_next(b);
    CHECK(seq_compare(a,b)<0 && ((Item *)seq_item(b))->id==model[at]);
    Node *added=seq_insert(o,b,1,seq_item(b));
    CHECK(added && added!=b && seq_compare(b,added)<0);
    seq_remove(o,added);
    seq_move(o,a,b,1);
    CHECK(seq_compare(b,a)<0);
#else
    const LksOrderHandle *a=lks_order_first(o),*b=a,*added=NULL;
    int cmp;
    for(size_t i=0;i<at;++i)b=lks_order_next(b);
    OK(lks_order_compare(o,a,b,&cmp));
    CHECK(cmp<0 && ((Item *)lks_order_item(b))->id==model[at]);
    OK(lks_order_insert_after(o,b,lks_order_item(b),&added));
    CHECK(added!=b);
    OK(lks_order_compare(o,b,added,&cmp));
    CHECK(cmp<0);
    OK(lks_order_remove(o,added,NULL));
    OK(lks_order_move_after(o,a,b));
    OK(lks_order_compare(o,b,a,&cmp));
    CHECK(cmp<0);
#endif
    size_t id=model[0];
    memmove(model,model+1,at*sizeof(*model));
    model[at]=id;
    CHECK(first(o)->id==model[0]);
}
#endif
int main(int argc,char **argv)""")
s=s.replace('    size_t bytes=0;',r"""    size_t bytes=0;
#if MIXED
    size_t *models=malloc((repeats+1)*n*sizeof(*models));
    CHECK(models);
    for(size_t i=0;i<n;++i)models[i]=i;
#endif""")
s=s.replace('        start=now();move(order);CHECK(first(order)->id==j+1);edit+=now()-start;',r"""
#if MIXED
        start=now();mixed(order,models,n,1779033703u+j*7919u);edit+=now()-start;
        if(repeats)memcpy(models+(j+1)*n,models,n*sizeof(*models));
#else
        start=now();move(order);CHECK(first(order)->id==j+1);edit+=now()-start;
#endif""")
s=s.replace('        Snapshot *s=history[j];',r"""        Snapshot *s=history[j];
#if MIXED
        ctx.expected=models+(j+1)*n;
#endif""")
s=s.replace('value((i+j+1)%n,i,j+1,k)','value(ctx.expected?ctx.expected[i]:(i+j+1)%n,i,j+1,k)')
s=s.replace('        CHECK(first(next)->id==j+1);move(next);CHECK(first(next)->id==j+2);',r"""
        CHECK(first(next)->id==(ctx.expected?ctx.expected[0]:j+1));move(next);
        CHECK(first(next)->id==(ctx.expected?ctx.expected[1]:j+2));""")
s=s.replace('    LksAllocStats stats=',r"""
#if MIXED
    free(models);
#endif
    LksAllocStats stats=""")
if len(sys.argv)>2:
    seed, rate=map(int,sys.argv[2:4])
    assert 0<seed<2**32 and 0<rate<=64
    s=s.replace('mixed(order,models,n,1779033703u+j*7919u);',
        f'for(size_t e=0;e<{rate};++e)mixed(order,models,n,{seed}u+j*7919u+e*317u);')
    s=s.replace('    size_t retained=lks_alloc_stats_get().live_bytes;',
        """    size_t retained=lks_alloc_stats_get().live_bytes;
#if INDEXED
    retained+=seq_stats().live;
#endif""")
    s=s.replace('    CHECK(!stats.live_bytes&&!stats.live_blocks);',
        """    CHECK(!stats.live_bytes&&!stats.live_blocks);
#if INDEXED
    CHECK(!seq_stats().live);
    fprintf(stderr,"sequence_peak=%zu,snapshot_peak=%zu\\n",seq_stats().peak,stats.peak_live_bytes);
#endif""")
s=s.replace('    unsigned char domain[4096]', '    double wall_start=now();\n    unsigned char domain[4096]')
s=s.replace('%.9f,%zu,%zu,%zu,%zu\\n",', '%.9f,%zu,%zu,%zu,%zu,%.9f\\n",')
s=s.replace('stats.peak_live_bytes,stats.alloc_calls+stats.realloc_calls);',
    'stats.peak_live_bytes,stats.alloc_calls+stats.realloc_calls,now()-wall_start);')
Path(sys.argv[1]).write_text(s,encoding='utf-8')
