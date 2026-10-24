#include "oracle.h"
#include <stdio.h>
int main(void)
{
    Oracle o;char op;unsigned id,before;
    oracle_init(&o,256);
    while(scanf(" %c %u %u",&op,&id,&before)==3){
        uint32_t p;size_t count=0;
        if(op=='i')oracle_insert(&o,id,before);
        else if(op=='r')oracle_remove(&o,id);
        else oracle_move(&o,id,before);
        printf("%zu",o.count);
        for(p=o.first;p!=ABSENT&&count<=o.count;p=o.next[p],count++)printf(" %u",p);
        putchar('\n');
    }
    oracle_destroy(&o);return 0;
}
