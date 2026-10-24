#include "oracle.h"
#include <stdlib.h>
#include <stdio.h>
static void require(int condition){if(!condition){fputs("oracle failure\n",stderr);exit(2);}}
void oracle_init(Oracle *o,size_t capacity)
{
    size_t i;
    o->previous=malloc(capacity*sizeof(uint32_t));o->next=malloc(capacity*sizeof(uint32_t));
    require(o->previous!=NULL&&o->next!=NULL);
    for(i=0;i<capacity;i++)o->previous[i]=o->next[i]=ABSENT;
    o->count=0;o->capacity=capacity;o->first=o->last=ABSENT;
}
void oracle_destroy(Oracle *o){free(o->previous);free(o->next);}
void oracle_insert(Oracle *o,uint32_t id,uint32_t before)
{
    uint32_t previous=before==ABSENT?o->last:o->previous[before];
    require(id<o->capacity&&o->count<o->capacity);
    o->next[id]=before;o->previous[id]=previous;
    if(previous==ABSENT)o->first=id;else o->next[previous]=id;
    if(before==ABSENT)o->last=id;else o->previous[before]=id;
    ++o->count;
}
void oracle_remove(Oracle *o,uint32_t id)
{
    uint32_t previous=o->previous[id],next=o->next[id];
    require(o->count>0&&id<o->capacity);
    if(previous==ABSENT)o->first=next;else o->next[previous]=next;
    if(next==ABSENT)o->last=previous;else o->previous[next]=previous;
    o->previous[id]=o->next[id]=ABSENT;--o->count;
}
void oracle_move(Oracle *o,uint32_t id,uint32_t before)
{
    if(id==before||o->next[id]==before)return;
    oracle_remove(o,id);oracle_insert(o,id,before);
}
uint64_t research_random(uint64_t *s)
{*s=*s*UINT64_C(6364136223846793005)+UINT64_C(1442695040888963407);return *s^(*s>>29);}
