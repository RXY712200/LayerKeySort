#ifndef WB_MEASURE_CLOCK_H
#define WB_MEASURE_CLOCK_H
#include <stdint.h>
#include <stddef.h>
uint64_t measure_now(void);
uint64_t measure_resolution(void);
uint64_t measure_empty_pair(void);
uint64_t measure_observed_step(void);
size_t measure_zero_pairs(void);
const char *measure_clock_name(void);
#endif
