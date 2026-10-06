/* RC gate for externally visible semantics, using only the installed API. */
#include "layerkeysort.h"
#include "distribution/consumer/v4_usage.h"
#include <string.h>
#define CHECK(x) do { if(!(x)) return __LINE__; } while(0)
int main(void)
{
    int a=1,b=2,c=3,comparison=99;LksOrder *o=lks_order_create(),*other=lks_order_create();
    const LksOrderHandle *h=NULL,*k=NULL,*foreign=NULL,*out=NULL;LksOrderCursor *cursor=NULL;
    LksSnapshot *s=NULL;unsigned char ns[]={0,255};LksSnapshotOptions options={ns,2,NULL,NULL};
    char key[64];
    {
        static const LksStatus values[]={LKS_STATUS_OK,LKS_STATUS_INVALID_ARGUMENT,
            LKS_STATUS_OUT_OF_MEMORY,LKS_STATUS_BUFFER_TOO_SMALL,LKS_STATUS_NOT_FOUND,
            LKS_STATUS_INTERNAL_ERROR,LKS_STATUS_ALREADY_EXISTS,LKS_STATUS_CAPACITY_LIMIT,
            LKS_STATUS_INVALIDATED,LKS_STATUS_REENTRANT,LKS_STATUS_DOMAIN_MISMATCH};
        size_t i;
        for(i=0;i<sizeof(values)/sizeof(values[0]);++i)
            CHECK(strcmp(lks_status_string(values[i]),"Unknown status"));
    }
    CHECK(o && other && !v4_consumer_usage());
    CHECK(!strcmp(lks_status_string((LksStatus)4),"Unknown status") &&
          !strcmp(lks_status_string((LksStatus)6),"Unknown status") &&
          !strcmp(lks_status_string((LksStatus)-1),"Unknown status"));
    CHECK(lks_order_insert_back(o,&a,&h)==LKS_STATUS_OK &&
          lks_order_insert_back(o,&b,&k)==LKS_STATUS_OK &&
          lks_order_insert_back(other,&c,&foreign)==LKS_STATUS_OK);
    CHECK(lks_order_cursor_create(o,2,&cursor)==LKS_STATUS_INVALID_ARGUMENT && !cursor);
    CHECK(lks_order_cursor_create(o,0,&cursor)==LKS_STATUS_OK);
    CHECK(lks_order_move_front(o,h)==LKS_STATUS_OK);
    CHECK(lks_order_cursor_next(cursor,&out)==LKS_STATUS_OK && out==h);
    CHECK(lks_order_compare(o,h,foreign,&comparison)==LKS_STATUS_INVALID_ARGUMENT && comparison==0);
    CHECK(lks_order_snapshot_capture(o,&options,&s)==LKS_STATUS_OK && lks_order_snapshot_is_current(o,s));
    CHECK(lks_snapshot_key_format(s,0,key,sizeof(key))==LKS_STATUS_OK && !strcmp(key,"LS1.00ff.0000000000000000"));
    CHECK(lks_order_move_front(o,k)==LKS_STATUS_OK && lks_order_item(h)==&a && lks_order_item(k)==&b);
    CHECK(!lks_order_snapshot_is_current(o,s));
    CHECK(lks_order_cursor_next(cursor,&out)==LKS_STATUS_INVALIDATED && !out);
    CHECK(lks_order_remove(o,k,NULL)==LKS_STATUS_OK && lks_order_first(o)==h);
    /* k is now expired; never dereference or test it after removal. */
    lks_order_cursor_destroy(cursor);lks_order_destroy(o);lks_order_destroy(other);
    CHECK(lks_snapshot_count(s)==2 && lks_snapshot_key_format(s,1,key,sizeof(key))==LKS_STATUS_OK);
    lks_snapshot_destroy(s);return 0;
}
