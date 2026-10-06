/* Frozen public wire gate; no native structures or private headers in fixture. */
#include "layerkeysort.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv)
{
    size_t i,n;LksSnapshot *s=NULL;unsigned char *bytes,*out;
    if(argc<3)return 1;
    if(!strcmp(argv[1],"key"))return lks_snapshot_key_validate(argv[2])!=LKS_STATUS_OK;
    if(!strcmp(argv[1],"legacy")) {
        int item=1;LksV3Lk1ImportEntry row={argv[2],&item};LksOrder *order=NULL;
        if(lks_order_import_v3_lk1(&row,1,&order)!=LKS_STATUS_OK)return 8;
        if(lks_order_size(order)!=1 || lks_order_item(lks_order_first(order))!=&item)return 9;
        lks_order_destroy(order);return 0;
    }
    if(!strcmp(argv[1],"migration")) {
        int items[]={1,2,3};LksV3Lk1ImportEntry rows[3];LksOrder *order=NULL;const LksOrderHandle *h;
        if(argc!=5)return 2;
        for(i=0;i<3;++i){rows[i].key=argv[i+2];rows[i].item=&items[i];}
        if(lks_order_import_v3_lk1(rows,3,&order)!=LKS_STATUS_OK)return 3;
        h=lks_order_first(order);
        for(i=0;i<3;++i){if(!h || lks_order_item(h)!=&items[2-i])return 4;h=lks_order_next(h);}
        lks_order_destroy(order);return 0;
    }
    n=strlen(argv[2])/2;bytes=(unsigned char *)malloc(n);out=(unsigned char *)malloc(n);
    if(!bytes || !out)return 5;
    for(i=0;i<n;++i){unsigned x;if(sscanf(argv[2]+2*i,"%2x",&x)!=1)return 6;bytes[i]=(unsigned char)x;}
    if(lks_snapshot_deserialize(bytes,n,&s)!=LKS_STATUS_OK || lks_snapshot_serialize(s,out,n)!=LKS_STATUS_OK)return 7;
    for(i=0;i<n;++i) { printf("%02x",(unsigned)out[i]); }
    puts("");
    lks_snapshot_destroy(s);free(bytes);free(out);return 0;
}
