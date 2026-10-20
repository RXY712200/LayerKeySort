#include <layerkeysort.h>
#include "allocation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"failure sweep line %d case=%d index=%zu: %s\n",__LINE__,mode,index,#x);exit(2);}}while(0)
static int cmp(const void *a,const void *b,void *c){(void)c;return (*(const int *)a>*(const int *)b)-(*(const int *)a<*(const int *)b);}
static LksStatus association(void *p,uint64_t row,void *ctx,const void **out,size_t *size){(void)row;(void)ctx;*out=p;*size=sizeof(int);return LKS_STATUS_OK;}
static LksStatus resolver(const void *p,size_t size,uint64_t row,void *ctx,void **out){(void)p;(void)size;*out=(int *)ctx+(size_t)row;return LKS_STATUS_OK;}
int main(void)
{
    int mode;size_t index,total=0;
    for(mode=0;mode<18;mode++){
        for(index=1;index<4096;index++){
            int values[160];void *items[160],*copy[160];const LksOrderHandle *h[160],*output=NULL,*walk;
            LksOrder *source,*result=NULL;LksSnapshot *baseline=NULL,*snapshot=NULL;LksGroup *group=NULL,*gout=NULL;LksGroupBatch *batch=NULL,*source_batch=NULL;
            LksManagedOrder *managed=NULL;LksOrderCursor *cursor=NULL;
            LksComparator comparator={cmp,NULL};LksSnapshotOptions options={"oom",3,association,NULL};
            unsigned char wire[4096];size_t bytes,i;LksStatus status=LKS_STATUS_OK;AllocationStats before,after;
            allocation_scope(1);allocation_fail(0);source=lks_order_create();CHECK(source);
            for(i=0;i<160;i++){values[i]=(int)i;items[i]=copy[i]=&values[i];CHECK(lks_order_insert_back(source,items[i],&h[i])==LKS_STATUS_OK);}
            CHECK(lks_order_snapshot_capture(source,&options,&baseline)==LKS_STATUS_OK);
            CHECK(lks_group_build(items,160,&comparator,&group)==LKS_STATUS_OK);
            CHECK(lks_group_batch_build(items,160,7,&comparator,&source_batch)==LKS_STATUS_OK);
            managed=lks_managed_order_create(&comparator);CHECK(managed);
            CHECK(lks_managed_order_insert(managed,items[0],&output)==LKS_STATUS_OK);output=NULL;
            bytes=lks_snapshot_serialized_size(baseline);CHECK(bytes<=sizeof(wire));CHECK(lks_snapshot_serialize(baseline,wire,bytes)==LKS_STATUS_OK);
            before=allocation_stats();allocation_fail(index);
            switch(mode){
            case 0:result=lks_order_create();status=result?LKS_STATUS_OK:LKS_STATUS_OUT_OF_MEMORY;break;
            case 1:status=lks_order_insert_after(source,h[70],&values[0],&output);break;
            case 2:status=lks_order_move_after(source,h[0],h[70]);break;
            case 3:status=lks_order_snapshot_capture(source,&options,&snapshot);break;
            case 4:status=lks_snapshot_deserialize(wire,bytes,&snapshot);break;
            case 5:status=lks_snapshot_restore_order(baseline,resolver,values,&result);break;
            case 6:status=lks_group_build(items,160,&comparator,&gout);break;
            case 7:status=lks_group_merge(group,group,&comparator,&gout);break;
            case 8:status=lks_group_batch_build(items,160,7,&comparator,&batch);break;
            case 9:status=lks_sort(copy,160,cmp,NULL);break;
            case 10:status=lks_managed_order_insert(managed,items[1],&output);break;
            case 11:status=lks_order_cursor_create(source,0,&cursor);break;
            case 12:{LksV3Lk1ImportEntry old[3]={{"LK1:201FF0000!",items[2]},{"LK1:1!",items[1]},{"LK1:001FF0000!",items[0]}};status=lks_order_import_v3_lk1(old,3,&result);break;}
            case 13:status=lks_order_insert_back(source,items[0],&output);break;
            case 14:status=lks_order_move_back(source,h[0]);break;
            case 15:status=lks_group_batch_merge_all(source_batch,&comparator,&gout);break;
            case 16:status=lks_order_remove(source,h[70],NULL);break;
            case 17:status=lks_order_move_after(source,h[70],h[69]);break;
            }
            after=allocation_stats();allocation_fail(0);
            if(mode==16||mode==17)CHECK(status==LKS_STATUS_OK&&after.attempts==0);
            if(status==LKS_STATUS_OUT_OF_MEMORY){
                CHECK(!result&&!snapshot&&!gout&&!batch&&!output&&!cursor);
                CHECK(before.live==after.live&&before.bytes==after.bytes);
                CHECK(lks_order_snapshot_is_current(source,baseline));
                walk=lks_order_first(source);for(i=0;i<160;i++){CHECK(walk==h[i]&&lks_order_item(walk)==items[i]);walk=lks_order_next(walk);}CHECK(!walk);
                CHECK(lks_managed_order_size(managed)==1);
                CHECK(!memcmp(copy,items,sizeof(items)));++total;
            }else CHECK(status==LKS_STATUS_OK);
            lks_order_cursor_destroy(cursor);lks_order_destroy(result);lks_snapshot_destroy(snapshot);lks_group_destroy(gout);lks_group_batch_destroy(batch);
            lks_group_batch_destroy(source_batch);lks_managed_order_destroy(managed);lks_group_destroy(group);lks_snapshot_destroy(baseline);lks_order_destroy(source);
            CHECK(allocation_stats().live==0&&allocation_stats().bytes==0);allocation_scope(0);
            if(status==LKS_STATUS_OK){printf("mode=%d failing_points=%zu then_success=true\n",mode,index-1);break;}
        }
        CHECK(index<4096);
    }
    {int value=1;LksComparator comparator={cmp,NULL};LksManagedOrder *managed;
     const LksOrderHandle *h=NULL;void *item=NULL;
     allocation_scope(1);allocation_fail(0);managed=lks_managed_order_create(&comparator);CHECK(managed);
     CHECK(lks_managed_order_insert(managed,&value,&h)==LKS_STATUS_OK);
     CHECK(lks_managed_order_remove(managed,h,&item)==LKS_STATUS_OK&&item==&value);h=NULL;
     value=2;allocation_fail(1);
     CHECK(lks_managed_order_insert(managed,&value,&h)==LKS_STATUS_OUT_OF_MEMORY&&!h&&lks_managed_order_size(managed)==0);
     allocation_fail(0);CHECK(lks_managed_order_insert(managed,&value,&h)==LKS_STATUS_OK);
     lks_managed_order_destroy(managed);CHECK(allocation_stats().live==0);allocation_scope(0);
     puts("managed remove-edit-reinsert: OOM leaves item nonresident; explicit retry recovered");}
    printf("all_sweeps failures=%zu zero_live=true\n",total);return 0;
}
