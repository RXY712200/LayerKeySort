#ifndef HISTORY_HOST_H
#define HISTORY_HOST_H
/* Experiment-only platform observation; never compiled into Full itself. */
#include <stddef.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
static inline double history_host_now(void)
{LARGE_INTEGER a,b;QueryPerformanceCounter(&a);QueryPerformanceFrequency(&b);return(double)a.QuadPart/(double)b.QuadPart;}
static inline size_t history_host_rss(void)
{PROCESS_MEMORY_COUNTERS p;return GetProcessMemoryInfo(GetCurrentProcess(),&p,sizeof(p))?(size_t)p.WorkingSetSize:0;}
static inline size_t history_host_available(void)
{MEMORYSTATUSEX m;m.dwLength=sizeof(m);return GlobalMemoryStatusEx(&m)?(size_t)m.ullAvailPhys:0;}
#else
#include <time.h>
#include <unistd.h>
static inline double history_host_now(void)
{struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))return 0;return(double)t.tv_sec+(double)t.tv_nsec/1e9;}
static inline size_t history_host_rss(void)
{
    FILE *f=fopen("/proc/self/statm","r");unsigned long total,pages;int n;
    if(!f)return 0;
    n=fscanf(f,"%lu %lu",&total,&pages);fclose(f);
    return n==2?(size_t)pages*(size_t)sysconf(_SC_PAGESIZE):0;
}
static inline size_t history_host_available(void)
{
    FILE *f=fopen("/proc/meminfo","r");char line[256];unsigned long long n=0;
    if(!f)return 0;
    while(fgets(line,sizeof(line),f))if(sscanf(line,"MemAvailable: %llu kB",&n)==1)break;
    fclose(f);return(size_t)n*1024;
}
#endif
#endif
