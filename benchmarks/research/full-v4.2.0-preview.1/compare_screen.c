#include "lks_order_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "check failed at %d\n", __LINE__); exit(1); } } while (0)

typedef struct Pair { size_t a, b; } Pair;
enum { PAIRS = 8192, LOOPS = 48 };
static volatile int sink;

static int ancestor_compare(const LksOrder *order, const LksOrderHandle *a,
                            const LksOrderHandle *b, int *out)
{
    *out = 0;
    if (!order || !a || !b || a->block->owner != order || b->block->owner != order)
        return LKS_STATUS_INVALID_ARGUMENT;
    if (order->callback_active) return LKS_STATUS_REENTRANT;
    const LksOrderBlock *left = a->block, *right = b->block;
    if (left == right) {
        *out = (a->local > b->local) - (a->local < b->local);
        return LKS_STATUS_OK;
    }
    while (left->parent != right->parent) {
        if (left->height < right->height) {
            if (left->parent == right) {
                *out = right->left == left ? -1 : 1;
                return LKS_STATUS_OK;
            }
            left = left->parent;
        } else {
            if (right->parent == left) {
                *out = left->right == right ? -1 : 1;
                return LKS_STATUS_OK;
            }
            right = right->parent;
        }
    }
    *out = left->parent->left == left ? -1 : 1;
    return LKS_STATUS_OK;
}

static uint32_t draw(uint32_t *state)
{
    *state = 1664525u * *state + 1013904223u;
    return *state >> 8;
}

int main(void)
{
    LARGE_INTEGER frequency;
    CHECK(QueryPerformanceFrequency(&frequency));
    puts("population,state,repeat,kernel,operations,ns_per_op");
    for (size_t n = 1024; n <= 100000; n = n == 1024 ? 100000 : 100001) {
        size_t *items = calloc(n,sizeof(*items));
        const LksOrderHandle **handles = calloc(n,sizeof(*handles));
        Pair *pairs = calloc(PAIRS,sizeof(*pairs));
        LksOrder *order = lks_order_create();
        CHECK(items && handles && pairs && order);
        for (size_t i=0; i<n; i++) {
            items[i] = i;
            CHECK(lks_order_insert_back(order,items+i,handles+i)==LKS_STATUS_OK);
        }
        uint32_t random = 17;
        for (int cross=0; cross<2; cross++) {
            for (size_t i=0; i<PAIRS; i++) {
                size_t a = draw(&random)%n, b;
                if (cross) {
                    do { b = draw(&random)%n; } while (handles[b]->block==handles[a]->block);
                } else {
                    size_t start = a-handles[a]->local;
                    b = start+draw(&random)%handles[a]->block->count;
                }
                pairs[i]=(Pair){a,b};
                int expected=(a>b)-(a<b), actual;
                CHECK(lks_order_compare(order,handles[a],handles[b],&actual)==LKS_STATUS_OK && actual==expected);
                CHECK(ancestor_compare(order,handles[a],handles[b],&actual)==LKS_STATUS_OK && actual==expected);
            }
            for (int repeat=0; repeat<8; repeat++) for (int j=0; j<3; j++) {
                int kernel=(repeat+j)%3, sum=0;
                LARGE_INTEGER start,end;
                QueryPerformanceCounter(&start);
                for (int loop=0; loop<LOOPS; loop++) for (size_t i=0; i<PAIRS; i++) {
                    Pair p=pairs[i];
                    int result;
                    if (kernel==0) lks_order_compare(order,handles[p.a],handles[p.b],&result);
                    else if (kernel==1) ancestor_compare(order,handles[p.a],handles[p.b],&result);
                    else {
                        size_t a=*(const size_t *)handles[p.a]->item, b=*(const size_t *)handles[p.b]->item;
                        result=(a>b)-(a<b);
                    }
                    sum+=result;
                }
                QueryPerformanceCounter(&end);
                sink=sum;
                printf("%zu,%s,%d,%s,%d,%.3f\n",n,cross?"cross":"same",repeat,
                       kernel==0?"public":kernel==1?"ancestor":"fixed_ordinals",PAIRS*LOOPS,
                       (end.QuadPart-start.QuadPart)*1e9/frequency.QuadPart/(PAIRS*LOOPS));
            }
        }
        lks_order_destroy(order);
        free(pairs);
        free(handles);
        free(items);
    }
    return 0;
}
