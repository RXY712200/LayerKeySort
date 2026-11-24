/* Independent research baselines; no LayerKeySort implementation code reused.
 * TREE selects a threaded implicit treap; otherwise a stable-node list. */
#include "baseline.h"
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
struct Node {
    Node *prev, *next;
    void *item;
#if TREE
    Node *left, *right, *parent;
    size_t size;
    uint64_t priority;
#endif
};
struct Sequence { Node *first, *last; size_t count;
#if TREE
    Node *root; uint64_t random;
#endif
};
static Stats stats;
static size_t failure;
static void *allocate(size_t n) {
#ifdef DIAGNOSTIC
    ++stats.attempts;
    if (failure && stats.attempts == failure) return NULL;
#endif
    void *p = calloc(1,n);
#ifdef DIAGNOSTIC
    if(p) { ++stats.allocations; stats.live+=n; if(stats.live>stats.peak)stats.peak=stats.live; }
#endif
    return p;
}
static void release(void *p,size_t n) {
    if(!p)return;
#ifdef DIAGNOSTIC
    ++stats.frees; assert(stats.live>=n); stats.live-=n;
#else
    (void)n;
#endif
    free(p);
}
void seq_fail(size_t ordinal) { failure=ordinal;stats.attempts=0; }
Stats seq_stats(void) { return stats; }
#if TREE
static size_t size(Node *n) { return n?n->size:0; }
static void refresh(Node *n) {
    n->size=1+size(n->left)+size(n->right);
    if(n->left)n->left->parent=n;
    if(n->right)n->right->parent=n;
}
/* Split by occurrence count, never by application payload. Root parents are
 * reset on every return; callers restore child parents through refresh. */
static void split(Node *n,size_t k,Node **a,Node **b) {
    if(!n) { *a=*b=NULL;return; }
    if(k<=size(n->left)) { split(n->left,k,a,&n->left);refresh(n);*b=n; }
    else { split(n->right,k-size(n->left)-1,&n->right,b);refresh(n);*a=n; }
    if(*a)(*a)->parent=NULL;
    if(*b)(*b)->parent=NULL;
}
static Node *merge(Node *a,Node *b) {
    Node *n;
    if(!a)n=b;
    else if(!b)n=a;
    else if(a->priority<b->priority) { a->right=merge(a->right,b);refresh(a);n=a; }
    else { b->left=merge(a,b->left);refresh(b);n=b; }
    if(n)n->parent=NULL;
    return n;
}
static size_t rank_of(Node *n) {
    size_t k=size(n->left);
    while(n->parent) { if(n==n->parent->right)k+=1+size(n->parent->left);n=n->parent; }
    return k;
}
static uint64_t priority(Sequence *s) {
    uint64_t z=(s->random+=UINT64_C(0x9e3779b97f4a7c15));
    z=(z^(z>>30))*UINT64_C(0xbf58476d1ce4e5b9);
    z=(z^(z>>27))*UINT64_C(0x94d049bb133111eb);return z^(z>>31);
}
#endif
Sequence *seq_create(void) { return allocate(sizeof(Sequence)); }
static void attach(Sequence *s,Node *n,Node *anchor,int after) {
    Node *prev=anchor?(after?anchor:anchor->prev):s->last;
    Node *next=prev?prev->next:s->first;
#if TREE
    Node *a,*b;size_t k=anchor?rank_of(anchor)+(size_t)after:s->count;
    split(s->root,k,&a,&b);s->root=merge(merge(a,n),b);
#endif
    n->prev=prev;n->next=next;
    if(prev)prev->next=n;else s->first=n;
    if(next)next->prev=n;else s->last=n;
    ++s->count;
}
Node *seq_insert(Sequence *s,Node *anchor,int after,void *item) {
    Node *n=allocate(sizeof(Node));if(!n)return NULL;n->item=item;
#if TREE
    n->size=1;n->priority=priority(s);
#endif
    attach(s,n,anchor,after);return n;
}
static void detach(Sequence *s,Node *n) {
#if TREE
    Node *a,*b,*one;split(s->root,rank_of(n),&a,&b);split(b,1,&one,&b);
    assert(one==n);s->root=merge(a,b);n->left=n->right=n->parent=NULL;n->size=1;
#endif
    if(n->prev)n->prev->next=n->next;else s->first=n->next;
    if(n->next)n->next->prev=n->prev;else s->last=n->prev;
    --s->count;
}
void seq_remove(Sequence *s,Node *n) { detach(s,n);release(n,sizeof(*n)); }
void seq_move(Sequence *s,Node *n,Node *a,int after) {
    if(n==a || (after?a->next==n:n->next==a))return;
    detach(s,n);attach(s,n,a,after);
}
int seq_compare(Node *a,Node *b) {
    if(a==b)return 0;
#if TREE
    size_t x=rank_of(a),y=rank_of(b);return (x>y)-(x<y);
#else
    for(Node *p=a->next;p;p=p->next)if(p==b)return -1;
    return 1;
#endif
}
Node *seq_first(Sequence *s) { return s->first; }
Node *seq_last(Sequence *s) { return s->last; }
Node *seq_next(Node *h) { return h->next; }
Node *seq_previous(Node *h) { return h->prev; }
void *seq_item(Node *h) { return h->item; }
size_t seq_size(Sequence *s) { return s->count; }
#if TREE
static int check(Node *n,Node *parent,Node **prev,size_t *budget,unsigned depth) {
    if(!n)return 1;
    if(depth>256 || !*budget || n->parent!=parent)return 0;
    --*budget;
    if(n->size!=1+size(n->left)+size(n->right) ||
       (n->left && n->left->priority<n->priority) ||
       (n->right && n->right->priority<n->priority))return 0;
    if(!check(n->left,n,prev,budget,depth+1) || n->prev!=*prev)return 0;
    *prev=n;return check(n->right,n,prev,budget,depth+1);
}
#endif
int seq_valid(Sequence *s) {
    Node *p=s->first,*prev=NULL;size_t n=0;
    while(p) { if(n++>=s->count || p->prev!=prev)return 0;prev=p;p=p->next; }
    if(n!=s->count || prev!=s->last)return 0;
#if TREE
    n=s->count;prev=NULL;
    if(!check(s->root,NULL,&prev,&n,0) || n || prev!=s->last)return 0;
#endif
    return 1;
}
void seq_destroy(Sequence *s) {
    Node *p=s->first;while(p) { Node *next=p->next;release(p,sizeof(*p));p=next; }
    release(s,sizeof(*s));
}
