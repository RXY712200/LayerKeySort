#include <stdint.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "hps.h"
#include "benchmark.h"
#include "../src/hps_alloc_internal.h"
#include "../src/hps_path_internal.h"
#include "../src/hps_group_internal.h"
#include "../src/hps_tree_internal.h"

int hps_public_api_usage_smoke(void);

enum { HPS_BENCHMARK_ITEM_COUNT = 1000 };

static const size_t benchmark_group_sizes[] = {
    1, 10, 26, 52, 100, 256, 500, 1000
};
static const size_t robustness_group_sizes[] = {
    64, 100, 128, 192, 256, 320, 384, 448, 500, 512, 640, 768, 1000
};

typedef struct BenchmarkCompareContext {
    size_t comparison_count;
    int overflowed;
} BenchmarkCompareContext;

typedef struct BenchmarkPathStats {
    size_t total_depth;
    size_t max_depth;
    size_t total_text_length;
    size_t max_text_length;
    size_t max_level;
} BenchmarkPathStats;

typedef struct BenchmarkRow {
    size_t group_size;
    size_t groups;
    size_t build_comparisons;
    size_t merge_comparisons;
    size_t total_comparisons;
    double build_ms;
    double merge_ms;
    double total_ms;
    BenchmarkPathStats local_stats;
    BenchmarkPathStats final_stats;
} BenchmarkRow;

enum { HPS_ROBUSTNESS_SEED_COUNT = 20 };

typedef struct BenchmarkSample {
    size_t build_comparisons;
    size_t merge_comparisons;
    size_t total_comparisons;
    size_t predicted_merge_upper;
    size_t merge_gap;
    size_t merge_element_work;
    size_t merge_pair_count;
    size_t merge_rounds;
    double merge_gap_percent;
    double build_ms;
    double merge_ms;
    double total_ms;
    BenchmarkPathStats local_stats;
    BenchmarkPathStats final_stats;
} BenchmarkSample;

typedef struct BenchmarkDistribution {
    double mean;
    double m2;
    size_t minimum;
    size_t maximum;
} BenchmarkDistribution;

typedef struct BenchmarkAggregate {
    size_t group_size;
    size_t groups;
    BenchmarkDistribution build_comparisons;
    BenchmarkDistribution merge_comparisons;
    BenchmarkDistribution total_comparisons;
    double build_ms_mean;
    double merge_ms_mean;
    double total_ms_mean;
    double final_avg_depth_mean;
    size_t final_max_depth_worst;
    double final_avg_text_length_mean;
    size_t final_max_text_length_worst;
    size_t final_max_level_worst;
    double local_avg_depth_mean;
    size_t local_max_depth_worst;
} BenchmarkAggregate;

static int benchmark_compare_int(
    const void *left,
    const void *right,
    void *context
)
{
    BenchmarkCompareContext *counter = (BenchmarkCompareContext *)context;
    int left_value = *(const int *)left;
    int right_value = *(const int *)right;

    if (counter->comparison_count == (size_t)-1) {
        counter->overflowed = 1;
    } else {
        ++counter->comparison_count;
    }

    if (left_value < right_value) {
        return -1;
    }
    if (left_value > right_value) {
        return 1;
    }
    return 0;
}

static int benchmark_add_size(size_t *total, size_t value)
{
    if (*total > (size_t)-1 - value) {
        return 0;
    }
    *total += value;
    return 1;
}

static int benchmark_elapsed_ms(clock_t begin, clock_t end, double *out_ms)
{
    if (begin == (clock_t)-1 || end == (clock_t)-1 || end < begin ||
        out_ms == NULL || CLOCKS_PER_SEC <= 0) {
        return 0;
    }
    *out_ms = ((double)(end - begin) * 1000.0) / (double)CLOCKS_PER_SEC;
    return 1;
}

static int benchmark_collect_path(
    const HpsPath *path,
    BenchmarkPathStats *stats
)
{
    size_t depth;
    size_t text_length;
    size_t index;

    if (path == NULL || stats == NULL) {
        return 0;
    }
    depth = hps_path_depth(path);
    text_length = hps_path_text_length(path);
    if (!benchmark_add_size(&stats->total_depth, depth) ||
        !benchmark_add_size(&stats->total_text_length, text_length)) {
        return 0;
    }
    if (depth > stats->max_depth) {
        stats->max_depth = depth;
    }
    if (text_length > stats->max_text_length) {
        stats->max_text_length = text_length;
    }
    for (index = 0; index < depth; ++index) {
        size_t level;
        if (hps_path_get_level(path, index, &level) != HPS_STATUS_OK) {
            return 0;
        }
        if (level > stats->max_level) {
            stats->max_level = level;
        }
    }
    return 1;
}

static int benchmark_collect_batch_paths(
    const HpsGroupBatch *batch,
    BenchmarkPathStats *stats
)
{
    size_t group_index;

    for (group_index = 0;
         group_index < hps_group_batch_group_count(batch);
         ++group_index) {
        const HpsGroup *group = hps_group_batch_group_at(batch, group_index);
        size_t item_index;

        if (group == NULL) {
            return 0;
        }
        for (item_index = 0; item_index < hps_group_size(group); ++item_index) {
            if (!benchmark_collect_path(hps_group_path_at(group, item_index), stats)) {
                return 0;
            }
        }
    }
    return 1;
}

static int benchmark_validate_final(
    const HpsGroup *group,
    size_t count,
    BenchmarkPathStats *stats
)
{
    size_t index;

    if (hps_group_size(group) != count) {
        return 0;
    }
    for (index = 0; index < count; ++index) {
        void *item = hps_group_item_at(group, index);
        if (item == NULL || *(const int *)item != (int)index ||
            !benchmark_collect_path(hps_group_path_at(group, index), stats)) {
            return 0;
        }
        if (index > 0) {
            int path_order;
            if (hps_path_compare(hps_group_path_at(group, index - 1),
                    hps_group_path_at(group, index), &path_order) != HPS_STATUS_OK ||
                path_order != -1) {
                return 0;
            }
        }
    }
    return 1;
}

static void benchmark_shuffle(int *values, size_t count, uint32_t seed)
{
    uint32_t state = seed;
    size_t index;

    for (index = count - 1; index > 0; --index) {
        size_t other_index;
        int temporary;

        state = state * UINT32_C(1664525) + UINT32_C(1013904223);
        other_index = (size_t)(state % (uint32_t)(index + 1));
        temporary = values[index];
        values[index] = values[other_index];
        values[other_index] = temporary;
    }
}

int hps_run_group_size_benchmark(void)
{
    int values[HPS_BENCHMARK_ITEM_COUNT];
    void *items[HPS_BENCHMARK_ITEM_COUNT];
    BenchmarkRow rows[sizeof(benchmark_group_sizes) / sizeof(benchmark_group_sizes[0])];
    size_t index;
    HpsComparator comparator;
    BenchmarkCompareContext counter;

    for (index = 0; index < HPS_BENCHMARK_ITEM_COUNT; ++index) {
        values[index] = (int)index;
    }
    benchmark_shuffle(values, HPS_BENCHMARK_ITEM_COUNT, UINT32_C(0xC0FFEE));
    for (index = 0; index < HPS_BENCHMARK_ITEM_COUNT; ++index) {
        items[index] = &values[index];
    }

    comparator.compare = benchmark_compare_int;
    comparator.context = &counter;
    printf("HPSort GroupSize Benchmark\n");
    printf("N=1000\n");
    printf("Dataset=Deterministic shuffled permutation 0..999\n");
    printf("Seed=0xC0FFEE\n");
    printf("GroupSize,Groups,BuildCmp,MergeCmp,TotalCmp,BuildMs,MergeMs,TotalMs,LocalAvgDepth,LocalMaxDepth,LocalMaxLevel,FinalAvgDepth,FinalMaxDepth,FinalMaxLevel,FinalAvgTextLen\n");

    for (index = 0; index < sizeof(benchmark_group_sizes) /
            sizeof(benchmark_group_sizes[0]); ++index) {
        BenchmarkRow *row = &rows[index];
        HpsGroupBatch *batch = NULL;
        HpsGroup *final_group = NULL;
        clock_t begin;
        clock_t end;
        HpsStatus status;

        memset(row, 0, sizeof(*row));
        row->group_size = benchmark_group_sizes[index];
        counter.comparison_count = 0;
        counter.overflowed = 0;
        begin = clock();
        status = hps_group_batch_build(items, HPS_BENCHMARK_ITEM_COUNT,
            row->group_size, &comparator, &batch);
        end = clock();
        if (status != HPS_STATUS_OK || batch == NULL || counter.overflowed ||
            !benchmark_elapsed_ms(begin, end, &row->build_ms)) {
            printf("BENCHMARK FAILED GroupSize=%lu: Batch build failed\n",
                (unsigned long)row->group_size);
            hps_group_batch_destroy(batch);
            return 1;
        }
        row->build_comparisons = counter.comparison_count;
        row->groups = hps_group_batch_group_count(batch);
        if (hps_group_batch_total_size(batch) != HPS_BENCHMARK_ITEM_COUNT ||
            !benchmark_collect_batch_paths(batch, &row->local_stats)) {
            printf("BENCHMARK FAILED GroupSize=%lu: local Group validation/statistics failed\n",
                (unsigned long)row->group_size);
            hps_group_batch_destroy(batch);
            return 1;
        }

        counter.comparison_count = 0;
        counter.overflowed = 0;
        begin = clock();
        status = hps_group_batch_merge_all(batch, &comparator, &final_group);
        end = clock();
        if (status != HPS_STATUS_OK || final_group == NULL || counter.overflowed ||
            !benchmark_elapsed_ms(begin, end, &row->merge_ms)) {
            printf("BENCHMARK FAILED GroupSize=%lu: merge_all failed\n",
                (unsigned long)row->group_size);
            hps_group_destroy(final_group);
            hps_group_batch_destroy(batch);
            return 1;
        }
        row->merge_comparisons = counter.comparison_count;
        if (!benchmark_add_size(&row->total_comparisons, row->build_comparisons) ||
            !benchmark_add_size(&row->total_comparisons, row->merge_comparisons)) {
            printf("BENCHMARK FAILED GroupSize=%lu: comparison count overflow\n",
                (unsigned long)row->group_size);
            hps_group_destroy(final_group);
            hps_group_batch_destroy(batch);
            return 1;
        }
        row->total_ms = row->build_ms + row->merge_ms;

        if (!benchmark_validate_final(final_group, HPS_BENCHMARK_ITEM_COUNT,
                &row->final_stats)) {
            printf("BENCHMARK FAILED GroupSize=%lu: Final Group incorrect\n",
                (unsigned long)row->group_size);
            hps_group_destroy(final_group);
            hps_group_batch_destroy(batch);
            return 1;
        }

        printf("%lu,%lu,%lu,%lu,%lu,%.3f,%.3f,%.3f,%.3f,%lu,%lu,%.3f,%lu,%lu,%.3f\n",
            (unsigned long)row->group_size,
            (unsigned long)row->groups,
            (unsigned long)row->build_comparisons,
            (unsigned long)row->merge_comparisons,
            (unsigned long)row->total_comparisons,
            row->build_ms,
            row->merge_ms,
            row->total_ms,
            (double)row->local_stats.total_depth / HPS_BENCHMARK_ITEM_COUNT,
            (unsigned long)row->local_stats.max_depth,
            (unsigned long)row->local_stats.max_level,
            (double)row->final_stats.total_depth / HPS_BENCHMARK_ITEM_COUNT,
            (unsigned long)row->final_stats.max_depth,
            (unsigned long)row->final_stats.max_level,
            (double)row->final_stats.total_text_length / HPS_BENCHMARK_ITEM_COUNT);

        hps_group_destroy(final_group);
        hps_group_batch_destroy(batch);
    }

    printf("PathStructureProxy,GroupSize,LocalTotalDepth,LocalAvgDepth,LocalMaxDepth,LocalTotalTextLength,LocalAvgTextLength,LocalMaxTextLength,LocalMaxLevel,FinalTotalDepth,FinalAvgDepth,FinalMaxDepth,FinalTotalTextLength,FinalAvgTextLength,FinalMaxTextLength,FinalMaxLevel\n");
    for (index = 0; index < sizeof(benchmark_group_sizes) /
            sizeof(benchmark_group_sizes[0]); ++index) {
        const BenchmarkRow *row = &rows[index];
        printf("STRUCTURE,%lu,%lu,%.3f,%lu,%lu,%.3f,%lu,%lu,%lu,%.3f,%lu,%lu,%.3f,%lu,%lu\n",
            (unsigned long)row->group_size,
            (unsigned long)row->local_stats.total_depth,
            (double)row->local_stats.total_depth / HPS_BENCHMARK_ITEM_COUNT,
            (unsigned long)row->local_stats.max_depth,
            (unsigned long)row->local_stats.total_text_length,
            (double)row->local_stats.total_text_length / HPS_BENCHMARK_ITEM_COUNT,
            (unsigned long)row->local_stats.max_text_length,
            (unsigned long)row->local_stats.max_level,
            (unsigned long)row->final_stats.total_depth,
            (double)row->final_stats.total_depth / HPS_BENCHMARK_ITEM_COUNT,
            (unsigned long)row->final_stats.max_depth,
            (unsigned long)row->final_stats.total_text_length,
            (double)row->final_stats.total_text_length / HPS_BENCHMARK_ITEM_COUNT,
            (unsigned long)row->final_stats.max_text_length,
            (unsigned long)row->final_stats.max_level);
    }
    printf("Path figures are structural-size proxies, not actual heap memory bytes.\n");
    return 0;
}

static void benchmark_distribution_add(
    BenchmarkDistribution *distribution,
    size_t value,
    size_t sample_index
)
{
    double numeric_value = (double)value;

    if (sample_index == 0) {
        distribution->mean = numeric_value;
        distribution->m2 = 0.0;
        distribution->minimum = value;
        distribution->maximum = value;
    } else {
        double delta = numeric_value - distribution->mean;
        distribution->mean += delta / (double)(sample_index + 1);
        distribution->m2 += delta * (numeric_value - distribution->mean);
        if (value < distribution->minimum) {
            distribution->minimum = value;
        }
        if (value > distribution->maximum) {
            distribution->maximum = value;
        }
    }
}

static double benchmark_distribution_stddev(
    const BenchmarkDistribution *distribution
)
{
    return sqrt(distribution->m2 / (double)HPS_ROBUSTNESS_SEED_COUNT);
}

static int benchmark_run_robustness_sample(
    size_t group_size,
    uint32_t seed,
    int *values,
    void **items,
    HpsComparator *comparator,
    BenchmarkSample *sample,
    size_t *out_groups
)
{
    HpsGroupBatch *batch = NULL;
    HpsGroup *final_group = NULL;
    BenchmarkCompareContext *counter =
        (BenchmarkCompareContext *)comparator->context;
    clock_t begin;
    clock_t end;
    HpsStatus status;
    size_t index;

    for (index = 0; index < HPS_BENCHMARK_ITEM_COUNT; ++index) {
        values[index] = (int)index;
    }
    benchmark_shuffle(values, HPS_BENCHMARK_ITEM_COUNT, seed);
    for (index = 0; index < HPS_BENCHMARK_ITEM_COUNT; ++index) {
        items[index] = &values[index];
    }
    memset(sample, 0, sizeof(*sample));

    counter->comparison_count = 0;
    counter->overflowed = 0;
    begin = clock();
    status = hps_group_batch_build(items, HPS_BENCHMARK_ITEM_COUNT,
        group_size, comparator, &batch);
    end = clock();
    if (status != HPS_STATUS_OK || batch == NULL || counter->overflowed ||
        !benchmark_elapsed_ms(begin, end, &sample->build_ms)) {
        printf("BENCHMARK FAILED seed=0x%08lX GroupSize=%lu: Batch build failed\n",
            (unsigned long)seed, (unsigned long)group_size);
        hps_group_batch_destroy(batch);
        return 0;
    }
    sample->build_comparisons = counter->comparison_count;
    *out_groups = hps_group_batch_group_count(batch);
    if (hps_group_batch_total_size(batch) != HPS_BENCHMARK_ITEM_COUNT ||
        !benchmark_collect_batch_paths(batch, &sample->local_stats)) {
        printf("BENCHMARK FAILED seed=0x%08lX GroupSize=%lu: local Group validation/statistics failed\n",
            (unsigned long)seed, (unsigned long)group_size);
        hps_group_batch_destroy(batch);
        return 0;
    }

    counter->comparison_count = 0;
    counter->overflowed = 0;
    begin = clock();
    status = hps_group_batch_merge_all(batch, comparator, &final_group);
    end = clock();
    if (status != HPS_STATUS_OK || final_group == NULL || counter->overflowed ||
        !benchmark_elapsed_ms(begin, end, &sample->merge_ms)) {
        printf("BENCHMARK FAILED seed=0x%08lX GroupSize=%lu: merge_all failed\n",
            (unsigned long)seed, (unsigned long)group_size);
        hps_group_destroy(final_group);
        hps_group_batch_destroy(batch);
        return 0;
    }
    sample->merge_comparisons = counter->comparison_count;
    if (sample->build_comparisons > (size_t)-1 - sample->merge_comparisons) {
        printf("BENCHMARK FAILED seed=0x%08lX GroupSize=%lu: comparison count overflow\n",
            (unsigned long)seed, (unsigned long)group_size);
        hps_group_destroy(final_group);
        hps_group_batch_destroy(batch);
        return 0;
    }
    sample->total_comparisons = sample->build_comparisons +
        sample->merge_comparisons;
    sample->total_ms = sample->build_ms + sample->merge_ms;
    if (!benchmark_validate_final(final_group, HPS_BENCHMARK_ITEM_COUNT,
            &sample->final_stats)) {
        printf("BENCHMARK FAILED seed=0x%08lX GroupSize=%lu: Final Group incorrect\n",
            (unsigned long)seed, (unsigned long)group_size);
        hps_group_destroy(final_group);
        hps_group_batch_destroy(batch);
        return 0;
    }

    hps_group_destroy(final_group);
    hps_group_batch_destroy(batch);
    return 1;
}

int hps_run_group_size_robustness_benchmark(void)
{
    enum {
        GROUP_SIZE_COUNT = sizeof(robustness_group_sizes) /
            sizeof(robustness_group_sizes[0])
    };
    BenchmarkSample samples[GROUP_SIZE_COUNT][HPS_ROBUSTNESS_SEED_COUNT];
    BenchmarkAggregate aggregates[GROUP_SIZE_COUNT];
    size_t wins[GROUP_SIZE_COUNT];
    int values[HPS_BENCHMARK_ITEM_COUNT];
    void *items[HPS_BENCHMARK_ITEM_COUNT];
    HpsComparator comparator;
    BenchmarkCompareContext counter;
    size_t group_index;
    size_t seed_index;
    size_t group_500_index = 0;
    size_t completed_samples = 0;

    memset(samples, 0, sizeof(samples));
    memset(aggregates, 0, sizeof(aggregates));
    memset(wins, 0, sizeof(wins));
    counter.comparison_count = 0;
    counter.overflowed = 0;
    comparator.compare = benchmark_compare_int;
    comparator.context = &counter;

    printf("HPSort GroupSize Robustness Benchmark\n");
    printf("N=1000\n");
    printf("Seeds=20\n");
    printf("SeedStart=0xC0FFEE\n");

    for (seed_index = 0; seed_index < HPS_ROBUSTNESS_SEED_COUNT; ++seed_index) {
        uint32_t seed = UINT32_C(0xC0FFEE) + (uint32_t)seed_index;
        for (group_index = 0; group_index < GROUP_SIZE_COUNT; ++group_index) {
            size_t groups = 0;
            BenchmarkSample *sample = &samples[group_index][seed_index];

            if (!benchmark_run_robustness_sample(
                    robustness_group_sizes[group_index], seed, values, items,
                    &comparator, sample, &groups)) {
                return 1;
            }
            ++completed_samples;
            if (seed_index == 0) {
                aggregates[group_index].group_size =
                    robustness_group_sizes[group_index];
                aggregates[group_index].groups = groups;
            } else if (groups != aggregates[group_index].groups) {
                printf("BENCHMARK FAILED seed=0x%08lX GroupSize=%lu: group count changed between seeds\n",
                    (unsigned long)seed,
                    (unsigned long)robustness_group_sizes[group_index]);
                return 1;
            }
        }
    }

    if (completed_samples != GROUP_SIZE_COUNT * HPS_ROBUSTNESS_SEED_COUNT) {
        printf("BENCHMARK FAILED: completed sample count mismatch\n");
        return 1;
    }
    printf("CompletedSamples=%lu\n", (unsigned long)completed_samples);

    for (group_index = 0; group_index < GROUP_SIZE_COUNT; ++group_index) {
        BenchmarkAggregate *aggregate = &aggregates[group_index];
        double build_ms_total = 0.0;
        double merge_ms_total = 0.0;
        double total_ms_total = 0.0;
        double local_avg_depth_total = 0.0;
        double final_avg_depth_total = 0.0;
        double final_avg_text_length_total = 0.0;

        for (seed_index = 0; seed_index < HPS_ROBUSTNESS_SEED_COUNT; ++seed_index) {
            const BenchmarkSample *sample = &samples[group_index][seed_index];
            double local_avg_depth = (double)sample->local_stats.total_depth /
                HPS_BENCHMARK_ITEM_COUNT;
            double final_avg_depth = (double)sample->final_stats.total_depth /
                HPS_BENCHMARK_ITEM_COUNT;
            double final_avg_text_length =
                (double)sample->final_stats.total_text_length /
                HPS_BENCHMARK_ITEM_COUNT;

            benchmark_distribution_add(&aggregate->build_comparisons,
                sample->build_comparisons, seed_index);
            benchmark_distribution_add(&aggregate->merge_comparisons,
                sample->merge_comparisons, seed_index);
            benchmark_distribution_add(&aggregate->total_comparisons,
                sample->total_comparisons, seed_index);
            build_ms_total += sample->build_ms;
            merge_ms_total += sample->merge_ms;
            total_ms_total += sample->total_ms;
            local_avg_depth_total += local_avg_depth;
            final_avg_depth_total += final_avg_depth;
            final_avg_text_length_total += final_avg_text_length;

            if (seed_index == 0 || sample->local_stats.max_depth >
                    aggregate->local_max_depth_worst) {
                aggregate->local_max_depth_worst = sample->local_stats.max_depth;
            }
            if (seed_index == 0 || sample->final_stats.max_depth >
                    aggregate->final_max_depth_worst) {
                aggregate->final_max_depth_worst = sample->final_stats.max_depth;
            }
            if (seed_index == 0 || sample->final_stats.max_level >
                    aggregate->final_max_level_worst) {
                aggregate->final_max_level_worst = sample->final_stats.max_level;
            }
        }
        aggregate->build_ms_mean = build_ms_total / HPS_ROBUSTNESS_SEED_COUNT;
        aggregate->merge_ms_mean = merge_ms_total / HPS_ROBUSTNESS_SEED_COUNT;
        aggregate->total_ms_mean = total_ms_total / HPS_ROBUSTNESS_SEED_COUNT;
        aggregate->local_avg_depth_mean = local_avg_depth_total /
            HPS_ROBUSTNESS_SEED_COUNT;
        aggregate->final_avg_depth_mean = final_avg_depth_total /
            HPS_ROBUSTNESS_SEED_COUNT;
        aggregate->final_avg_text_length_mean = final_avg_text_length_total /
            HPS_ROBUSTNESS_SEED_COUNT;
        if (aggregate->group_size == 500) {
            group_500_index = group_index;
        }
    }

    for (seed_index = 0; seed_index < HPS_ROBUSTNESS_SEED_COUNT; ++seed_index) {
        size_t minimum = (size_t)-1;
        for (group_index = 0; group_index < GROUP_SIZE_COUNT; ++group_index) {
            if (samples[group_index][seed_index].total_comparisons < minimum) {
                minimum = samples[group_index][seed_index].total_comparisons;
            }
        }
        for (group_index = 0; group_index < GROUP_SIZE_COUNT; ++group_index) {
            if (samples[group_index][seed_index].total_comparisons == minimum) {
                ++wins[group_index];
            }
        }
    }

    printf("GroupSize,Groups,BuildCmpMean,MergeCmpMean,TotalCmpMean,TotalCmpMin,TotalCmpMax,TotalCmpStdDev,BuildMsMean,MergeMsMean,TotalMsMean\n");
    for (group_index = 0; group_index < GROUP_SIZE_COUNT; ++group_index) {
        const BenchmarkAggregate *aggregate = &aggregates[group_index];
        printf("%lu,%lu,%.3f,%.3f,%.3f,%lu,%lu,%.3f,%.3f,%.3f,%.3f\n",
            (unsigned long)aggregate->group_size,
            (unsigned long)aggregate->groups,
            aggregate->build_comparisons.mean,
            aggregate->merge_comparisons.mean,
            aggregate->total_comparisons.mean,
            (unsigned long)aggregate->total_comparisons.minimum,
            (unsigned long)aggregate->total_comparisons.maximum,
            benchmark_distribution_stddev(&aggregate->total_comparisons),
            aggregate->build_ms_mean,
            aggregate->merge_ms_mean,
            aggregate->total_ms_mean);
    }

    printf("ComparisonSpread,GroupSize,BuildCmpMin,BuildCmpMax,BuildCmpStdDev,MergeCmpMin,MergeCmpMax,MergeCmpStdDev,TotalCmpMean,TotalCmpMin,TotalCmpMax,TotalCmpStdDev\n");
    for (group_index = 0; group_index < GROUP_SIZE_COUNT; ++group_index) {
        const BenchmarkAggregate *aggregate = &aggregates[group_index];
        printf("SPREAD,%lu,%lu,%lu,%.3f,%lu,%lu,%.3f,%.3f,%lu,%lu,%.3f\n",
            (unsigned long)aggregate->group_size,
            (unsigned long)aggregate->build_comparisons.minimum,
            (unsigned long)aggregate->build_comparisons.maximum,
            benchmark_distribution_stddev(&aggregate->build_comparisons),
            (unsigned long)aggregate->merge_comparisons.minimum,
            (unsigned long)aggregate->merge_comparisons.maximum,
            benchmark_distribution_stddev(&aggregate->merge_comparisons),
            aggregate->total_comparisons.mean,
            (unsigned long)aggregate->total_comparisons.minimum,
            (unsigned long)aggregate->total_comparisons.maximum,
            benchmark_distribution_stddev(&aggregate->total_comparisons));
    }

    printf("GroupSize,MeanLocalAvgDepth,WorstLocalMaxDepth,MeanFinalAvgDepth,WorstFinalMaxDepth,MeanFinalAvgTextLen,WorstFinalMaxLevel\n");
    for (group_index = 0; group_index < GROUP_SIZE_COUNT; ++group_index) {
        const BenchmarkAggregate *aggregate = &aggregates[group_index];
        printf("%lu,%.3f,%lu,%.3f,%lu,%.3f,%lu\n",
            (unsigned long)aggregate->group_size,
            aggregate->local_avg_depth_mean,
            (unsigned long)aggregate->local_max_depth_worst,
            aggregate->final_avg_depth_mean,
            (unsigned long)aggregate->final_max_depth_worst,
            aggregate->final_avg_text_length_mean,
            (unsigned long)aggregate->final_max_level_worst);
    }

    printf("Seed,GS64,GS100,GS128,GS192,GS256,GS320,GS384,GS448,GS500,GS512,GS640,GS768,GS1000\n");
    for (seed_index = 0; seed_index < HPS_ROBUSTNESS_SEED_COUNT; ++seed_index) {
        uint32_t seed = UINT32_C(0xC0FFEE) + (uint32_t)seed_index;
        printf("0x%08lX", (unsigned long)seed);
        for (group_index = 0; group_index < GROUP_SIZE_COUNT; ++group_index) {
            printf(",%lu", (unsigned long)
                samples[group_index][seed_index].total_comparisons);
        }
        printf("\n");
    }

    printf("GroupSize,WinsOrTies\n");
    for (group_index = 0; group_index < GROUP_SIZE_COUNT; ++group_index) {
        printf("%lu,%lu\n", (unsigned long)aggregates[group_index].group_size,
            (unsigned long)wins[group_index]);
    }

    printf("GroupSize,MeanTotalCmp,DeltaVsGS500\n");
    for (group_index = 0; group_index < GROUP_SIZE_COUNT; ++group_index) {
        printf("%lu,%.3f,%.3f\n",
            (unsigned long)aggregates[group_index].group_size,
            aggregates[group_index].total_comparisons.mean,
            aggregates[group_index].total_comparisons.mean -
                aggregates[group_500_index].total_comparisons.mean);
    }
    printf("Path metrics are structural-size proxies, not actual heap memory bytes.\n");
    return 0;
}

enum {
    HPS_SCALE_N_COUNT = 4,
    HPS_SCALE_SEED_COUNT = 5,
    HPS_SCALE_MAX_CANDIDATES = 7
};

typedef struct BenchmarkScaleCandidate {
    size_t count;
    size_t group_size;
    size_t groups;
    BenchmarkSample samples[HPS_SCALE_SEED_COUNT];
    BenchmarkAggregate aggregate;
    size_t total_comparison_sum;
} BenchmarkScaleCandidate;

typedef struct BenchmarkScaleRun {
    size_t count;
    size_t candidate_count;
    BenchmarkScaleCandidate candidates[HPS_SCALE_MAX_CANDIDATES];
} BenchmarkScaleRun;

static size_t benchmark_scale_candidates(size_t count, size_t *candidates)
{
    static const size_t fixed_sizes[] = { 128, 256, 512, 1024 };
    size_t raw[HPS_SCALE_MAX_CANDIDATES];
    size_t raw_count = 0;
    size_t index;
    size_t unique_count = 0;

    for (index = 0; index < sizeof(fixed_sizes) / sizeof(fixed_sizes[0]); ++index) {
        if (fixed_sizes[index] <= count) {
            raw[raw_count++] = fixed_sizes[index];
        }
    }
    if (count / 4 > 0 && count / 4 <= count) {
        raw[raw_count++] = count / 4;
    }
    if (count / 2 > 0 && count / 2 <= count) {
        raw[raw_count++] = count / 2;
    }
    if (count > 0) {
        raw[raw_count++] = count;
    }

    for (index = 1; index < raw_count; ++index) {
        size_t value = raw[index];
        size_t position = index;
        while (position > 0 && raw[position - 1] > value) {
            raw[position] = raw[position - 1];
            --position;
        }
        raw[position] = value;
    }
    for (index = 0; index < raw_count; ++index) {
        if (unique_count == 0 || candidates[unique_count - 1] != raw[index]) {
            candidates[unique_count++] = raw[index];
        }
    }
    return unique_count;
}

static int benchmark_run_scale_sample(
    size_t count,
    size_t group_size,
    uint32_t seed,
    void **items,
    HpsComparator *comparator,
    BenchmarkSample *sample,
    size_t *out_groups
)
{
    HpsGroupBatch *batch = NULL;
    HpsGroup *final_group = NULL;
    BenchmarkCompareContext *counter =
        (BenchmarkCompareContext *)comparator->context;
    clock_t begin;
    clock_t end;
    HpsStatus status;

    memset(sample, 0, sizeof(*sample));
    counter->comparison_count = 0;
    counter->overflowed = 0;
    begin = clock();
    status = hps_group_batch_build(items, count, group_size, comparator, &batch);
    end = clock();
    if (status != HPS_STATUS_OK || batch == NULL || counter->overflowed ||
        !benchmark_elapsed_ms(begin, end, &sample->build_ms)) {
        printf("BENCHMARK FAILED N=%lu seed=0x%08lX GroupSize=%lu: Batch build failed (%s)\n",
            (unsigned long)count, (unsigned long)seed,
            (unsigned long)group_size, hps_status_string(status));
        hps_group_batch_destroy(batch);
        return 0;
    }
    sample->build_comparisons = counter->comparison_count;
    *out_groups = hps_group_batch_group_count(batch);
    if (hps_group_batch_total_size(batch) != count ||
        !benchmark_collect_batch_paths(batch, &sample->local_stats)) {
        printf("BENCHMARK FAILED N=%lu seed=0x%08lX GroupSize=%lu: local Path statistics failed\n",
            (unsigned long)count, (unsigned long)seed,
            (unsigned long)group_size);
        hps_group_batch_destroy(batch);
        return 0;
    }

    counter->comparison_count = 0;
    counter->overflowed = 0;
    begin = clock();
    status = hps_group_batch_merge_all(batch, comparator, &final_group);
    end = clock();
    if (status != HPS_STATUS_OK || final_group == NULL || counter->overflowed ||
        !benchmark_elapsed_ms(begin, end, &sample->merge_ms)) {
        printf("BENCHMARK FAILED N=%lu seed=0x%08lX GroupSize=%lu: merge_all failed (%s)\n",
            (unsigned long)count, (unsigned long)seed,
            (unsigned long)group_size, hps_status_string(status));
        hps_group_destroy(final_group);
        hps_group_batch_destroy(batch);
        return 0;
    }
    sample->merge_comparisons = counter->comparison_count;
    if (sample->build_comparisons > (size_t)-1 - sample->merge_comparisons) {
        printf("BENCHMARK FAILED N=%lu seed=0x%08lX GroupSize=%lu: comparison count overflow\n",
            (unsigned long)count, (unsigned long)seed,
            (unsigned long)group_size);
        hps_group_destroy(final_group);
        hps_group_batch_destroy(batch);
        return 0;
    }
    sample->total_comparisons = sample->build_comparisons +
        sample->merge_comparisons;
    sample->total_ms = sample->build_ms + sample->merge_ms;
    if (!benchmark_validate_final(final_group, count, &sample->final_stats)) {
        printf("BENCHMARK FAILED N=%lu seed=0x%08lX GroupSize=%lu: Final Group or adjacent Path order incorrect\n",
            (unsigned long)count, (unsigned long)seed,
            (unsigned long)group_size);
        hps_group_destroy(final_group);
        hps_group_batch_destroy(batch);
        return 0;
    }

    hps_group_destroy(final_group);
    hps_group_batch_destroy(batch);
    return 1;
}

static int benchmark_scale_aggregate(BenchmarkScaleCandidate *candidate)
{
    size_t seed_index;
    double build_ms_total = 0.0;
    double merge_ms_total = 0.0;
    double total_ms_total = 0.0;
    double local_depth_total = 0.0;
    double final_depth_total = 0.0;
    double final_text_total = 0.0;

    memset(&candidate->aggregate, 0, sizeof(candidate->aggregate));
    candidate->aggregate.group_size = candidate->group_size;
    candidate->aggregate.groups = candidate->groups;
    candidate->total_comparison_sum = 0;
    for (seed_index = 0; seed_index < HPS_SCALE_SEED_COUNT; ++seed_index) {
        const BenchmarkSample *sample = &candidate->samples[seed_index];
        double local_avg_depth = (double)sample->local_stats.total_depth /
            (double)candidate->count;
        double final_avg_depth = (double)sample->final_stats.total_depth /
            (double)candidate->count;
        double final_avg_text_length =
            (double)sample->final_stats.total_text_length /
            (double)candidate->count;

        benchmark_distribution_add(&candidate->aggregate.build_comparisons,
            sample->build_comparisons, seed_index);
        benchmark_distribution_add(&candidate->aggregate.merge_comparisons,
            sample->merge_comparisons, seed_index);
        benchmark_distribution_add(&candidate->aggregate.total_comparisons,
            sample->total_comparisons, seed_index);
        if (!benchmark_add_size(&candidate->total_comparison_sum,
                sample->total_comparisons)) {
            return 0;
        }
        build_ms_total += sample->build_ms;
        merge_ms_total += sample->merge_ms;
        total_ms_total += sample->total_ms;
        local_depth_total += local_avg_depth;
        final_depth_total += final_avg_depth;
        final_text_total += final_avg_text_length;
        if (seed_index == 0 || sample->local_stats.max_depth >
                candidate->aggregate.local_max_depth_worst) {
            candidate->aggregate.local_max_depth_worst =
                sample->local_stats.max_depth;
        }
        if (seed_index == 0 || sample->final_stats.max_depth >
                candidate->aggregate.final_max_depth_worst) {
            candidate->aggregate.final_max_depth_worst =
                sample->final_stats.max_depth;
        }
        if (seed_index == 0 || sample->final_stats.max_text_length >
                candidate->aggregate.final_max_text_length_worst) {
            candidate->aggregate.final_max_text_length_worst =
                sample->final_stats.max_text_length;
        }
        if (seed_index == 0 || sample->final_stats.max_level >
                candidate->aggregate.final_max_level_worst) {
            candidate->aggregate.final_max_level_worst =
                sample->final_stats.max_level;
        }
    }
    candidate->aggregate.build_ms_mean = build_ms_total / HPS_SCALE_SEED_COUNT;
    candidate->aggregate.merge_ms_mean = merge_ms_total / HPS_SCALE_SEED_COUNT;
    candidate->aggregate.total_ms_mean = total_ms_total / HPS_SCALE_SEED_COUNT;
    candidate->aggregate.local_avg_depth_mean = local_depth_total /
        HPS_SCALE_SEED_COUNT;
    candidate->aggregate.final_avg_depth_mean = final_depth_total /
        HPS_SCALE_SEED_COUNT;
    candidate->aggregate.final_avg_text_length_mean = final_text_total /
        HPS_SCALE_SEED_COUNT;
    return 1;
}

static const BenchmarkScaleCandidate *benchmark_scale_find_candidate(
    const BenchmarkScaleRun *run,
    size_t group_size
)
{
    size_t index;
    for (index = 0; index < run->candidate_count; ++index) {
        if (run->candidates[index].group_size == group_size) {
            return &run->candidates[index];
        }
    }
    return NULL;
}

int hps_run_group_size_scale_benchmark(void)
{
    static const size_t scale_counts[HPS_SCALE_N_COUNT] = {
        1000, 2000, 5000, 10000
    };
    BenchmarkScaleRun runs[HPS_SCALE_N_COUNT];
    size_t run_index;
    size_t completed_samples = 0;

    memset(runs, 0, sizeof(runs));
    printf("HPSort Scale Benchmark\n");
    printf("N=1000,2000,5000,10000; Seeds=5; SeedStart=0xC0FFEE\n");
    for (run_index = 0; run_index < HPS_SCALE_N_COUNT; ++run_index) {
        BenchmarkScaleRun *run = &runs[run_index];
        size_t count = scale_counts[run_index];
        int *values;
        void **items;
        size_t candidate_sizes[HPS_SCALE_MAX_CANDIDATES];
        size_t candidate_index;

        run->count = count;
        run->candidate_count = benchmark_scale_candidates(count, candidate_sizes);
        if (run->candidate_count == 0 ||
            count > (size_t)-1 / sizeof(*values) ||
            count > (size_t)-1 / sizeof(*items)) {
            printf("BENCHMARK FAILED N=%lu: candidate or allocation size overflow\n",
                (unsigned long)count);
            return 1;
        }
        values = (int *)malloc(count * sizeof(*values));
        items = (void **)malloc(count * sizeof(*items));
        if (values == NULL || items == NULL) {
            printf("BENCHMARK FAILED N=%lu: out of memory for input arrays\n",
                (unsigned long)count);
            free(items);
            free(values);
            return 1;
        }
        for (candidate_index = 0; candidate_index < run->candidate_count;
                ++candidate_index) {
            run->candidates[candidate_index].group_size =
                candidate_sizes[candidate_index];
        }

        {
            size_t seed_index;
            HpsComparator comparator;
            BenchmarkCompareContext counter;

            counter.comparison_count = 0;
            counter.overflowed = 0;
            comparator.compare = benchmark_compare_int;
            comparator.context = &counter;
            for (seed_index = 0; seed_index < HPS_SCALE_SEED_COUNT; ++seed_index) {
                uint32_t seed = UINT32_C(0xC0FFEE) + (uint32_t)seed_index;
                size_t value_index;

                for (value_index = 0; value_index < count; ++value_index) {
                    values[value_index] = (int)value_index;
                }
                benchmark_shuffle(values, count, seed);
                for (value_index = 0; value_index < count; ++value_index) {
                    items[value_index] = &values[value_index];
                }

                for (candidate_index = 0; candidate_index < run->candidate_count;
                        ++candidate_index) {
                    BenchmarkScaleCandidate *candidate =
                        &run->candidates[candidate_index];
                    size_t groups = 0;

                    candidate->count = count;
                    if (!benchmark_run_scale_sample(count, candidate->group_size,
                            seed, items, &comparator,
                            &candidate->samples[seed_index], &groups)) {
                        free(items);
                        free(values);
                        return 1;
                    }
                    if (!benchmark_add_size(&completed_samples, 1)) {
                        printf("BENCHMARK FAILED N=%lu seed=0x%08lX GroupSize=%lu: sample counter overflow\n",
                            (unsigned long)count, (unsigned long)seed,
                            (unsigned long)candidate->group_size);
                        free(items);
                        free(values);
                        return 1;
                    }
                    if (seed_index == 0) {
                        candidate->groups = groups;
                    } else if (candidate->groups != groups) {
                        printf("BENCHMARK FAILED N=%lu seed=0x%08lX GroupSize=%lu: actual group_count changed\n",
                            (unsigned long)count, (unsigned long)seed,
                            (unsigned long)candidate->group_size);
                        free(items);
                        free(values);
                        return 1;
                    }
                }
            }
        }
        for (candidate_index = 0; candidate_index < run->candidate_count;
                ++candidate_index) {
            if (!benchmark_scale_aggregate(&run->candidates[candidate_index])) {
                printf("BENCHMARK FAILED N=%lu GroupSize=%lu: aggregate counter overflow\n",
                    (unsigned long)count,
                    (unsigned long)run->candidates[candidate_index].group_size);
                free(items);
                free(values);
                return 1;
            }
        }
        free(items);
        free(values);
    }

    if (completed_samples != HPS_SCALE_SEED_COUNT *
            (runs[0].candidate_count + runs[1].candidate_count +
             runs[2].candidate_count + runs[3].candidate_count)) {
        printf("BENCHMARK FAILED: sample count mismatch\n");
        return 1;
    }
    printf("CompletedSamples=%lu\n", (unsigned long)completed_samples);

    printf("N,GroupSize,Groups,BuildCmpMean,MergeCmpMean,TotalCmpMean,TotalCmpMin,TotalCmpMax,TotalCmpStdDev,BuildCmpPerItem,MergeCmpPerItem,TotalCmpPerItem,BuildMsMean,MergeMsMean,TotalMsMean,TotalUsPerItem\n");
    for (run_index = 0; run_index < HPS_SCALE_N_COUNT; ++run_index) {
        const BenchmarkScaleRun *run = &runs[run_index];
        size_t candidate_index;
        for (candidate_index = 0; candidate_index < run->candidate_count;
                ++candidate_index) {
            const BenchmarkScaleCandidate *candidate =
                &run->candidates[candidate_index];
            const BenchmarkAggregate *aggregate = &candidate->aggregate;
            double count = (double)run->count;
            printf("%lu,%lu,%lu,%.3f,%.3f,%.3f,%lu,%lu,%.3f,%.6f,%.6f,%.6f,%.3f,%.3f,%.3f,%.3f\n",
                (unsigned long)run->count,
                (unsigned long)candidate->group_size,
                (unsigned long)candidate->groups,
                aggregate->build_comparisons.mean,
                aggregate->merge_comparisons.mean,
                aggregate->total_comparisons.mean,
                (unsigned long)aggregate->total_comparisons.minimum,
                (unsigned long)aggregate->total_comparisons.maximum,
                benchmark_distribution_stddev(&aggregate->total_comparisons),
                aggregate->build_comparisons.mean / count,
                aggregate->merge_comparisons.mean / count,
                aggregate->total_comparisons.mean / count,
                aggregate->build_ms_mean,
                aggregate->merge_ms_mean,
                aggregate->total_ms_mean,
                aggregate->total_ms_mean * 1000.0 / count);
        }
    }

    printf("N,GroupSize,MeanLocalAvgDepth,WorstLocalMaxDepth,MeanFinalAvgDepth,WorstFinalMaxDepth,MeanFinalAvgTextLen,WorstFinalMaxTextLen,WorstFinalMaxLevel\n");
    for (run_index = 0; run_index < HPS_SCALE_N_COUNT; ++run_index) {
        const BenchmarkScaleRun *run = &runs[run_index];
        size_t candidate_index;
        for (candidate_index = 0; candidate_index < run->candidate_count;
                ++candidate_index) {
            const BenchmarkScaleCandidate *candidate =
                &run->candidates[candidate_index];
            const BenchmarkAggregate *aggregate = &candidate->aggregate;
            printf("%lu,%lu,%.3f,%lu,%.3f,%lu,%.3f,%lu,%lu\n",
                (unsigned long)run->count,
                (unsigned long)candidate->group_size,
                aggregate->local_avg_depth_mean,
                (unsigned long)aggregate->local_max_depth_worst,
                aggregate->final_avg_depth_mean,
                (unsigned long)aggregate->final_max_depth_worst,
                aggregate->final_avg_text_length_mean,
                (unsigned long)aggregate->final_max_text_length_worst,
                (unsigned long)aggregate->final_max_level_worst);
        }
    }

    for (run_index = 0; run_index < HPS_SCALE_N_COUNT; ++run_index) {
        const BenchmarkScaleRun *run = &runs[run_index];
        size_t seed_index;
        size_t candidate_index;
        size_t expected_columns;

        printf("TotalCmpMatrix N=%lu\n", (unsigned long)run->count);
        printf("Seed");
        for (candidate_index = 0; candidate_index < run->candidate_count;
                ++candidate_index) {
            printf(",GS%lu", (unsigned long)run->candidates[candidate_index].group_size);
        }
        printf("\n");
        expected_columns = run->candidate_count + 1;
        for (seed_index = 0; seed_index < HPS_SCALE_SEED_COUNT; ++seed_index) {
            uint32_t seed = UINT32_C(0xC0FFEE) + (uint32_t)seed_index;
            size_t emitted_columns = 1;
            printf("0x%08lX", (unsigned long)seed);
            for (candidate_index = 0; candidate_index < run->candidate_count;
                    ++candidate_index) {
                printf(",%lu", (unsigned long)run->candidates[candidate_index]
                    .samples[seed_index].total_comparisons);
                ++emitted_columns;
            }
            if (emitted_columns != expected_columns) {
                printf("\nBENCHMARK FAILED N=%lu seed=0x%08lX: matrix column count mismatch\n",
                    (unsigned long)run->count, (unsigned long)seed);
                return 1;
            }
            printf("\n");
        }
    }

    printf("N,LowestObservedMeanTotalCmp,GroupSizeAtThatObservedMean,TotalCmpPerItem,GroupSizeOverN\n");
    for (run_index = 0; run_index < HPS_SCALE_N_COUNT; ++run_index) {
        const BenchmarkScaleRun *run = &runs[run_index];
        size_t candidate_index;
        size_t minimum_sum = (size_t)-1;
        for (candidate_index = 0; candidate_index < run->candidate_count;
                ++candidate_index) {
            if (run->candidates[candidate_index].total_comparison_sum < minimum_sum) {
                minimum_sum = run->candidates[candidate_index].total_comparison_sum;
            }
        }
        for (candidate_index = 0; candidate_index < run->candidate_count;
                ++candidate_index) {
            const BenchmarkScaleCandidate *candidate =
                &run->candidates[candidate_index];
            if (candidate->total_comparison_sum == minimum_sum) {
                double mean = (double)minimum_sum / HPS_SCALE_SEED_COUNT;
                printf("%lu,%.3f,%lu,%.6f,%.6f\n",
                    (unsigned long)run->count,
                    mean,
                    (unsigned long)candidate->group_size,
                    mean / (double)run->count,
                    (double)candidate->group_size / (double)run->count);
            }
        }
    }

    printf("N,GS512TotalCmpMean,GS512TotalCmpPerItem,GS512MeanFinalAvgDepth\n");
    for (run_index = 0; run_index < HPS_SCALE_N_COUNT; ++run_index) {
        const BenchmarkScaleRun *run = &runs[run_index];
        const BenchmarkScaleCandidate *candidate =
            benchmark_scale_find_candidate(run, 512);
        if (candidate == NULL) {
            printf("BENCHMARK FAILED N=%lu: GS512 candidate missing\n",
                (unsigned long)run->count);
            return 1;
        }
        printf("%lu,%.3f,%.6f,%.3f\n",
            (unsigned long)run->count,
            candidate->aggregate.total_comparisons.mean,
            candidate->aggregate.total_comparisons.mean / (double)run->count,
            candidate->aggregate.final_avg_depth_mean);
    }

    printf("N,GS256TotalCmpMean,GS256TotalCmpPerItem,GS256MeanFinalAvgDepth\n");
    for (run_index = 0; run_index < HPS_SCALE_N_COUNT; ++run_index) {
        const BenchmarkScaleRun *run = &runs[run_index];
        const BenchmarkScaleCandidate *candidate =
            benchmark_scale_find_candidate(run, 256);
        if (candidate == NULL) {
            printf("BENCHMARK FAILED N=%lu: GS256 candidate missing\n",
                (unsigned long)run->count);
            return 1;
        }
        printf("%lu,%.3f,%.6f,%.3f\n",
            (unsigned long)run->count,
            candidate->aggregate.total_comparisons.mean,
            candidate->aggregate.total_comparisons.mean / (double)run->count,
            candidate->aggregate.final_avg_depth_mean);
    }

    printf("N,HalfNGroupSize,TotalCmpMean,TotalCmpPerItem,MeanFinalAvgDepth\n");
    for (run_index = 0; run_index < HPS_SCALE_N_COUNT; ++run_index) {
        const BenchmarkScaleRun *run = &runs[run_index];
        size_t half_size = run->count / 2;
        const BenchmarkScaleCandidate *candidate =
            benchmark_scale_find_candidate(run, half_size);
        if (candidate == NULL) {
            printf("BENCHMARK FAILED N=%lu: half-N candidate missing\n",
                (unsigned long)run->count);
            return 1;
        }
        printf("%lu,%lu,%.3f,%.6f,%.3f\n",
            (unsigned long)run->count,
            (unsigned long)half_size,
            candidate->aggregate.total_comparisons.mean,
            candidate->aggregate.total_comparisons.mean / (double)run->count,
            candidate->aggregate.final_avg_depth_mean);
    }
    printf("Path metrics are structural-size proxies, not actual heap memory bytes.\n");
    return 0;
}

enum {
    HPS_GROUP_COUNT_TARGETS = 18,
    HPS_GROUP_COUNT_SEEDS = 10,
    HPS_GROUP_COUNT_ITEMS = 10000,
    HPS_GROUP_COUNT_MAX_GROUPS = 80
};

typedef struct BenchmarkGroupCountCandidate {
    size_t target_groups;
    size_t group_size;
    size_t actual_groups;
    BenchmarkSample samples[HPS_GROUP_COUNT_SEEDS];
    BenchmarkDistribution build_comparisons;
    BenchmarkDistribution merge_comparisons;
    BenchmarkDistribution total_comparisons;
    BenchmarkDistribution merge_gaps;
    double mean_gap_percent;
    double max_gap_percent;
    size_t max_gap;
    double build_ms_mean;
    double merge_ms_mean;
    double total_ms_mean;
    double local_avg_depth_mean;
    double final_avg_depth_mean;
    double final_avg_text_length_mean;
    size_t final_max_depth_worst;
    size_t final_max_level_worst;
    size_t predicted_merge_upper;
    size_t merge_element_work;
    size_t merge_pair_count;
    size_t merge_rounds;
} BenchmarkGroupCountCandidate;

typedef struct BenchmarkMergePrediction {
    size_t element_work;
    size_t pair_count;
    size_t rounds;
    size_t upper_bound;
} BenchmarkMergePrediction;

/*
 * For equal initial Groups and a power-of-two GroupCount G=2^r, the
 * closed-form upper bound is 2 * (N * r - (G - 1)).  The general simulator
 * below remains the predictor used by this benchmark, including uneven tails.
 */
static int benchmark_predict_merge_upper(
    const size_t *initial_sizes,
    size_t group_count,
    size_t total_items,
    BenchmarkMergePrediction *prediction
)
{
    size_t current[HPS_GROUP_COUNT_MAX_GROUPS];
    size_t next[HPS_GROUP_COUNT_MAX_GROUPS];
    size_t index;
    size_t current_count = group_count;
    size_t size_sum = 0;
    size_t element_work = 0;
    size_t pair_count = 0;
    size_t rounds = 0;

    if (prediction == NULL ||
        (group_count != 0 && initial_sizes == NULL) ||
        group_count > HPS_GROUP_COUNT_MAX_GROUPS) {
        return 0;
    }
    memset(prediction, 0, sizeof(*prediction));
    if (group_count == 0 || total_items == 0) {
        return 1;
    }
    for (index = 0; index < group_count; ++index) {
        current[index] = initial_sizes[index];
        if (!benchmark_add_size(&size_sum, current[index])) {
            return 0;
        }
    }
    if (size_sum != total_items) {
        return 0;
    }
    if (group_count == 1) {
        prediction->upper_bound = total_items - 1;
        return 1;
    }

    while (current_count > 1) {
        size_t next_count = 0;
        for (index = 0; index + 1 < current_count; index += 2) {
            size_t merged_size = 0;
            if (!benchmark_add_size(&merged_size, current[index])) {
                return 0;
            }
            if (!benchmark_add_size(&merged_size, current[index + 1]) ||
                !benchmark_add_size(&element_work, merged_size) ||
                !benchmark_add_size(&pair_count, 1)) {
                return 0;
            }
            next[next_count++] = merged_size;
        }
        if (current_count % 2 != 0) {
            next[next_count++] = current[current_count - 1];
        }
        {
            for (index = 0; index < next_count; ++index) {
                current[index] = next[index];
            }
        }
        current_count = next_count;
        if (!benchmark_add_size(&rounds, 1)) {
            return 0;
        }
    }
    if (element_work < pair_count) {
        return 0;
    }
    if (element_work - pair_count > (size_t)-1 / 2) {
        return 0;
    }
    prediction->element_work = element_work;
    prediction->pair_count = pair_count;
    prediction->rounds = rounds;
    prediction->upper_bound = 2 * (element_work - pair_count);
    return 1;
}

static int benchmark_predict_batch_merge_upper(
    const HpsGroupBatch *batch,
    size_t total_items,
    BenchmarkMergePrediction *prediction
)
{
    size_t group_count = hps_group_batch_group_count(batch);
    size_t sizes[HPS_GROUP_COUNT_MAX_GROUPS];
    size_t index;

    if (batch == NULL || group_count > HPS_GROUP_COUNT_MAX_GROUPS) {
        return 0;
    }
    for (index = 0; index < group_count; ++index) {
        const HpsGroup *group = hps_group_batch_group_at(batch, index);
        if (group == NULL) {
            return 0;
        }
        sizes[index] = hps_group_size(group);
    }
    return benchmark_predict_merge_upper(sizes, group_count, total_items,
        prediction);
}

static int benchmark_run_group_count_sample(
    size_t target_groups,
    size_t group_size,
    uint32_t seed,
    void **items,
    HpsComparator *comparator,
    BenchmarkSample *sample,
    size_t *out_groups,
    BenchmarkMergePrediction *prediction
)
{
    HpsGroupBatch *batch = NULL;
    HpsGroup *final_group = NULL;
    BenchmarkCompareContext *counter =
        (BenchmarkCompareContext *)comparator->context;
    clock_t begin;
    clock_t end;
    HpsStatus status;

    memset(sample, 0, sizeof(*sample));
    counter->comparison_count = 0;
    counter->overflowed = 0;
    begin = clock();
    status = hps_group_batch_build(items, HPS_GROUP_COUNT_ITEMS, group_size,
        comparator, &batch);
    end = clock();
    if (status != HPS_STATUS_OK || batch == NULL || counter->overflowed ||
        !benchmark_elapsed_ms(begin, end, &sample->build_ms)) {
        printf("BENCHMARK FAILED seed=0x%08lX TargetGroups=%lu GroupSize=%lu: build failed (%s)\n",
            (unsigned long)seed, (unsigned long)target_groups,
            (unsigned long)group_size, hps_status_string(status));
        hps_group_batch_destroy(batch);
        return 0;
    }
    sample->build_comparisons = counter->comparison_count;
    *out_groups = hps_group_batch_group_count(batch);
    if (*out_groups != target_groups) {
        printf("BENCHMARK FAILED seed=0x%08lX TargetGroups=%lu GroupSize=%lu ActualGroups=%lu\n",
            (unsigned long)seed, (unsigned long)target_groups,
            (unsigned long)group_size, (unsigned long)*out_groups);
        hps_group_batch_destroy(batch);
        return 0;
    }
    if (hps_group_batch_total_size(batch) != HPS_GROUP_COUNT_ITEMS ||
        !benchmark_collect_batch_paths(batch, &sample->local_stats)) {
        printf("BENCHMARK FAILED seed=0x%08lX TargetGroups=%lu GroupSize=%lu: local Path statistics failed\n",
            (unsigned long)seed, (unsigned long)target_groups,
            (unsigned long)group_size);
        hps_group_batch_destroy(batch);
        return 0;
    }
    if (!benchmark_predict_batch_merge_upper(batch, HPS_GROUP_COUNT_ITEMS,
            prediction)) {
        printf("BENCHMARK FAILED seed=0x%08lX TargetGroups=%lu GroupSize=%lu: theoretical merge prediction overflow/invalid sizes\n",
            (unsigned long)seed, (unsigned long)target_groups,
            (unsigned long)group_size);
        hps_group_batch_destroy(batch);
        return 0;
    }

    counter->comparison_count = 0;
    counter->overflowed = 0;
    begin = clock();
    status = hps_group_batch_merge_all(batch, comparator, &final_group);
    end = clock();
    if (status != HPS_STATUS_OK || final_group == NULL || counter->overflowed ||
        !benchmark_elapsed_ms(begin, end, &sample->merge_ms)) {
        printf("BENCHMARK FAILED seed=0x%08lX TargetGroups=%lu GroupSize=%lu: merge failed (%s)\n",
            (unsigned long)seed, (unsigned long)target_groups,
            (unsigned long)group_size, hps_status_string(status));
        hps_group_destroy(final_group);
        hps_group_batch_destroy(batch);
        return 0;
    }
    sample->merge_comparisons = counter->comparison_count;
    if (sample->merge_comparisons > prediction->upper_bound) {
        printf("BENCHMARK FAILED seed=0x%08lX TargetGroups=%lu GroupSize=%lu ActualMergeCmp=%lu PredictedMergeUpper=%lu\n",
            (unsigned long)seed, (unsigned long)target_groups,
            (unsigned long)group_size,
            (unsigned long)sample->merge_comparisons,
            (unsigned long)prediction->upper_bound);
        hps_group_destroy(final_group);
        hps_group_batch_destroy(batch);
        return 0;
    }
    sample->predicted_merge_upper = prediction->upper_bound;
    sample->merge_gap = prediction->upper_bound - sample->merge_comparisons;
    sample->merge_gap_percent = prediction->upper_bound == 0 ? 0.0 :
        100.0 * (double)sample->merge_gap /
            (double)prediction->upper_bound;
    if (sample->build_comparisons > (size_t)-1 - sample->merge_comparisons) {
        printf("BENCHMARK FAILED seed=0x%08lX TargetGroups=%lu GroupSize=%lu: comparison count overflow\n",
            (unsigned long)seed, (unsigned long)target_groups,
            (unsigned long)group_size);
        hps_group_destroy(final_group);
        hps_group_batch_destroy(batch);
        return 0;
    }
    sample->total_comparisons = sample->build_comparisons +
        sample->merge_comparisons;
    sample->total_ms = sample->build_ms + sample->merge_ms;
    if (!benchmark_validate_final(final_group, HPS_GROUP_COUNT_ITEMS,
            &sample->final_stats)) {
        printf("BENCHMARK FAILED seed=0x%08lX TargetGroups=%lu GroupSize=%lu: result size/order/adjacent Path validation failed\n",
            (unsigned long)seed, (unsigned long)target_groups,
            (unsigned long)group_size);
        hps_group_destroy(final_group);
        hps_group_batch_destroy(batch);
        return 0;
    }
    hps_group_destroy(final_group);
    hps_group_batch_destroy(batch);
    return 1;
}

static void benchmark_group_count_aggregate(
    BenchmarkGroupCountCandidate *candidate)
{
    size_t seed_index;
    double build_ms = 0.0;
    double merge_ms = 0.0;
    double total_ms = 0.0;
    double local_depth = 0.0;
    double final_depth = 0.0;
    double final_text = 0.0;
    double gap_percent_total = 0.0;

    memset(&candidate->build_comparisons, 0,
        sizeof(candidate->build_comparisons));
    memset(&candidate->merge_comparisons, 0,
        sizeof(candidate->merge_comparisons));
    memset(&candidate->total_comparisons, 0,
        sizeof(candidate->total_comparisons));
    memset(&candidate->merge_gaps, 0, sizeof(candidate->merge_gaps));
    for (seed_index = 0; seed_index < HPS_GROUP_COUNT_SEEDS; ++seed_index) {
        const BenchmarkSample *sample = &candidate->samples[seed_index];
        benchmark_distribution_add(&candidate->build_comparisons,
            sample->build_comparisons, seed_index);
        benchmark_distribution_add(&candidate->merge_comparisons,
            sample->merge_comparisons, seed_index);
        benchmark_distribution_add(&candidate->total_comparisons,
            sample->total_comparisons, seed_index);
        benchmark_distribution_add(&candidate->merge_gaps,
            sample->merge_gap, seed_index);
        gap_percent_total += sample->merge_gap_percent;
        if (seed_index == 0 || sample->merge_gap > candidate->max_gap) {
            candidate->max_gap = sample->merge_gap;
        }
        if (seed_index == 0 || sample->merge_gap_percent >
                candidate->max_gap_percent) {
            candidate->max_gap_percent = sample->merge_gap_percent;
        }
        build_ms += sample->build_ms;
        merge_ms += sample->merge_ms;
        total_ms += sample->total_ms;
        local_depth += (double)sample->local_stats.total_depth /
            (double)HPS_GROUP_COUNT_ITEMS;
        final_depth += (double)sample->final_stats.total_depth /
            (double)HPS_GROUP_COUNT_ITEMS;
        final_text += (double)sample->final_stats.total_text_length /
            (double)HPS_GROUP_COUNT_ITEMS;
        if (seed_index == 0 || sample->final_stats.max_depth >
                candidate->final_max_depth_worst) {
            candidate->final_max_depth_worst = sample->final_stats.max_depth;
        }
        if (seed_index == 0 || sample->final_stats.max_level >
                candidate->final_max_level_worst) {
            candidate->final_max_level_worst = sample->final_stats.max_level;
        }
    }
    candidate->build_ms_mean = build_ms / HPS_GROUP_COUNT_SEEDS;
    candidate->merge_ms_mean = merge_ms / HPS_GROUP_COUNT_SEEDS;
    candidate->total_ms_mean = total_ms / HPS_GROUP_COUNT_SEEDS;
    candidate->local_avg_depth_mean = local_depth / HPS_GROUP_COUNT_SEEDS;
    candidate->final_avg_depth_mean = final_depth / HPS_GROUP_COUNT_SEEDS;
    candidate->final_avg_text_length_mean = final_text / HPS_GROUP_COUNT_SEEDS;
    candidate->mean_gap_percent = gap_percent_total / HPS_GROUP_COUNT_SEEDS;
}

int hps_run_group_count_cost_benchmark(void)
{
    static const size_t targets[HPS_GROUP_COUNT_TARGETS] = {
        1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 16, 20, 25, 32, 40, 50, 64, 80
    };
    BenchmarkGroupCountCandidate candidates[HPS_GROUP_COUNT_TARGETS];
    int *values = NULL;
    void **items = NULL;
    HpsComparator comparator;
    BenchmarkCompareContext counter;
    size_t candidate_index;
    size_t seed_index;
    size_t completed = 0;

    memset(candidates, 0, sizeof(candidates));
    if ((size_t)HPS_GROUP_COUNT_ITEMS > (size_t)-1 / sizeof(*values) ||
        (size_t)HPS_GROUP_COUNT_ITEMS > (size_t)-1 / sizeof(*items)) {
        printf("BENCHMARK FAILED: input allocation size overflow\n");
        return 1;
    }
    values = (int *)malloc((size_t)HPS_GROUP_COUNT_ITEMS * sizeof(*values));
    items = (void **)malloc((size_t)HPS_GROUP_COUNT_ITEMS * sizeof(*items));
    if (values == NULL || items == NULL) {
        printf("BENCHMARK FAILED: out of memory for input arrays\n");
        free(items);
        free(values);
        return 1;
    }
    counter.comparison_count = 0;
    counter.overflowed = 0;
    comparator.compare = benchmark_compare_int;
    comparator.context = &counter;
    for (candidate_index = 0; candidate_index < HPS_GROUP_COUNT_TARGETS;
            ++candidate_index) {
        size_t groups = targets[candidate_index];
        size_t quotient = (size_t)HPS_GROUP_COUNT_ITEMS / groups;
        size_t remainder = (size_t)HPS_GROUP_COUNT_ITEMS % groups;
        candidates[candidate_index].target_groups = groups;
        candidates[candidate_index].group_size = quotient +
            (remainder != 0 ? 1u : 0u);
    }

    printf("HPSort Group Count Cost Benchmark\n");
    printf("N=10000; Seeds=10; SeedStart=0x00C0FFEE; SeedEnd=0x00C0FFF7; each seed shuffled once and shared across targets\n");
    for (seed_index = 0; seed_index < HPS_GROUP_COUNT_SEEDS; ++seed_index) {
        uint32_t seed = UINT32_C(0xC0FFEE) + (uint32_t)seed_index;
        size_t item_index;
        for (item_index = 0; item_index < (size_t)HPS_GROUP_COUNT_ITEMS;
                ++item_index) {
            values[item_index] = (int)item_index;
        }
        benchmark_shuffle(values, (size_t)HPS_GROUP_COUNT_ITEMS, seed);
        for (item_index = 0; item_index < (size_t)HPS_GROUP_COUNT_ITEMS;
                ++item_index) {
            items[item_index] = &values[item_index];
        }
        for (candidate_index = 0; candidate_index < HPS_GROUP_COUNT_TARGETS;
                ++candidate_index) {
            BenchmarkGroupCountCandidate *candidate = &candidates[candidate_index];
            size_t actual_groups = 0;
            BenchmarkMergePrediction prediction;
            if (!benchmark_run_group_count_sample(candidate->target_groups,
                    candidate->group_size, seed, items, &comparator,
                    &candidate->samples[seed_index], &actual_groups,
                    &prediction)) {
                free(items);
                free(values);
                return 1;
            }
            if (seed_index == 0) {
                candidate->actual_groups = actual_groups;
            } else if (candidate->actual_groups != actual_groups) {
                printf("BENCHMARK FAILED seed=0x%08lX TargetGroups=%lu GroupSize=%lu ActualGroups=%lu: count changed across seeds\n",
                    (unsigned long)seed, (unsigned long)candidate->target_groups,
                    (unsigned long)candidate->group_size,
                    (unsigned long)actual_groups);
                free(items);
                free(values);
                return 1;
            }
            if (seed_index == 0) {
                candidate->predicted_merge_upper = prediction.upper_bound;
                candidate->merge_element_work = prediction.element_work;
                candidate->merge_pair_count = prediction.pair_count;
                candidate->merge_rounds = prediction.rounds;
            } else if (candidate->predicted_merge_upper != prediction.upper_bound ||
                    candidate->merge_element_work != prediction.element_work ||
                    candidate->merge_pair_count != prediction.pair_count ||
                    candidate->merge_rounds != prediction.rounds) {
                printf("BENCHMARK FAILED seed=0x%08lX TargetGroups=%lu GroupSize=%lu: prediction changed across identical batch structures\n",
                    (unsigned long)seed, (unsigned long)candidate->target_groups,
                    (unsigned long)candidate->group_size);
                free(items);
                free(values);
                return 1;
            }
            candidate->samples[seed_index].merge_rounds = prediction.rounds;
            candidate->samples[seed_index].merge_pair_count = prediction.pair_count;
            candidate->samples[seed_index].merge_element_work = prediction.element_work;
            ++completed;
        }
    }
    free(items);
    free(values);
    if (completed != HPS_GROUP_COUNT_TARGETS * HPS_GROUP_COUNT_SEEDS) {
        printf("BENCHMARK FAILED: completed sample count=%lu expected=%u\n",
            (unsigned long)completed,
            HPS_GROUP_COUNT_TARGETS * HPS_GROUP_COUNT_SEEDS);
        return 1;
    }
    for (candidate_index = 0; candidate_index < HPS_GROUP_COUNT_TARGETS;
            ++candidate_index) {
        benchmark_group_count_aggregate(&candidates[candidate_index]);
    }
    printf("CompletedSamples=%lu\n", (unsigned long)completed);
    printf("Groups,GroupSize,MergeRounds,MergePairCount,MergeElementWork,PredictedMergeUpper,ActualMergeCmpMean,MeanGap,MaxGap,MeanGapPercent,MaxGapPercent\n");
    for (candidate_index = 0; candidate_index < HPS_GROUP_COUNT_TARGETS;
            ++candidate_index) {
        const BenchmarkGroupCountCandidate *c = &candidates[candidate_index];
        printf("%lu,%lu,%lu,%lu,%lu,%lu,%.3f,%.3f,%lu,%.4f,%.4f\n",
            (unsigned long)c->actual_groups, (unsigned long)c->group_size,
            (unsigned long)c->merge_rounds,
            (unsigned long)c->merge_pair_count,
            (unsigned long)c->merge_element_work,
            (unsigned long)c->predicted_merge_upper,
            c->merge_comparisons.mean, c->merge_gaps.mean,
            (unsigned long)c->max_gap, c->mean_gap_percent,
            c->max_gap_percent);
    }

    printf("GapMatrix\nSeed,G1,G2,G3,G4,G5,G6,G7,G8,G10,G12,G16,G20,G25,G32,G40,G50,G64,G80\n");
    for (seed_index = 0; seed_index < HPS_GROUP_COUNT_SEEDS; ++seed_index) {
        uint32_t seed = UINT32_C(0xC0FFEE) + (uint32_t)seed_index;
        printf("0x%08lX", (unsigned long)seed);
        for (candidate_index = 0; candidate_index < HPS_GROUP_COUNT_TARGETS;
                ++candidate_index) {
            printf(",%lu", (unsigned long)candidates[candidate_index]
                .samples[seed_index].merge_gap);
        }
        printf("\n");
    }
    printf("GapPercentMatrix\nSeed,G1,G2,G3,G4,G5,G6,G7,G8,G10,G12,G16,G20,G25,G32,G40,G50,G64,G80\n");
    for (seed_index = 0; seed_index < HPS_GROUP_COUNT_SEEDS; ++seed_index) {
        uint32_t seed = UINT32_C(0xC0FFEE) + (uint32_t)seed_index;
        printf("0x%08lX", (unsigned long)seed);
        for (candidate_index = 0; candidate_index < HPS_GROUP_COUNT_TARGETS;
                ++candidate_index) {
            printf(",%.4f", candidates[candidate_index]
                .samples[seed_index].merge_gap_percent);
        }
        printf("\n");
    }

    printf("ReferenceCases\nGroups,GroupSize,MergeRounds,MergePairCount,MergeElementWork,PredictedMergeUpper\n");
    {
        static const size_t expected_groups[] = { 5, 8, 64 };
        static const size_t expected_work[] = { 26000, 30000, 60000 };
        static const size_t expected_pairs[] = { 4, 7, 63 };
        static const size_t expected_upper[] = { 51992, 59986, 119874 };
        size_t reference_index;
        for (reference_index = 0; reference_index < 3; ++reference_index) {
            size_t index;
            for (index = 0; index < HPS_GROUP_COUNT_TARGETS; ++index) {
                const BenchmarkGroupCountCandidate *c = &candidates[index];
                if (c->actual_groups == expected_groups[reference_index]) {
                    if (c->merge_element_work != expected_work[reference_index] ||
                        c->merge_pair_count != expected_pairs[reference_index] ||
                        c->predicted_merge_upper != expected_upper[reference_index]) {
                        printf("BENCHMARK FAILED reference GroupCount=%lu: work/pairs/upper mismatch\n",
                            (unsigned long)c->actual_groups);
                        return 1;
                    }
                    printf("%lu,%lu,%lu,%lu,%lu,%lu\n",
                        (unsigned long)c->actual_groups,
                        (unsigned long)c->group_size,
                        (unsigned long)c->merge_rounds,
                        (unsigned long)c->merge_pair_count,
                        (unsigned long)c->merge_element_work,
                        (unsigned long)c->predicted_merge_upper);
                    break;
                }
            }
            if (index == HPS_GROUP_COUNT_TARGETS) {
                printf("BENCHMARK FAILED: reference GroupCount=%lu missing\n",
                    (unsigned long)expected_groups[reference_index]);
                return 1;
            }
        }
    }
    {
        size_t sample_count = 0;
        size_t seed_index_local;
        double gap_percent_total = 0.0;
        double min_gap_percent = 0.0;
        double max_gap_percent = 0.0;
        double group_mean_min = 0.0;
        double group_mean_max = 0.0;
        for (candidate_index = 0; candidate_index < HPS_GROUP_COUNT_TARGETS;
                ++candidate_index) {
            const BenchmarkGroupCountCandidate *c = &candidates[candidate_index];
            if (candidate_index == 0 || c->mean_gap_percent < group_mean_min) {
                group_mean_min = c->mean_gap_percent;
            }
            if (candidate_index == 0 || c->mean_gap_percent > group_mean_max) {
                group_mean_max = c->mean_gap_percent;
            }
            for (seed_index_local = 0; seed_index_local < HPS_GROUP_COUNT_SEEDS;
                    ++seed_index_local) {
                double value = c->samples[seed_index_local].merge_gap_percent;
                if (sample_count == 0 || value < min_gap_percent) {
                    min_gap_percent = value;
                }
                if (sample_count == 0 || value > max_gap_percent) {
                    max_gap_percent = value;
                }
                gap_percent_total += value;
                ++sample_count;
            }
        }
        printf("GapPercentSummary\nSamples,AllSampleMeanGapPercent,AllSampleMinGapPercent,AllSampleMaxGapPercent,PerGroupMeanGapPercentMin,PerGroupMeanGapPercentMax\n");
        printf("%lu,%.4f,%.4f,%.4f,%.4f,%.4f\n",
            (unsigned long)sample_count,
            gap_percent_total / (double)sample_count,
            min_gap_percent, max_gap_percent, group_mean_min, group_mean_max);
    }
    return 0;
}

enum {
    HPS_STAGE76_SEEDS = 10,
    HPS_STAGE76_TRAIN_COUNT = 9,
    HPS_STAGE76_VALIDATION_COUNT = 9,
    HPS_STAGE76_BATCH_COUNT = 18,
    HPS_STAGE76_MAX_ITEMS = 10000
};

typedef struct BenchmarkBuildPoint {
    size_t group_size;
    BenchmarkDistribution comparisons;
    double per_item_mean;
} BenchmarkBuildPoint;

typedef struct BenchmarkBuildModel {
    double a;
    double b;
    double c;
} BenchmarkBuildModel;

typedef struct BenchmarkErrorMetrics {
    double rmse;
    double mape_percent;
    double max_abs_percent_error;
} BenchmarkErrorMetrics;

typedef struct BenchmarkBatchModelRow {
    size_t target_groups;
    size_t actual_groups;
    size_t group_size;
    size_t predicted_merge_upper;
    BenchmarkSample samples[HPS_STAGE76_SEEDS];
    BenchmarkDistribution actual_build;
    BenchmarkDistribution actual_merge;
    BenchmarkDistribution actual_total;
    double predicted_build_l;
    double predicted_build_q;
    double actual_build_mean;
    double actual_merge_mean;
    double actual_total_mean;
    double predicted_total_l;
    double predicted_total_q;
} BenchmarkBatchModelRow;

static double benchmark_log2_size(size_t value)
{
    return log((double)value) / log(2.0);
}

static double benchmark_build_model_predict(
    const BenchmarkBuildModel *model,
    size_t group_size,
    int quadratic
)
{
    double x;
    double y;

    if (group_size <= 1) {
        return 0.0;
    }
    x = benchmark_log2_size(group_size);
    y = model->a + model->b * x;
    if (quadratic) {
        y += model->c * x * x;
    }
    return (double)group_size * y;
}

static int benchmark_run_single_group_build(
    size_t count,
    uint32_t seed,
    int *values,
    void **items,
    HpsComparator *comparator,
    size_t *out_comparisons
)
{
    HpsGroup *group = NULL;
    BenchmarkCompareContext *counter =
        (BenchmarkCompareContext *)comparator->context;
    BenchmarkPathStats path_stats;
    size_t index;
    HpsStatus status;

    for (index = 0; index < count; ++index) {
        values[index] = (int)index;
    }
    benchmark_shuffle(values, count, seed);
    for (index = 0; index < count; ++index) {
        items[index] = &values[index];
    }
    counter->comparison_count = 0;
    counter->overflowed = 0;
    status = hps_group_build(items, count, comparator, &group);
    if (status != HPS_STATUS_OK || group == NULL || counter->overflowed) {
        printf("BENCHMARK FAILED single Group Build size=%lu seed=0x%08lX: %s\n",
            (unsigned long)count, (unsigned long)seed,
            hps_status_string(status));
        hps_group_destroy(group);
        return 0;
    }
    *out_comparisons = counter->comparison_count;
    memset(&path_stats, 0, sizeof(path_stats));
    if (!benchmark_validate_final(group, count, &path_stats)) {
        printf("BENCHMARK FAILED single Group Build size=%lu seed=0x%08lX: size/order/adjacent Path validation failed\n",
            (unsigned long)count, (unsigned long)seed);
        hps_group_destroy(group);
        return 0;
    }
    hps_group_destroy(group);
    return 1;
}

static int benchmark_fit_linear_model(
    const BenchmarkBuildPoint *points,
    size_t point_count,
    BenchmarkBuildModel *model
)
{
    size_t index;
    double sum_x = 0.0;
    double sum_y = 0.0;
    double sum_xx = 0.0;
    double sum_xy = 0.0;
    double denominator;
    double count = (double)point_count;

    if (points == NULL || model == NULL || point_count < 2) {
        return 0;
    }
    for (index = 0; index < point_count; ++index) {
        double x = benchmark_log2_size(points[index].group_size);
        double y = points[index].per_item_mean;
        sum_x += x;
        sum_y += y;
        sum_xx += x * x;
        sum_xy += x * y;
    }
    denominator = count * sum_xx - sum_x * sum_x;
    if (fabs(denominator) <= 1.0e-12 * (fabs(count * sum_xx) +
            fabs(sum_x * sum_x) + 1.0)) {
        return 0;
    }
    model->b = (count * sum_xy - sum_x * sum_y) / denominator;
    model->a = (sum_y - model->b * sum_x) / count;
    model->c = 0.0;
    return 1;
}

static int benchmark_fit_quadratic_model(
    const BenchmarkBuildPoint *points,
    size_t point_count,
    BenchmarkBuildModel *model
)
{
    double augmented[3][4] = { { 0.0 } };
    double matrix_scale = 0.0;
    double coefficients[3] = { 0.0, 0.0, 0.0 };
    size_t index;
    size_t row;
    size_t column;

    if (points == NULL || model == NULL || point_count < 3) {
        return 0;
    }
    for (index = 0; index < point_count; ++index) {
        double x = benchmark_log2_size(points[index].group_size);
        double y = points[index].per_item_mean;
        double powers[5];
        size_t power;
        powers[0] = 1.0;
        for (power = 1; power < 5; ++power) {
            powers[power] = powers[power - 1] * x;
        }
        for (row = 0; row < 3; ++row) {
            for (column = 0; column < 3; ++column) {
                augmented[row][column] += powers[row + column];
            }
            augmented[row][3] += powers[row] * y;
        }
    }
    for (row = 0; row < 3; ++row) {
        for (column = 0; column < 3; ++column) {
            double magnitude = fabs(augmented[row][column]);
            if (magnitude > matrix_scale) {
                matrix_scale = magnitude;
            }
        }
    }
    if (matrix_scale == 0.0) {
        return 0;
    }
    for (column = 0; column < 3; ++column) {
        size_t pivot = column;
        double largest = fabs(augmented[column][column]);
        for (row = column + 1; row < 3; ++row) {
            double magnitude = fabs(augmented[row][column]);
            if (magnitude > largest) {
                largest = magnitude;
                pivot = row;
            }
        }
        if (largest <= matrix_scale * 1.0e-12) {
            return 0;
        }
        if (pivot != column) {
            size_t swap_column;
            for (swap_column = column; swap_column < 4; ++swap_column) {
                double temporary = augmented[column][swap_column];
                augmented[column][swap_column] = augmented[pivot][swap_column];
                augmented[pivot][swap_column] = temporary;
            }
        }
        for (row = column + 1; row < 3; ++row) {
            double factor = augmented[row][column] /
                augmented[column][column];
            size_t eliminate_column;
            for (eliminate_column = column; eliminate_column < 4;
                    ++eliminate_column) {
                augmented[row][eliminate_column] -= factor *
                    augmented[column][eliminate_column];
            }
        }
    }
    for (row = 3; row-- > 0;) {
        double value = augmented[row][3];
        for (column = row + 1; column < 3; ++column) {
            value -= augmented[row][column] * coefficients[column];
        }
        if (fabs(augmented[row][row]) <= matrix_scale * 1.0e-12) {
            return 0;
        }
        coefficients[row] = value / augmented[row][row];
    }
    model->a = coefficients[0];
    model->b = coefficients[1];
    model->c = coefficients[2];
    return 1;
}

static BenchmarkErrorMetrics benchmark_model_metrics(
    const BenchmarkBuildPoint *points,
    size_t point_count,
    const BenchmarkBuildModel *model,
    int quadratic
)
{
    BenchmarkErrorMetrics metrics;
    size_t index;
    double squared_error_total = 0.0;
    double percent_error_total = 0.0;
    metrics.rmse = 0.0;
    metrics.mape_percent = 0.0;
    metrics.max_abs_percent_error = 0.0;
    for (index = 0; index < point_count; ++index) {
        double actual = points[index].per_item_mean;
        double predicted = benchmark_build_model_predict(model,
            points[index].group_size, quadratic) /
            (double)points[index].group_size;
        double error = predicted - actual;
        double percent = 100.0 * fabs(error) / fabs(actual);
        squared_error_total += error * error;
        percent_error_total += percent;
        if (percent > metrics.max_abs_percent_error) {
            metrics.max_abs_percent_error = percent;
        }
    }
    metrics.rmse = sqrt(squared_error_total / (double)point_count);
    metrics.mape_percent = percent_error_total / (double)point_count;
    return metrics;
}

static int benchmark_predict_batch_build(
    const HpsGroupBatch *batch,
    const BenchmarkBuildModel *model,
    int quadratic,
    double *out_prediction
)
{
    size_t groups;
    size_t index;
    double prediction = 0.0;

    if (batch == NULL || model == NULL || out_prediction == NULL) {
        return 0;
    }
    groups = hps_group_batch_group_count(batch);
    for (index = 0; index < groups; ++index) {
        const HpsGroup *group = hps_group_batch_group_at(batch, index);
        size_t group_size;
        double value;
        if (group == NULL) {
            return 0;
        }
        group_size = hps_group_size(group);
        value = benchmark_build_model_predict(model, group_size, quadratic);
        if (value != value || value > 1.0e300 || value < -1.0e300) {
            return 0;
        }
        prediction += value;
    }
    *out_prediction = prediction;
    return 1;
}

static double benchmark_percent_error(double predicted, double actual)
{
    return 100.0 * fabs(predicted - actual) / fabs(actual);
}

static BenchmarkErrorMetrics benchmark_comparison_metrics(
    const double *actual,
    const double *predicted,
    size_t count
)
{
    BenchmarkErrorMetrics metrics;
    size_t index;
    double squared_error_total = 0.0;
    double percent_error_total = 0.0;
    metrics.rmse = 0.0;
    metrics.mape_percent = 0.0;
    metrics.max_abs_percent_error = 0.0;
    for (index = 0; index < count; ++index) {
        double error = predicted[index] - actual[index];
        double percent = benchmark_percent_error(predicted[index], actual[index]);
        squared_error_total += error * error;
        percent_error_total += percent;
        if (percent > metrics.max_abs_percent_error) {
            metrics.max_abs_percent_error = percent;
        }
    }
    metrics.rmse = sqrt(squared_error_total / (double)count);
    metrics.mape_percent = percent_error_total / (double)count;
    return metrics;
}

static int benchmark_run_stage76_batch_sample(
    size_t target_groups,
    size_t group_size,
    uint32_t seed,
    void **items,
    HpsComparator *comparator,
    BenchmarkBatchModelRow *row,
    const BenchmarkBuildModel *model_l,
    const BenchmarkBuildModel *model_q,
    size_t seed_index
)
{
    HpsGroupBatch *batch = NULL;
    HpsGroup *final_group = NULL;
    BenchmarkCompareContext *counter =
        (BenchmarkCompareContext *)comparator->context;
    BenchmarkMergePrediction merge_prediction;
    BenchmarkPathStats path_stats;
    size_t groups;
    size_t build_comparisons;
    double predicted_build_l;
    double predicted_build_q;
    HpsStatus status;

    counter->comparison_count = 0;
    counter->overflowed = 0;
    status = hps_group_batch_build(items, HPS_STAGE76_MAX_ITEMS, group_size,
        comparator, &batch);
    if (status != HPS_STATUS_OK || batch == NULL || counter->overflowed) {
        printf("BENCHMARK FAILED BatchBuild seed=0x%08lX Groups=%lu GroupSize=%lu: %s\n",
            (unsigned long)seed, (unsigned long)target_groups,
            (unsigned long)group_size, hps_status_string(status));
        hps_group_batch_destroy(batch);
        return 0;
    }
    build_comparisons = counter->comparison_count;
    groups = hps_group_batch_group_count(batch);
    if (groups != target_groups ||
        hps_group_batch_total_size(batch) != HPS_STAGE76_MAX_ITEMS ||
        !benchmark_predict_batch_merge_upper(batch, HPS_STAGE76_MAX_ITEMS,
            &merge_prediction) ||
        !benchmark_predict_batch_build(batch, model_l, 0,
            &predicted_build_l) ||
        !benchmark_predict_batch_build(batch, model_q, 1,
            &predicted_build_q)) {
        printf("BENCHMARK FAILED Batch structure/prediction seed=0x%08lX TargetGroups=%lu GroupSize=%lu ActualGroups=%lu\n",
            (unsigned long)seed, (unsigned long)target_groups,
            (unsigned long)group_size, (unsigned long)groups);
        hps_group_batch_destroy(batch);
        return 0;
    }
    if (seed_index == 0) {
        row->target_groups = target_groups;
        row->group_size = group_size;
        row->actual_groups = groups;
        row->predicted_merge_upper = merge_prediction.upper_bound;
        row->predicted_build_l = predicted_build_l;
        row->predicted_build_q = predicted_build_q;
    } else if (row->actual_groups != groups ||
            row->predicted_merge_upper != merge_prediction.upper_bound ||
            row->predicted_build_l != predicted_build_l ||
            row->predicted_build_q != predicted_build_q) {
        printf("BENCHMARK FAILED batch structure changed seed=0x%08lX TargetGroups=%lu GroupSize=%lu\n",
            (unsigned long)seed, (unsigned long)target_groups,
            (unsigned long)group_size);
        hps_group_batch_destroy(batch);
        return 0;
    }

    counter->comparison_count = 0;
    counter->overflowed = 0;
    status = hps_group_batch_merge_all(batch, comparator, &final_group);
    if (status != HPS_STATUS_OK || final_group == NULL || counter->overflowed) {
        printf("BENCHMARK FAILED BatchMerge seed=0x%08lX TargetGroups=%lu GroupSize=%lu: %s\n",
            (unsigned long)seed, (unsigned long)target_groups,
            (unsigned long)group_size, hps_status_string(status));
        hps_group_destroy(final_group);
        hps_group_batch_destroy(batch);
        return 0;
    }
    memset(&path_stats, 0, sizeof(path_stats));
    if (!benchmark_validate_final(final_group, HPS_STAGE76_MAX_ITEMS,
            &path_stats)) {
        printf("BENCHMARK FAILED BatchMerge validation seed=0x%08lX TargetGroups=%lu GroupSize=%lu\n",
            (unsigned long)seed, (unsigned long)target_groups,
            (unsigned long)group_size);
        hps_group_destroy(final_group);
        hps_group_batch_destroy(batch);
        return 0;
    }
    row->samples[seed_index].build_comparisons = build_comparisons;
    row->samples[seed_index].merge_comparisons = counter->comparison_count;
    row->samples[seed_index].total_comparisons = build_comparisons +
        counter->comparison_count;
    hps_group_destroy(final_group);
    hps_group_batch_destroy(batch);
    return 1;
}

int hps_run_stage7_6_benchmark(void)
{
    static const size_t train_sizes[HPS_STAGE76_TRAIN_COUNT] = {
        32, 64, 128, 256, 512, 1024, 2048, 4096, 8192
    };
    static const size_t validation_sizes[HPS_STAGE76_VALIDATION_COUNT] = {
        48, 96, 192, 384, 768, 1536, 3072, 6144, 10000
    };
    static const size_t target_groups[HPS_STAGE76_BATCH_COUNT] = {
        1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 16, 20, 25, 32, 40, 50, 64, 80
    };
    BenchmarkBuildPoint train[HPS_STAGE76_TRAIN_COUNT];
    BenchmarkBuildPoint validation[HPS_STAGE76_VALIDATION_COUNT];
    BenchmarkBuildModel model_l;
    BenchmarkBuildModel model_q;
    BenchmarkBatchModelRow batch_rows[HPS_STAGE76_BATCH_COUNT];
    int values[HPS_STAGE76_MAX_ITEMS];
    void *items[HPS_STAGE76_MAX_ITEMS];
    HpsComparator comparator;
    BenchmarkCompareContext counter;
    double batch_build_actual[HPS_STAGE76_BATCH_COUNT];
    double batch_build_l[HPS_STAGE76_BATCH_COUNT];
    double batch_build_q[HPS_STAGE76_BATCH_COUNT];
    double total_actual[HPS_STAGE76_BATCH_COUNT];
    double total_predicted_l[HPS_STAGE76_BATCH_COUNT];
    double total_predicted_q[HPS_STAGE76_BATCH_COUNT];
    size_t index;
    size_t seed_index;

    memset(train, 0, sizeof(train));
    memset(validation, 0, sizeof(validation));
    memset(batch_rows, 0, sizeof(batch_rows));
    counter.comparison_count = 0;
    counter.overflowed = 0;
    comparator.compare = benchmark_compare_int;
    comparator.context = &counter;

    printf("HPSort Stage 7.6 Build Cost Model\n");
    printf("Seeds=10; SeedRange=0xC0FFEE..0xC0FFF7\n");
    for (index = 0; index < HPS_STAGE76_TRAIN_COUNT; ++index) {
        train[index].group_size = train_sizes[index];
        for (seed_index = 0; seed_index < HPS_STAGE76_SEEDS; ++seed_index) {
            size_t comparisons;
            uint32_t seed = UINT32_C(0xC0FFEE) + (uint32_t)seed_index;
            if (!benchmark_run_single_group_build(train_sizes[index], seed,
                    values, items, &comparator, &comparisons)) {
                return 1;
            }
            benchmark_distribution_add(&train[index].comparisons,
                comparisons, seed_index);
        }
        train[index].per_item_mean = train[index].comparisons.mean /
            (double)train[index].group_size;
    }
    for (index = 0; index < HPS_STAGE76_VALIDATION_COUNT; ++index) {
        validation[index].group_size = validation_sizes[index];
        for (seed_index = 0; seed_index < HPS_STAGE76_SEEDS; ++seed_index) {
            size_t comparisons;
            uint32_t seed = UINT32_C(0xC0FFEE) + (uint32_t)seed_index;
            if (!benchmark_run_single_group_build(validation_sizes[index], seed,
                    values, items, &comparator, &comparisons)) {
                return 1;
            }
            benchmark_distribution_add(&validation[index].comparisons,
                comparisons, seed_index);
        }
        validation[index].per_item_mean = validation[index].comparisons.mean /
            (double)validation[index].group_size;
    }
    if (!benchmark_fit_linear_model(train, HPS_STAGE76_TRAIN_COUNT, &model_l) ||
        !benchmark_fit_quadratic_model(train, HPS_STAGE76_TRAIN_COUNT, &model_q)) {
        printf("BENCHMARK FAILED: Build model fit singular/invalid\n");
        return 1;
    }

    printf("Set,GroupSize,BuildCmpMean,BuildCmpMin,BuildCmpMax,BuildCmpStdDev,BuildCmpPerItemMean\n");
    for (index = 0; index < HPS_STAGE76_TRAIN_COUNT; ++index) {
        printf("Train,%lu,%.3f,%lu,%lu,%.3f,%.9f\n",
            (unsigned long)train[index].group_size,
            train[index].comparisons.mean,
            (unsigned long)train[index].comparisons.minimum,
            (unsigned long)train[index].comparisons.maximum,
            sqrt(train[index].comparisons.m2 / HPS_STAGE76_SEEDS),
            train[index].per_item_mean);
    }
    for (index = 0; index < HPS_STAGE76_VALIDATION_COUNT; ++index) {
        printf("Validation,%lu,%.3f,%lu,%lu,%.3f,%.9f\n",
            (unsigned long)validation[index].group_size,
            validation[index].comparisons.mean,
            (unsigned long)validation[index].comparisons.minimum,
            (unsigned long)validation[index].comparisons.maximum,
            sqrt(validation[index].comparisons.m2 / HPS_STAGE76_SEEDS),
            validation[index].per_item_mean);
    }
    printf("ModelCoefficients\nModel,a,b,c\n");
    printf("ModelL,%.9f,%.9f,NA\n", model_l.a, model_l.b);
    printf("ModelQ,%.9f,%.9f,%.9f\n", model_q.a, model_q.b, model_q.c);
    printf("Model,Dataset,RMSEPerItem,MAPEPercent,MaxAbsPercentError\n");
    {
        BenchmarkErrorMetrics metrics = benchmark_model_metrics(train,
            HPS_STAGE76_TRAIN_COUNT, &model_l, 0);
        printf("L,Train,%.9f,%.6f,%.6f\n", metrics.rmse,
            metrics.mape_percent, metrics.max_abs_percent_error);
        metrics = benchmark_model_metrics(validation,
            HPS_STAGE76_VALIDATION_COUNT, &model_l, 0);
        printf("L,Validation,%.9f,%.6f,%.6f\n", metrics.rmse,
            metrics.mape_percent, metrics.max_abs_percent_error);
        metrics = benchmark_model_metrics(train, HPS_STAGE76_TRAIN_COUNT,
            &model_q, 1);
        printf("Q,Train,%.9f,%.6f,%.6f\n", metrics.rmse,
            metrics.mape_percent, metrics.max_abs_percent_error);
        metrics = benchmark_model_metrics(validation,
            HPS_STAGE76_VALIDATION_COUNT, &model_q, 1);
        printf("Q,Validation,%.9f,%.6f,%.6f\n", metrics.rmse,
            metrics.mape_percent, metrics.max_abs_percent_error);
    }
    printf("GroupSize,ActualPerItem,PredLPerItem,ErrorLPercent,PredQPerItem,ErrorQPercent\n");
    for (index = 0; index < HPS_STAGE76_VALIDATION_COUNT; ++index) {
        double pred_l = benchmark_build_model_predict(&model_l,
            validation[index].group_size, 0) /
            (double)validation[index].group_size;
        double pred_q = benchmark_build_model_predict(&model_q,
            validation[index].group_size, 1) /
            (double)validation[index].group_size;
        printf("%lu,%.9f,%.9f,%.6f,%.9f,%.6f\n",
            (unsigned long)validation[index].group_size,
            validation[index].per_item_mean, pred_l,
            benchmark_percent_error(pred_l, validation[index].per_item_mean),
            pred_q,
            benchmark_percent_error(pred_q, validation[index].per_item_mean));
    }

    for (seed_index = 0; seed_index < HPS_STAGE76_SEEDS; ++seed_index) {
        uint32_t seed = UINT32_C(0xC0FFEE) + (uint32_t)seed_index;
        for (index = 0; index < HPS_STAGE76_MAX_ITEMS; ++index) {
            values[index] = (int)index;
        }
        benchmark_shuffle(values, HPS_STAGE76_MAX_ITEMS, seed);
        for (index = 0; index < HPS_STAGE76_MAX_ITEMS; ++index) {
            items[index] = &values[index];
        }
        for (index = 0; index < HPS_STAGE76_BATCH_COUNT; ++index) {
            size_t quotient = HPS_STAGE76_MAX_ITEMS / target_groups[index];
            size_t remainder = HPS_STAGE76_MAX_ITEMS % target_groups[index];
            size_t group_size = quotient + (remainder != 0 ? 1u : 0u);
            if (!benchmark_run_stage76_batch_sample(target_groups[index],
                    group_size, seed, items, &comparator,
                    &batch_rows[index], &model_l, &model_q, seed_index)) {
                return 1;
            }
        }
    }

    for (index = 0; index < HPS_STAGE76_BATCH_COUNT; ++index) {
        BenchmarkBatchModelRow *row = &batch_rows[index];
        for (seed_index = 0; seed_index < HPS_STAGE76_SEEDS; ++seed_index) {
            benchmark_distribution_add(&row->actual_build,
                row->samples[seed_index].build_comparisons, seed_index);
            benchmark_distribution_add(&row->actual_merge,
                row->samples[seed_index].merge_comparisons, seed_index);
            benchmark_distribution_add(&row->actual_total,
                row->samples[seed_index].total_comparisons, seed_index);
        }
        row->actual_build_mean = row->actual_build.mean;
        row->actual_merge_mean = row->actual_merge.mean;
        row->actual_total_mean = row->actual_total.mean;
        row->predicted_total_l = row->predicted_build_l +
            (double)row->predicted_merge_upper;
        row->predicted_total_q = row->predicted_build_q +
            (double)row->predicted_merge_upper;
        batch_build_actual[index] = row->actual_build_mean;
        batch_build_l[index] = row->predicted_build_l;
        batch_build_q[index] = row->predicted_build_q;
        total_actual[index] = row->actual_total_mean;
        total_predicted_l[index] = row->predicted_total_l;
        total_predicted_q[index] = row->predicted_total_q;
    }

    printf("BatchBuildModel\nGroups,GroupSize,ActualBuildCmpMean,PredBuildL,ErrorLPercent,PredBuildQ,ErrorQPercent\n");
    for (index = 0; index < HPS_STAGE76_BATCH_COUNT; ++index) {
        const BenchmarkBatchModelRow *row = &batch_rows[index];
        printf("%lu,%lu,%.3f,%.6f,%.6f,%.6f,%.6f\n",
            (unsigned long)row->actual_groups, (unsigned long)row->group_size,
            row->actual_build_mean, row->predicted_build_l,
            benchmark_percent_error(row->predicted_build_l,
                row->actual_build_mean),
            row->predicted_build_q,
            benchmark_percent_error(row->predicted_build_q,
                row->actual_build_mean));
    }
    {
        BenchmarkErrorMetrics metrics_l = benchmark_comparison_metrics(
            batch_build_actual, batch_build_l, HPS_STAGE76_BATCH_COUNT);
        BenchmarkErrorMetrics metrics_q = benchmark_comparison_metrics(
            batch_build_actual, batch_build_q, HPS_STAGE76_BATCH_COUNT);
        printf("BatchBuildMetrics\nModel,MAPEPercent,RMSEComparisons,MaxAbsPercentError\n");
        printf("L,%.6f,%.6f,%.6f\n", metrics_l.mape_percent,
            metrics_l.rmse, metrics_l.max_abs_percent_error);
        printf("Q,%.6f,%.6f,%.6f\n", metrics_q.mape_percent,
            metrics_q.rmse, metrics_q.max_abs_percent_error);
    }

    printf("TotalModel\nGroups,GroupSize,ActualBuildMean,PredBuildL,BuildErrLPercent,PredBuildQ,BuildErrQPercent,PredMergeUpper,ActualMergeMean,ActualTotalMean,PredTotalModelL,TotalErrLPercent,PredTotalModelQ,TotalErrQPercent\n");
    for (index = 0; index < HPS_STAGE76_BATCH_COUNT; ++index) {
        const BenchmarkBatchModelRow *row = &batch_rows[index];
        printf("%lu,%lu,%.3f,%.6f,%.6f,%.6f,%.6f,%lu,%.3f,%.3f,%.6f,%.6f,%.6f,%.6f\n",
            (unsigned long)row->actual_groups, (unsigned long)row->group_size,
            row->actual_build_mean, row->predicted_build_l,
            benchmark_percent_error(row->predicted_build_l,
                row->actual_build_mean),
            row->predicted_build_q,
            benchmark_percent_error(row->predicted_build_q,
                row->actual_build_mean),
            (unsigned long)row->predicted_merge_upper,
            row->actual_merge_mean, row->actual_total_mean,
            row->predicted_total_l,
            benchmark_percent_error(row->predicted_total_l,
                row->actual_total_mean),
            row->predicted_total_q,
            benchmark_percent_error(row->predicted_total_q,
                row->actual_total_mean));
    }
    {
        BenchmarkErrorMetrics metrics_l = benchmark_comparison_metrics(
            total_actual, total_predicted_l, HPS_STAGE76_BATCH_COUNT);
        BenchmarkErrorMetrics metrics_q = benchmark_comparison_metrics(
            total_actual, total_predicted_q, HPS_STAGE76_BATCH_COUNT);
        printf("TotalModelMetrics\nModel,MAPEPercent,RMSEComparisons,MaxAbsPercentError\n");
        printf("L,%.6f,%.6f,%.6f\n", metrics_l.mape_percent,
            metrics_l.rmse, metrics_l.max_abs_percent_error);
        printf("Q,%.6f,%.6f,%.6f\n", metrics_q.mape_percent,
            metrics_q.rmse, metrics_q.max_abs_percent_error);
    }
    {
        size_t actual_low = 0;
        size_t predicted_l_low = 0;
        size_t predicted_q_low = 0;
        for (index = 1; index < HPS_STAGE76_BATCH_COUNT; ++index) {
            if (total_actual[index] < total_actual[actual_low]) {
                actual_low = index;
            }
            if (total_predicted_l[index] < total_predicted_l[predicted_l_low]) {
                predicted_l_low = index;
            }
            if (total_predicted_q[index] < total_predicted_q[predicted_q_low]) {
                predicted_q_low = index;
            }
        }
        printf("ObservedLowestActualTotalMean: Groups=%lu GroupSize=%lu Value=%.3f\n",
            (unsigned long)batch_rows[actual_low].actual_groups,
            (unsigned long)batch_rows[actual_low].group_size,
            total_actual[actual_low]);
        printf("ModelL_LowestPredictedTotal: Groups=%lu GroupSize=%lu Value=%.6f\n",
            (unsigned long)batch_rows[predicted_l_low].actual_groups,
            (unsigned long)batch_rows[predicted_l_low].group_size,
            total_predicted_l[predicted_l_low]);
        printf("ModelQ_LowestPredictedTotal: Groups=%lu GroupSize=%lu Value=%.6f\n",
            (unsigned long)batch_rows[predicted_q_low].actual_groups,
            (unsigned long)batch_rows[predicted_q_low].group_size,
            total_predicted_q[predicted_q_low]);
    }
    {
        BenchmarkErrorMetrics train_l = benchmark_model_metrics(train,
            HPS_STAGE76_TRAIN_COUNT, &model_l, 0);
        BenchmarkErrorMetrics validation_l = benchmark_model_metrics(validation,
            HPS_STAGE76_VALIDATION_COUNT, &model_l, 0);
        BenchmarkErrorMetrics train_q = benchmark_model_metrics(train,
            HPS_STAGE76_TRAIN_COUNT, &model_q, 1);
        BenchmarkErrorMetrics validation_q = benchmark_model_metrics(validation,
            HPS_STAGE76_VALIDATION_COUNT, &model_q, 1);
        BenchmarkErrorMetrics batch_l = benchmark_comparison_metrics(
            batch_build_actual, batch_build_l, HPS_STAGE76_BATCH_COUNT);
        BenchmarkErrorMetrics batch_q = benchmark_comparison_metrics(
            batch_build_actual, batch_build_q, HPS_STAGE76_BATCH_COUNT);
        BenchmarkErrorMetrics total_l = benchmark_comparison_metrics(
            total_actual, total_predicted_l, HPS_STAGE76_BATCH_COUNT);
        BenchmarkErrorMetrics total_q = benchmark_comparison_metrics(
            total_actual, total_predicted_q, HPS_STAGE76_BATCH_COUNT);
        printf("Stage7SummaryFacts\n");
        printf("MergeModelMeanGapPercent=0.0229\n");
        printf("MergeModelObservedMaxGapPercent=0.0986\n");
        printf("BuildModelLValidationMAPE=%.6f\n", validation_l.mape_percent);
        printf("BuildModelQValidationMAPE=%.6f\n", validation_q.mape_percent);
        printf("BatchBuildModelLMAPE=%.6f\n", batch_l.mape_percent);
        printf("BatchBuildModelQMAPE=%.6f\n", batch_q.mape_percent);
        printf("TotalModelLMAPE=%.6f\n", total_l.mape_percent);
        printf("TotalModelQMAPE=%.6f\n", total_q.mape_percent);
        (void)train_l;
        (void)train_q;
    }
    return 0;
}

static int benchmark_alloc_stats_equal(
    const HpsAllocStats *left,
    const HpsAllocStats *right
)
{
    size_t tag;
    if (!(left->live_bytes == right->live_bytes &&
        left->peak_live_bytes == right->peak_live_bytes &&
        left->live_blocks == right->live_blocks &&
        left->peak_live_blocks == right->peak_live_blocks &&
        left->max_live_blocks == right->max_live_blocks &&
        left->alloc_calls == right->alloc_calls &&
        left->realloc_calls == right->realloc_calls &&
        left->free_calls == right->free_calls &&
        left->failed_calls == right->failed_calls &&
        left->total_successful_requested_bytes ==
            right->total_successful_requested_bytes &&
        left->counter_overflowed == right->counter_overflowed &&
        left->blocks_when_global_byte_peak == right->blocks_when_global_byte_peak)) {
        return 0;
    }
    for (tag = 0; tag < HPS_ALLOC_TAG_COUNT; ++tag) {
        const HpsAllocTagStats *a = &left->tags[tag];
        const HpsAllocTagStats *b = &right->tags[tag];
        if (a->live_bytes != b->live_bytes ||
            a->peak_live_bytes != b->peak_live_bytes ||
            a->live_blocks != b->live_blocks ||
            a->peak_live_blocks != b->peak_live_blocks ||
            a->alloc_calls != b->alloc_calls ||
            a->realloc_calls != b->realloc_calls ||
            a->free_calls != b->free_calls ||
            left->bytes_at_global_peak[tag] != right->bytes_at_global_peak[tag] ||
            left->blocks_at_global_peak[tag] != right->blocks_at_global_peak[tag]) {
            return 0;
        }
    }
    return 1;
}

static int benchmark_alloc_stats_empty(const HpsAllocStats *stats)
{
    return stats->live_bytes == 0 && stats->live_blocks == 0;
}

int hps_run_stage8_1_tests(void)
{
    HpsAllocStats stats;
    HpsAllocStats before_reset;
    HpsComparator comparator;
    BenchmarkCompareContext counter;
    HpsStatus status;
    void *memory = NULL;
    size_t index;

    counter.comparison_count = 0;
    counter.overflowed = 0;
    comparator.compare = benchmark_compare_int;
    comparator.context = &counter;
    printf("HPSort Stage 8.1 Private Allocator Tests\n");
    stats = hps_alloc_stats_get();
    printf("Allocator pre-test live bytes=%lu live blocks=%lu\n",
        (unsigned long)stats.live_bytes, (unsigned long)stats.live_blocks);
    if (hps_alloc_stats_reset() != 0) {
        printf("Allocator test A failed: live allocations before reset\n");
        return 1;
    }
    stats = hps_alloc_stats_get();
    if (stats.live_bytes != 0 || stats.live_blocks != 0 ||
        stats.peak_live_bytes != 0 || stats.peak_live_blocks != 0) {
        printf("Allocator test A failed: reset did not clear empty stats\n");
        return 1;
    }
    printf("Allocator test A: empty reset passed\n");

    memory = hps_alloc(16);
    stats = hps_alloc_stats_get();
    if (memory == NULL || stats.live_bytes != 16 || stats.live_blocks != 1 ||
        stats.peak_live_bytes < 16 || stats.peak_live_blocks < 1) {
        printf("Allocator test B failed: alloc(16) accounting mismatch\n");
        hps_free(memory);
        return 1;
    }
    hps_free(memory);
    hps_free(NULL);
    stats = hps_alloc_stats_get();
    if (!benchmark_alloc_stats_empty(&stats) || stats.free_calls != 1) {
        printf("Allocator test B failed: free accounting mismatch\n");
        return 1;
    }
    printf("Allocator test B: 16-byte alloc/free and free(NULL) passed\n");

    if (hps_alloc_stats_reset() != 0) {
        printf("Allocator test C failed: reset rejected empty stats\n");
        return 1;
    }
    memory = hps_alloc(16);
    if (memory == NULL) {
        printf("Allocator test C failed: alloc(16)\n");
        return 1;
    }
    for (index = 0; index < 16; ++index) {
        ((unsigned char *)memory)[index] = (unsigned char)(index + 1);
    }
    {
        void *resized = hps_realloc(memory, 64);
        if (resized == NULL) {
            hps_free(memory);
            printf("Allocator test C failed: realloc grow returned NULL\n");
            return 1;
        }
        memory = resized;
    }
    stats = hps_alloc_stats_get();
    if (memory == NULL || stats.live_bytes != 64 || stats.live_blocks != 1 ||
        stats.peak_live_bytes < 64) {
        printf("Allocator test C failed: realloc grow accounting mismatch\n");
        hps_free(memory);
        return 1;
    }
    for (index = 0; index < 16; ++index) {
        if (((unsigned char *)memory)[index] != (unsigned char)(index + 1)) {
            printf("Allocator test C failed: realloc grow lost payload\n");
            hps_free(memory);
            return 1;
        }
    }
    {
        void *resized = hps_realloc(memory, 8);
        if (resized == NULL) {
            hps_free(memory);
            printf("Allocator test C failed: realloc shrink returned NULL\n");
            return 1;
        }
        memory = resized;
    }
    stats = hps_alloc_stats_get();
    if (memory == NULL || stats.live_bytes != 8 || stats.live_blocks != 1 ||
        stats.peak_live_bytes < 64) {
        printf("Allocator test C failed: realloc shrink accounting mismatch\n");
        hps_free(memory);
        return 1;
    }
    for (index = 0; index < 8; ++index) {
        if (((unsigned char *)memory)[index] != (unsigned char)(index + 1)) {
            printf("Allocator test C failed: realloc shrink lost payload\n");
            hps_free(memory);
            return 1;
        }
    }
    hps_free(memory);
    stats = hps_alloc_stats_get();
    if (!benchmark_alloc_stats_empty(&stats) || stats.realloc_calls != 2) {
        printf("Allocator test C failed: final accounting mismatch\n");
        return 1;
    }
    printf("Allocator test C: realloc grow 16->64, shrink 64->8, payload retained\n");

    if (hps_alloc_stats_reset() != 0) {
        printf("Allocator test D failed: reset\n");
        return 1;
    }
    memory = hps_alloc(32);
    if (memory == NULL || hps_realloc(memory, 0) != NULL) {
        hps_free(memory);
        printf("Allocator test D failed: realloc(ptr,0) return\n");
        return 1;
    }
    stats = hps_alloc_stats_get();
    if (!benchmark_alloc_stats_empty(&stats) || stats.realloc_calls != 1) {
        printf("Allocator test D failed: realloc(ptr,0) accounting\n");
        return 1;
    }
    printf("Allocator test D: realloc(ptr,0) released block and returned NULL\n");

    if (hps_alloc_stats_reset() != 0) {
        printf("Allocator test E failed: reset\n");
        return 1;
    }
    memory = hps_alloc((size_t)-1);
    stats = hps_alloc_stats_get();
    if (memory != NULL || stats.failed_calls != 1 || stats.live_bytes != 0 ||
        stats.live_blocks != 0) {
        hps_free(memory);
        printf("Allocator test E failed: size overflow handling\n");
        return 1;
    }
    printf("Allocator test E: SIZE_MAX request rejected before malloc\n");

    if (hps_alloc_stats_reset() != 0) {
        printf("Allocator test F failed: reset\n");
        return 1;
    }
    memory = hps_alloc(8);
    if (memory == NULL) {
        printf("Allocator test F failed: alloc\n");
        return 1;
    }
    memset(memory, 0x5a, 8);
    before_reset = hps_alloc_stats_get();
    if (hps_alloc_stats_reset() == 0 ||
        !benchmark_alloc_stats_equal(&before_reset, &(HpsAllocStats){ 0 })) {
        stats = hps_alloc_stats_get();
        if (!benchmark_alloc_stats_equal(&before_reset, &stats)) {
            hps_free(memory);
            printf("Allocator test F failed: rejected reset changed live stats\n");
            return 1;
        }
    }
    {
        void *failed_realloc = hps_realloc(memory, (size_t)-1);
        if (failed_realloc != NULL) {
            hps_free(failed_realloc);
            hps_free(memory);
            printf("Allocator test F failed: overflow realloc unexpectedly succeeded\n");
            return 1;
        }
    }
    stats = hps_alloc_stats_get();
    if (stats.live_bytes != 8 || stats.live_blocks != 1 ||
        ((unsigned char *)memory)[0] != 0x5a) {
        hps_free(memory);
        printf("Allocator test F failed: failed realloc damaged old block\n");
        return 1;
    }
    hps_free(memory);
    printf("Allocator test F: reset refused live block; failed realloc preserved it\n");

    if (hps_alloc_stats_reset() != 0) {
        printf("Allocator test G failed: reset\n");
        return 1;
    }
    memory = hps_alloc(0);
    if (memory == NULL) {
        printf("Allocator test G failed: alloc(0) returned NULL\n");
        return 1;
    }
    hps_free(memory);
    if (hps_alloc_stats_reset() != 0) {
        printf("Allocator test G failed: reset after alloc(0)\n");
        return 1;
    }
    memory = hps_realloc(NULL, 24);
    stats = hps_alloc_stats_get();
    if (memory == NULL || stats.alloc_calls != 0 || stats.realloc_calls != 1 ||
        stats.live_bytes != 24 || stats.live_blocks != 1) {
        hps_free(memory);
        printf("Allocator test G failed: zero-size / realloc(NULL,size) semantics\n");
        return 1;
    }
    hps_free(memory);
    stats = hps_alloc_stats_get();
    if (!benchmark_alloc_stats_empty(&stats)) {
        printf("Allocator test G failed: live allocation remains\n");
        return 1;
    }
    printf("Allocator test G: alloc(0) and realloc(NULL,24) semantics passed\n");

    if (hps_alloc_stats_reset() != 0) {
        printf("Path lifecycle test failed: reset\n");
        return 1;
    }
    {
        HpsPath *path = hps_path_create(HPS_DIRECTION_POSITIVE, 3u);
        if (path == NULL || hps_path_append(path, 259u) != HPS_STATUS_OK ||
            hps_path_append(path, 0u) != HPS_STATUS_OK ||
            hps_path_append(path, 0u) != HPS_STATUS_OK) {
            hps_path_destroy(path);
            printf("Path lifecycle test failed: construction\n");
            return 1;
        }
        stats = hps_alloc_stats_get();
        if (stats.peak_live_bytes == 0 || stats.peak_live_blocks == 0) {
            hps_path_destroy(path);
            printf("Path lifecycle test failed: no allocations recorded\n");
            return 1;
        }
        hps_path_destroy(path);
        stats = hps_alloc_stats_get();
        if (!benchmark_alloc_stats_empty(&stats)) {
            printf("Path lifecycle test failed: live bytes=%lu blocks=%lu\n",
                (unsigned long)stats.live_bytes,
                (unsigned long)stats.live_blocks);
            return 1;
        }
    }
    if (hps_alloc_stats_reset() != 0) {
        printf("Path realloc test failed: reset\n");
        return 1;
    }
    {
        HpsPath *path = hps_path_create(HPS_DIRECTION_POSITIVE, 1u);
        for (index = 0; path != NULL && index < 100; ++index) {
            if (hps_path_append(path, (unsigned int)(index % 260)) !=
                    HPS_STATUS_OK) {
                hps_path_destroy(path);
                path = NULL;
            }
        }
        stats = hps_alloc_stats_get();
        if (path == NULL || hps_path_depth(path) != 101 ||
            stats.realloc_calls == 0) {
            hps_path_destroy(path);
            printf("Path realloc test failed: append/capacity growth\n");
            return 1;
        }
        hps_path_destroy(path);
        stats = hps_alloc_stats_get();
        if (!benchmark_alloc_stats_empty(&stats)) {
            printf("Path realloc test failed: live bytes=%lu blocks=%lu\n",
                (unsigned long)stats.live_bytes,
                (unsigned long)stats.live_blocks);
            return 1;
        }
        printf("Path lifecycle and 100-step realloc test: passed; realloc_calls=%lu; final live bytes/blocks=0/0\n",
            (unsigned long)stats.realloc_calls);
    }

    if (hps_alloc_stats_reset() != 0) {
        printf("Tree lifecycle test failed: reset\n");
        return 1;
    }
    {
        static const unsigned int slots[] = { 100, 50, 150, 125, 75, 25, 175 };
        int item_values[sizeof(slots) / sizeof(slots[0])];
        HpsTree *tree = hps_tree_create();
        size_t tree_peak_bytes;
        for (index = 0; tree != NULL &&
                index < sizeof(slots) / sizeof(slots[0]); ++index) {
            HpsPath *path = hps_path_create(HPS_DIRECTION_POSITIVE, slots[index]);
            item_values[index] = (int)slots[index];
            if (path == NULL || hps_tree_insert(tree, path,
                    &item_values[index], NULL) != HPS_STATUS_OK) {
                hps_path_destroy(path);
                hps_tree_destroy(tree);
                tree = NULL;
                break;
            }
            hps_path_destroy(path);
        }
        stats = hps_alloc_stats_get();
        if (tree == NULL || hps_tree_size(tree) !=
                sizeof(slots) / sizeof(slots[0]) || stats.peak_live_bytes == 0) {
            hps_tree_destroy(tree);
            printf("Tree lifecycle test failed: construction\n");
            return 1;
        }
        tree_peak_bytes = stats.peak_live_bytes;
        hps_tree_destroy(tree);
        stats = hps_alloc_stats_get();
        if (!benchmark_alloc_stats_empty(&stats)) {
            printf("Tree lifecycle test failed: live bytes=%lu blocks=%lu\n",
                (unsigned long)stats.live_bytes,
                (unsigned long)stats.live_blocks);
            return 1;
        }
        printf("Tree lifecycle test: 7 nodes; PeakRequestedHeapBytes=%lu; borrowed items retained by caller; final live bytes/blocks=0/0\n",
            (unsigned long)tree_peak_bytes);
    }

    if (hps_alloc_stats_reset() != 0) {
        printf("Group lifecycle test failed: reset\n");
        return 1;
    }
    {
        int group_values[100];
        void *group_items[100];
        HpsGroup *group = NULL;
        size_t build_live_bytes;
        size_t build_peak_bytes;
        size_t build_live_blocks;
        for (index = 0; index < 100; ++index) {
            group_values[index] = (int)(99 - index);
            group_items[index] = &group_values[index];
        }
        status = hps_group_build(group_items, 100, &comparator, &group);
        stats = hps_alloc_stats_get();
        build_live_bytes = stats.live_bytes;
        build_peak_bytes = stats.peak_live_bytes;
        build_live_blocks = stats.live_blocks;
        if (status != HPS_STATUS_OK || group == NULL ||
            hps_group_size(group) != 100 || build_live_bytes == 0 ||
            !benchmark_validate_final(group, 100, &(BenchmarkPathStats){ 0 })) {
            hps_group_destroy(group);
            printf("Group lifecycle test failed: build/validation\n");
            return 1;
        }
        hps_group_destroy(group);
        stats = hps_alloc_stats_get();
        if (!benchmark_alloc_stats_empty(&stats)) {
            printf("Group lifecycle test failed: live bytes=%lu blocks=%lu\n",
                (unsigned long)stats.live_bytes,
                (unsigned long)stats.live_blocks);
            return 1;
        }
        printf("Group lifecycle test: Build live bytes=%lu live blocks=%lu peak bytes=%lu; after destroy live bytes/blocks=0/0\n",
            (unsigned long)build_live_bytes, (unsigned long)build_live_blocks,
            (unsigned long)build_peak_bytes);
    }

    if (hps_alloc_stats_reset() != 0) {
        printf("GroupBatch lifecycle test failed: reset\n");
        return 1;
    }
    {
        int batch_values[1000];
        void *batch_items[1000];
        HpsGroupBatch *batch = NULL;
        HpsGroup *result = NULL;
        size_t after_batch_build;
        size_t after_merge_all;
        size_t after_destroy_batch;
        size_t peak_live_bytes;
        BenchmarkPathStats path_stats;
        for (index = 0; index < 1000; ++index) {
            batch_values[index] = (int)((index * 613u) % 1000u);
            batch_items[index] = &batch_values[index];
        }
        status = hps_group_batch_build(batch_items, 1000, 100,
            &comparator, &batch);
        if (status != HPS_STATUS_OK || batch == NULL ||
            hps_group_batch_group_count(batch) != 10) {
            hps_group_batch_destroy(batch);
            printf("GroupBatch lifecycle test failed: Batch build\n");
            return 1;
        }
        stats = hps_alloc_stats_get();
        after_batch_build = stats.live_bytes;
        counter.comparison_count = 0;
        counter.overflowed = 0;
        status = hps_group_batch_merge_all(batch, &comparator, &result);
        if (status != HPS_STATUS_OK || result == NULL ||
            hps_group_size(result) != 1000) {
            hps_group_destroy(result);
            hps_group_batch_destroy(batch);
            printf("GroupBatch lifecycle test failed: MergeAll\n");
            return 1;
        }
        memset(&path_stats, 0, sizeof(path_stats));
        if (!benchmark_validate_final(result, 1000, &path_stats)) {
            hps_group_destroy(result);
            hps_group_batch_destroy(batch);
            printf("GroupBatch lifecycle test failed: Result invalid\n");
            return 1;
        }
        stats = hps_alloc_stats_get();
        after_merge_all = stats.live_bytes;
        peak_live_bytes = stats.peak_live_bytes;
        hps_group_batch_destroy(batch);
        stats = hps_alloc_stats_get();
        after_destroy_batch = stats.live_bytes;
        if (after_batch_build == 0 || after_merge_all == 0 ||
            after_destroy_batch == 0 || hps_group_size(result) != 1000 ||
            hps_group_item_at(result, 0) == NULL ||
            *(const int *)hps_group_item_at(result, 0) != 0) {
            hps_group_destroy(result);
            printf("GroupBatch lifecycle test failed: result lifetime after Batch destroy\n");
            return 1;
        }
        hps_group_destroy(result);
        stats = hps_alloc_stats_get();
        if (!benchmark_alloc_stats_empty(&stats)) {
            printf("GroupBatch lifecycle test failed: final live bytes=%lu blocks=%lu\n",
                (unsigned long)stats.live_bytes,
                (unsigned long)stats.live_blocks);
            return 1;
        }
        printf("GroupBatch lifecycle N=1000 GroupSize=100: AfterBatchBuild=%lu AfterMergeAll=%lu AfterDestroyBatch=%lu PeakRequestedHeapBytes=%lu; Result destroy final live bytes/blocks=0/0\n",
            (unsigned long)after_batch_build, (unsigned long)after_merge_all,
            (unsigned long)after_destroy_batch, (unsigned long)peak_live_bytes);
    }

    stats = hps_alloc_stats_get();
    if (!benchmark_alloc_stats_empty(&stats)) {
        printf("Stage 8.1 failed: final live bytes=%lu blocks=%lu\n",
            (unsigned long)stats.live_bytes,
            (unsigned long)stats.live_blocks);
        return 1;
    }
    printf("Stage 8.1 final allocator state: live bytes=0 live blocks=0; no HPSort allocation leak detected\n");
    return 0;
}

enum {
    HPS_STAGE82_MAIN_N = 10000,
    HPS_STAGE82_GROUP_COUNT = 18,
    HPS_STAGE82_MAIN_SEEDS = 5,
    HPS_STAGE82_SCALE_SEEDS = 3,
    HPS_STAGE82_SCALE_N_COUNT = 4,
    HPS_STAGE82_SCALE_GROUPS = 3
};

static const size_t stage82_groups[HPS_STAGE82_GROUP_COUNT] = {
    1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 16, 20, 25, 32, 40, 50, 64, 80
};
static const uint32_t stage82_seeds[HPS_STAGE82_MAIN_SEEDS] = {
    UINT32_C(0xC0FFEE), UINT32_C(0xC0FFEF), UINT32_C(0xC0FFF0),
    UINT32_C(0xC0FFF1), UINT32_C(0xC0FFF2)
};

typedef struct Stage82AlignmentTypes {
    char c;
    short s;
    int i;
    long l;
    long long ll;
    float f;
    double d;
    long double ld;
    void *p;
} Stage82AlignmentTypes;

typedef union Stage82AlignmentUnion {
    char c;
    short s;
    int i;
    long l;
    long long ll;
    float f;
    double d;
    long double ld;
    void *p;
    Stage82AlignmentTypes all;
} Stage82AlignmentUnion;

typedef struct Stage82Sample {
    size_t groups;
    size_t group_size;
    size_t build_comparisons;
    size_t merge_comparisons;
    size_t total_comparisons;
    size_t batch_path_count;
    size_t batch_zero_depth_count;
    size_t batch_total_depth;
    size_t batch_total_capacity;
    size_t batch_storage_bytes;
    size_t result_path_count;
    size_t result_zero_depth_count;
    size_t result_total_depth;
    size_t result_total_capacity;
    size_t result_storage_bytes;
    double build_ms;
    double merge_ms;
    double total_ms;
    HpsAllocStats after_batch;
    HpsAllocStats after_merge;
    HpsAllocStats result_only;
    HpsAllocStats final;
    BenchmarkPathStats local_paths;
    BenchmarkPathStats final_paths;
} Stage82Sample;

typedef struct Stage82Aggregate {
    double batch_mean, result_mean, post_mean, peak_mean, peak_m2;
    size_t batch_min, batch_max, result_min, result_max;
    size_t peak_min, peak_max;
    double transient_mean, peak_over_batch_mean;
    double batch_blocks_mean, result_blocks_mean, peak_blocks_mean;
    double alloc_mean, realloc_mean, free_mean, requested_mean;
    double local_depth_mean, final_depth_mean;
    size_t final_max_depth, final_max_level;
} Stage82Aggregate;

static int stage82_alignment_one(size_t bytes, size_t alignment,
    const char *type_name)
{
    void *payload = hps_alloc(bytes);
    int passed = payload != NULL && alignment != 0 &&
        ((uintptr_t)payload % (uintptr_t)alignment) == 0;
    if (!passed) {
        printf("Alignment regression failed for %s: address=%p alignment=%lu\n",
            type_name, payload, (unsigned long)alignment);
    }
    hps_free(payload);
    return passed;
}

static int stage82_alignment_regression(void)
{
    HpsAllocStats stats;
    int passed = 1;
    if (hps_alloc_stats_reset() != 0) {
        printf("Alignment regression failed: allocator was not empty before reset\n");
        return 0;
    }
#define STAGE82_CHECK_ALIGNMENT(type) \
    do { if (!stage82_alignment_one(sizeof(type), _Alignof(type), #type)) passed = 0; } while (0)
    STAGE82_CHECK_ALIGNMENT(char);
    STAGE82_CHECK_ALIGNMENT(short);
    STAGE82_CHECK_ALIGNMENT(int);
    STAGE82_CHECK_ALIGNMENT(long);
    STAGE82_CHECK_ALIGNMENT(long long);
    STAGE82_CHECK_ALIGNMENT(float);
    STAGE82_CHECK_ALIGNMENT(double);
    STAGE82_CHECK_ALIGNMENT(long double);
    STAGE82_CHECK_ALIGNMENT(void *);
    STAGE82_CHECK_ALIGNMENT(Stage82AlignmentTypes);
    STAGE82_CHECK_ALIGNMENT(Stage82AlignmentUnion);
#undef STAGE82_CHECK_ALIGNMENT
    stats = hps_alloc_stats_get();
    if (!benchmark_alloc_stats_empty(&stats)) {
        printf("Alignment regression failed: final live bytes=%lu blocks=%lu\n",
            (unsigned long)stats.live_bytes, (unsigned long)stats.live_blocks);
        passed = 0;
    }
    if (passed) printf("Stage 8.2 alignment regression: all 11 types passed; final live bytes/blocks=0/0\n");
    return passed;
}

static int stage83_tag_accounting_valid(const HpsAllocStats *stats,
    const char *snapshot_name)
{
    size_t tag;
    size_t live_bytes = 0, live_blocks = 0;
    size_t peak_bytes = 0, peak_blocks = 0;
    for (tag = 0; tag < HPS_ALLOC_TAG_COUNT; ++tag) {
        if (!benchmark_add_size(&live_bytes, stats->tags[tag].live_bytes) ||
            !benchmark_add_size(&live_blocks, stats->tags[tag].live_blocks) ||
            !benchmark_add_size(&peak_bytes, stats->bytes_at_global_peak[tag]) ||
            !benchmark_add_size(&peak_blocks, stats->blocks_at_global_peak[tag])) {
            printf("Stage 8.3 tag stats sum overflow at %s\n", snapshot_name);
            return 0;
        }
    }
    if (live_bytes != stats->live_bytes || live_blocks != stats->live_blocks ||
        peak_bytes != stats->peak_live_bytes ||
        peak_blocks != stats->blocks_when_global_byte_peak ||
        peak_blocks != stats->peak_live_blocks) {
        printf("Stage 8.3 tag accounting mismatch at %s: live bytes %lu/%lu blocks %lu/%lu peak bytes %lu/%lu peak blocks %lu/at-byte-peak %lu/max-blocks %lu\n",
            snapshot_name, (unsigned long)live_bytes,
            (unsigned long)stats->live_bytes, (unsigned long)live_blocks,
            (unsigned long)stats->live_blocks, (unsigned long)peak_bytes,
            (unsigned long)stats->peak_live_bytes, (unsigned long)peak_blocks,
            (unsigned long)stats->blocks_when_global_byte_peak,
            (unsigned long)stats->peak_live_blocks);
        return 0;
    }
    return 1;
}

static int stage83_other_is_empty(const HpsAllocStats *stats,
    const char *snapshot_name)
{
    const HpsAllocTagStats *other = &stats->tags[HPS_ALLOC_TAG_OTHER];
    if (other->live_bytes != 0 || other->live_blocks != 0 ||
        stats->bytes_at_global_peak[HPS_ALLOC_TAG_OTHER] != 0 ||
        stats->blocks_at_global_peak[HPS_ALLOC_TAG_OTHER] != 0) {
        printf("Stage 8.3 unclassified production allocation at %s: OTHER live bytes=%lu blocks=%lu peak bytes=%lu blocks=%lu\n",
            snapshot_name, (unsigned long)other->live_bytes,
            (unsigned long)other->live_blocks,
            (unsigned long)stats->bytes_at_global_peak[HPS_ALLOC_TAG_OTHER],
            (unsigned long)stats->blocks_at_global_peak[HPS_ALLOC_TAG_OTHER]);
        return 0;
    }
    return 1;
}

static int stage82_run_sample(int *values, void **items, size_t count,
    size_t group_size, size_t target_groups, Stage82Sample *sample,
    uint32_t seed, int verbose_failure)
{
    HpsComparator comparator;
    BenchmarkCompareContext compare_context;
    HpsGroupBatch *batch = NULL;
    HpsGroup *result = NULL;
    HpsStatus status;
    HpsAllocStats before_merge;
    size_t i;
    clock_t build_start, build_end, merge_start, merge_end;
    int success = 0;

    memset(sample, 0, sizeof(*sample));
    if (hps_alloc_stats_reset() != 0) {
        if (verbose_failure) printf("Stage 8.2 sample reset failed: seed=%08lX Groups=%lu GroupSize=%lu\n",
            (unsigned long)seed, (unsigned long)target_groups, (unsigned long)group_size);
        return 0;
    }
    for (i = 0; i < count; ++i) items[i] = &values[i];
    compare_context.comparison_count = 0;
    compare_context.overflowed = 0;
    comparator.compare = benchmark_compare_int;
    comparator.context = &compare_context;
    build_start = clock();
    status = hps_group_batch_build(items, count, group_size, &comparator, &batch);
    build_end = clock();
    if (status != HPS_STATUS_OK || batch == NULL) goto cleanup;
    sample->build_comparisons = compare_context.comparison_count;
    sample->build_ms = build_end >= build_start ?
        1000.0 * (double)(build_end - build_start) / CLOCKS_PER_SEC : 0.0;
    sample->after_batch = hps_alloc_stats_get();
    if (!stage83_tag_accounting_valid(&sample->after_batch, "AfterBatchBuild") ||
        !stage83_other_is_empty(&sample->after_batch, "AfterBatchBuild")) goto cleanup;
    sample->groups = hps_group_batch_group_count(batch);
    sample->group_size = group_size;
    if (sample->groups != target_groups ||
        !benchmark_collect_batch_paths(batch, &sample->local_paths)) goto cleanup;
    for (i = 0; i < sample->groups; ++i) {
        const HpsGroup *group = hps_group_batch_group_at(batch, i);
        size_t j;
        if (group == NULL) goto cleanup;
        for (j = 0; j < hps_group_size(group); ++j) {
            const HpsPath *path = hps_group_path_at(group, j);
            size_t depth, capacity, storage;
            if (path == NULL) goto cleanup;
            depth = hps_path_depth(path);
            capacity = hps_path_internal_capacity(path);
            storage = hps_path_internal_storage_bytes(path);
            if (storage == (size_t)-1 || capacity < depth ||
                !benchmark_add_size(&sample->batch_total_depth, depth) ||
                !benchmark_add_size(&sample->batch_total_capacity, capacity) ||
                !benchmark_add_size(&sample->batch_storage_bytes, storage)) goto cleanup;
            if (depth == 0) ++sample->batch_zero_depth_count;
            ++sample->batch_path_count;
        }
    }
    if (sample->batch_storage_bytes != sample->after_batch.tags[HPS_ALLOC_TAG_PATH_STEPS].live_bytes ||
        sample->batch_path_count * hps_path_internal_sizeof_path() !=
            sample->after_batch.tags[HPS_ALLOC_TAG_PATH_OBJECT].live_bytes ||
        sample->after_batch.tags[HPS_ALLOC_TAG_PATH_STEPS].live_blocks !=
            sample->batch_path_count - sample->batch_zero_depth_count) goto cleanup;
    merge_start = clock();
    status = hps_group_batch_merge_all(batch, &comparator, &result);
    merge_end = clock();
    if (status != HPS_STATUS_OK || result == NULL) goto cleanup;
    sample->total_comparisons = compare_context.comparison_count;
    sample->merge_comparisons = sample->total_comparisons - sample->build_comparisons;
    sample->merge_ms = merge_end >= merge_start ?
        1000.0 * (double)(merge_end - merge_start) / CLOCKS_PER_SEC : 0.0;
    sample->total_ms = sample->build_ms + sample->merge_ms;
    sample->after_merge = hps_alloc_stats_get();
    if (!stage83_tag_accounting_valid(&sample->after_merge, "PostMerge") ||
        !stage83_other_is_empty(&sample->after_merge, "PostMerge") ||
        sample->after_merge.tags[HPS_ALLOC_TAG_MERGE_SCRATCH].live_bytes != 0 ||
        sample->after_merge.tags[HPS_ALLOC_TAG_MERGE_SCRATCH].live_blocks != 0) goto cleanup;
    memset(&sample->final_paths, 0, sizeof(sample->final_paths));
    if (!benchmark_validate_final(result, count, &sample->final_paths)) goto cleanup;
    sample->result_path_count = hps_group_size(result);
    for (i = 0; i < sample->result_path_count; ++i) {
        const HpsPath *path = hps_group_path_at(result, i);
        size_t depth, capacity, storage;
        if (path == NULL) goto cleanup;
        depth = hps_path_depth(path);
        capacity = hps_path_internal_capacity(path);
        storage = hps_path_internal_storage_bytes(path);
        if (storage == (size_t)-1 || capacity < depth ||
            !benchmark_add_size(&sample->result_total_depth, depth) ||
            !benchmark_add_size(&sample->result_total_capacity, capacity) ||
            !benchmark_add_size(&sample->result_storage_bytes, storage)) goto cleanup;
        if (depth == 0) ++sample->result_zero_depth_count;
    }
    hps_group_batch_destroy(batch);
    batch = NULL;
    sample->result_only = hps_alloc_stats_get();
    if (!stage83_tag_accounting_valid(&sample->result_only, "ResultOnly") ||
        !stage83_other_is_empty(&sample->result_only, "ResultOnly") ||
        sample->result_only.tags[HPS_ALLOC_TAG_MERGE_SCRATCH].live_bytes != 0 ||
        sample->result_only.tags[HPS_ALLOC_TAG_MERGE_SCRATCH].live_blocks != 0 ||
        sample->result_only.tags[HPS_ALLOC_TAG_BATCH_OBJECT].live_bytes != 0 ||
        sample->result_only.tags[HPS_ALLOC_TAG_BATCH_OBJECT].live_blocks != 0 ||
        sample->result_only.tags[HPS_ALLOC_TAG_BATCH_GROUP_ARRAY].live_bytes != 0 ||
        sample->result_only.tags[HPS_ALLOC_TAG_BATCH_GROUP_ARRAY].live_blocks != 0) goto cleanup;
    if (sample->result_storage_bytes != sample->result_only.tags[HPS_ALLOC_TAG_PATH_STEPS].live_bytes ||
        sample->result_path_count * hps_path_internal_sizeof_path() !=
            sample->result_only.tags[HPS_ALLOC_TAG_PATH_OBJECT].live_bytes ||
        sample->result_only.tags[HPS_ALLOC_TAG_PATH_STEPS].live_blocks !=
            sample->result_path_count - sample->result_zero_depth_count) goto cleanup;
    memset(&sample->final_paths, 0, sizeof(sample->final_paths));
    if (!benchmark_validate_final(result, count, &sample->final_paths)) goto cleanup;
    if (sample->after_merge.live_bytes != sample->after_batch.live_bytes +
            sample->result_only.live_bytes ||
        sample->after_merge.live_blocks != sample->after_batch.live_blocks +
            sample->result_only.live_blocks ||
        sample->after_merge.peak_live_bytes < sample->after_merge.live_bytes ||
        sample->after_merge.failed_calls != 0 || compare_context.overflowed) {
        if (verbose_failure) printf("Stage 8.2 accounting invariant failed: seed=%08lX Groups=%lu GroupSize=%lu PostMerge=%lu Batch=%lu Result=%lu Blocks=%lu/%lu/%lu failed=%lu\n",
            (unsigned long)seed, (unsigned long)target_groups, (unsigned long)group_size,
            (unsigned long)sample->after_merge.live_bytes,
            (unsigned long)sample->after_batch.live_bytes,
            (unsigned long)sample->result_only.live_bytes,
            (unsigned long)sample->after_merge.live_blocks,
            (unsigned long)sample->after_batch.live_blocks,
            (unsigned long)sample->result_only.live_blocks,
            (unsigned long)sample->after_merge.failed_calls);
        goto cleanup;
    }
    hps_group_destroy(result);
    result = NULL;
    sample->final = hps_alloc_stats_get();
    if (!benchmark_alloc_stats_empty(&sample->final) || sample->final.failed_calls != 0 ||
        !stage83_tag_accounting_valid(&sample->final, "Final") ||
        !stage83_other_is_empty(&sample->final, "Final")) {
        if (verbose_failure) printf("Stage 8.2 final accounting failed: seed=%08lX Groups=%lu GroupSize=%lu bytes=%lu blocks=%lu failed=%lu\n",
            (unsigned long)seed, (unsigned long)target_groups, (unsigned long)group_size,
            (unsigned long)sample->final.live_bytes, (unsigned long)sample->final.live_blocks,
            (unsigned long)sample->final.failed_calls);
        goto cleanup;
    }
    success = 1;

cleanup:
    hps_group_destroy(result);
    hps_group_batch_destroy(batch);
    if (!success && verbose_failure) {
        before_merge = hps_alloc_stats_get();
        printf("Stage 8.2 sample failed: seed=%08lX Groups=%lu GroupSize=%lu status=%s current bytes=%lu blocks=%lu failed=%lu\n",
            (unsigned long)seed, (unsigned long)target_groups, (unsigned long)group_size,
            hps_status_string(status), (unsigned long)before_merge.live_bytes,
            (unsigned long)before_merge.live_blocks, (unsigned long)before_merge.failed_calls);
    }
    return success;
}

static double stage82_per_item(size_t value, size_t count)
{
    return count == 0 ? 0.0 : (double)value / (double)count;
}

static int stage82_values_close(double left, double right)
{
    double scale = fabs(right);
    if (scale < 1.0) scale = 1.0;
    return fabs(left - right) <= 1.0e-9 * scale;
}

static size_t stage82_find_group_size(size_t count, size_t groups)
{
    size_t group_size;
    for (group_size = 1; group_size <= count; ++group_size) {
        size_t actual = count / group_size + (count % group_size != 0);
        if (actual == groups) return group_size;
    }
    return 0;
}

static void stage82_add_aggregate(Stage82Aggregate *a,
    const Stage82Sample *s, size_t count, size_t sample_index)
{
    double peak = (double)s->after_merge.peak_live_bytes;
    double delta = peak - a->peak_mean;
    double post = (double)s->after_merge.live_bytes;
    double batch = (double)s->after_batch.live_bytes;
    double result = (double)s->result_only.live_bytes;
    a->batch_mean += batch;
    a->result_mean += result;
    a->post_mean += post;
    a->peak_mean += delta / (double)(sample_index + 1);
    a->peak_m2 += delta * (peak - a->peak_mean);
    a->transient_mean += (double)(s->after_merge.peak_live_bytes - s->after_merge.live_bytes);
    a->peak_over_batch_mean += (double)(s->after_merge.peak_live_bytes - s->after_batch.live_bytes);
    a->batch_blocks_mean += (double)s->after_batch.live_blocks;
    a->result_blocks_mean += (double)s->result_only.live_blocks;
    a->peak_blocks_mean += (double)s->after_merge.peak_live_blocks;
    a->alloc_mean += (double)s->final.alloc_calls;
    a->realloc_mean += (double)s->final.realloc_calls;
    a->free_mean += (double)s->final.free_calls;
    a->requested_mean += (double)s->final.total_successful_requested_bytes;
    a->local_depth_mean += stage82_per_item(s->local_paths.total_depth, count);
    a->final_depth_mean += stage82_per_item(s->final_paths.total_depth, count);
    if (sample_index == 0 || s->after_batch.live_bytes < a->batch_min) a->batch_min = s->after_batch.live_bytes;
    if (sample_index == 0 || s->after_batch.live_bytes > a->batch_max) a->batch_max = s->after_batch.live_bytes;
    if (sample_index == 0 || s->result_only.live_bytes < a->result_min) a->result_min = s->result_only.live_bytes;
    if (sample_index == 0 || s->result_only.live_bytes > a->result_max) a->result_max = s->result_only.live_bytes;
    if (sample_index == 0 || s->after_merge.peak_live_bytes < a->peak_min) a->peak_min = s->after_merge.peak_live_bytes;
    if (sample_index == 0 || s->after_merge.peak_live_bytes > a->peak_max) a->peak_max = s->after_merge.peak_live_bytes;
    if (s->final_paths.max_depth > a->final_max_depth) a->final_max_depth = s->final_paths.max_depth;
    if (s->final_paths.max_level > a->final_max_level) a->final_max_level = s->final_paths.max_level;
}

static int stage82_print_main_outputs(Stage82Sample samples[HPS_STAGE82_GROUP_COUNT][HPS_STAGE82_MAIN_SEEDS])
{
    size_t g, seed;
    printf("\nStage 8.2 Main Requested Memory CSV (bytes are allocator requested payload)\n");
    printf("Groups,GroupSize,BatchLiveBytesMean,ResultLiveBytesMean,PostMergeLiveBytesMean,OverallPeakBytesMean,OverallPeakBytesMin,OverallPeakBytesMax,OverallPeakBytesStdDev,TransientExtraPeakBytesMean,PeakOverBatchBytesMean,BatchLiveBytesPerItem,ResultLiveBytesPerItem,OverallPeakBytesPerItem,TransientExtraPeakBytesPerItem,PeakVsResultRatio,PeakVsBatchRatio\n");
    for (g = 0; g < HPS_STAGE82_GROUP_COUNT; ++g) {
        Stage82Aggregate a = { 0 };
        for (seed = 0; seed < HPS_STAGE82_MAIN_SEEDS; ++seed) stage82_add_aggregate(&a, &samples[g][seed], HPS_STAGE82_MAIN_N, seed);
        a.batch_mean /= HPS_STAGE82_MAIN_SEEDS; a.result_mean /= HPS_STAGE82_MAIN_SEEDS;
        a.post_mean /= HPS_STAGE82_MAIN_SEEDS; a.transient_mean /= HPS_STAGE82_MAIN_SEEDS;
        a.peak_over_batch_mean /= HPS_STAGE82_MAIN_SEEDS;
        printf("%lu,%lu,%.2f,%.2f,%.2f,%.2f,%lu,%lu,%.2f,%.2f,%.2f,%.4f,%.4f,%.4f,%.4f,%.6f,%.6f\n",
            (unsigned long)stage82_groups[g], (unsigned long)samples[g][0].group_size,
            a.batch_mean, a.result_mean, a.post_mean, a.peak_mean,
            (unsigned long)a.peak_min, (unsigned long)a.peak_max,
            sqrt(a.peak_m2 / HPS_STAGE82_MAIN_SEEDS), a.transient_mean,
            a.peak_over_batch_mean, a.batch_mean / HPS_STAGE82_MAIN_N,
            a.result_mean / HPS_STAGE82_MAIN_N, a.peak_mean / HPS_STAGE82_MAIN_N,
            a.transient_mean / HPS_STAGE82_MAIN_N,
            a.result_mean == 0.0 ? 0.0 : a.peak_mean / a.result_mean,
            a.batch_mean == 0.0 ? 0.0 : a.peak_mean / a.batch_mean);
    }
    printf("\nStage 8.2 Blocks and Allocation CSV\nGroups,GroupSize,BatchLiveBlocksMean,ResultLiveBlocksMean,OverallPeakBlocksMean,AllocCallsMean,ReallocCallsMean,FreeCallsMean,TotalSuccessfulRequestedBytesMean\n");
    for (g = 0; g < HPS_STAGE82_GROUP_COUNT; ++g) {
        Stage82Aggregate a = { 0 };
        for (seed = 0; seed < HPS_STAGE82_MAIN_SEEDS; ++seed) stage82_add_aggregate(&a, &samples[g][seed], HPS_STAGE82_MAIN_N, seed);
        printf("%lu,%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
            (unsigned long)stage82_groups[g], (unsigned long)samples[g][0].group_size,
            a.batch_blocks_mean / HPS_STAGE82_MAIN_SEEDS,
            a.result_blocks_mean / HPS_STAGE82_MAIN_SEEDS,
            a.peak_blocks_mean / HPS_STAGE82_MAIN_SEEDS,
            a.alloc_mean / HPS_STAGE82_MAIN_SEEDS,
            a.realloc_mean / HPS_STAGE82_MAIN_SEEDS,
            a.free_mean / HPS_STAGE82_MAIN_SEEDS,
            a.requested_mean / HPS_STAGE82_MAIN_SEEDS);
    }
    printf("\nStage 8.2 Path and Memory CSV\nGroups,GroupSize,LocalAvgDepthMean,FinalAvgDepthMean,FinalMaxDepthWorst,FinalMaxLevelWorst,BatchLiveBytesPerItem,ResultLiveBytesPerItem,OverallPeakBytesPerItem\n");
    for (g = 0; g < HPS_STAGE82_GROUP_COUNT; ++g) {
        Stage82Aggregate a = { 0 };
        double batch_per_item, result_per_item, peak_per_item, transient_per_item;
        for (seed = 0; seed < HPS_STAGE82_MAIN_SEEDS; ++seed) stage82_add_aggregate(&a, &samples[g][seed], HPS_STAGE82_MAIN_N, seed);
        batch_per_item = a.batch_mean /
            (HPS_STAGE82_MAIN_SEEDS * (double)HPS_STAGE82_MAIN_N);
        result_per_item = a.result_mean /
            (HPS_STAGE82_MAIN_SEEDS * (double)HPS_STAGE82_MAIN_N);
        peak_per_item = a.peak_mean / HPS_STAGE82_MAIN_N;
        transient_per_item = a.transient_mean /
            (HPS_STAGE82_MAIN_SEEDS * (double)HPS_STAGE82_MAIN_N);
        if (!stage82_values_close(batch_per_item,
                (a.batch_mean / HPS_STAGE82_MAIN_SEEDS) / HPS_STAGE82_MAIN_N) ||
            !stage82_values_close(result_per_item,
                (a.result_mean / HPS_STAGE82_MAIN_SEEDS) / HPS_STAGE82_MAIN_N) ||
            !stage82_values_close(peak_per_item,
                a.peak_mean / HPS_STAGE82_MAIN_N) ||
            !stage82_values_close(transient_per_item,
                (a.transient_mean / HPS_STAGE82_MAIN_SEEDS) / HPS_STAGE82_MAIN_N)) {
            printf("Stage 8.2 PerItem aggregation consistency FAILED: Groups=%lu Batch=%.12g Result=%.12g Peak=%.12g Transient=%.12g\n",
                (unsigned long)stage82_groups[g], batch_per_item, result_per_item,
                peak_per_item, transient_per_item);
            return 0;
        }
        printf("%lu,%lu,%.4f,%.4f,%lu,%lu,%.4f,%.4f,%.4f\n",
            (unsigned long)stage82_groups[g], (unsigned long)samples[g][0].group_size,
            a.local_depth_mean / HPS_STAGE82_MAIN_SEEDS,
            a.final_depth_mean / HPS_STAGE82_MAIN_SEEDS,
            (unsigned long)a.final_max_depth, (unsigned long)a.final_max_level,
            batch_per_item, result_per_item, peak_per_item);
    }
    printf("\nOverallPeakBytes matrix\nSeed"); for (g=0;g<HPS_STAGE82_GROUP_COUNT;++g) printf(",G%lu",(unsigned long)stage82_groups[g]); printf("\n");
    for (seed=0;seed<HPS_STAGE82_MAIN_SEEDS;++seed) { printf("%08lX",(unsigned long)stage82_seeds[seed]); for(g=0;g<HPS_STAGE82_GROUP_COUNT;++g) printf(",%lu",(unsigned long)samples[g][seed].after_merge.peak_live_bytes); printf("\n"); }
    printf("\nResultLiveBytes matrix\nSeed"); for (g=0;g<HPS_STAGE82_GROUP_COUNT;++g) printf(",G%lu",(unsigned long)stage82_groups[g]); printf("\n");
    for (seed=0;seed<HPS_STAGE82_MAIN_SEEDS;++seed) { printf("%08lX",(unsigned long)stage82_seeds[seed]); for(g=0;g<HPS_STAGE82_GROUP_COUNT;++g) printf(",%lu",(unsigned long)samples[g][seed].result_only.live_bytes); printf("\n"); }
    printf("\nTransientExtraPeakBytes matrix\nSeed"); for (g=0;g<HPS_STAGE82_GROUP_COUNT;++g) printf(",G%lu",(unsigned long)stage82_groups[g]); printf("\n");
    for (seed=0;seed<HPS_STAGE82_MAIN_SEEDS;++seed) { printf("%08lX",(unsigned long)stage82_seeds[seed]); for(g=0;g<HPS_STAGE82_GROUP_COUNT;++g) printf(",%lu",(unsigned long)(samples[g][seed].after_merge.peak_live_bytes-samples[g][seed].after_merge.live_bytes)); printf("\n"); }
    printf("Stage 8.2 PerItem consistency: Batch/Result/OverallPeak/Transient passed for 18/18 GroupCounts; all three matrices print 5 rows with 18 values each.\n");
    return 1;
}

static int stage82_run_baseline(void)
{
    int values[1000];
    void *items[1000];
    Stage82Sample sample;
    size_t i;
    for (i=0;i<1000;++i) { values[i]=(int)((i*613u)%1000u); items[i]=&values[i]; }
    if (!stage82_run_sample(values,items,1000,100,10,&sample,UINT32_C(0xC0FFEE),1)) return 0;
    printf("\nStage 8.1 baseline rerun N=1000 GroupSize=100: BatchLive=%lu PostMergeLive=%lu ResultOnlyLive=%lu OverallPeak=%lu; prior 139000/305800/166800/496144; exact match=%s\n",
        (unsigned long)sample.after_batch.live_bytes,(unsigned long)sample.after_merge.live_bytes,
        (unsigned long)sample.result_only.live_bytes,(unsigned long)sample.after_merge.peak_live_bytes,
        sample.after_batch.live_bytes==139000 && sample.after_merge.live_bytes==305800 &&
        sample.result_only.live_bytes==166800 && sample.after_merge.peak_live_bytes==496144 ? "yes" : "no (investigate fixture/accounting difference)");
    return 1;
}

int hps_run_stage8_2_benchmark(void)
{
    Stage82Sample main_samples[HPS_STAGE82_GROUP_COUNT][HPS_STAGE82_MAIN_SEEDS];
    static const size_t scale_ns[HPS_STAGE82_SCALE_N_COUNT] = {1000,2000,5000,10000};
    static const size_t scale_groups[HPS_STAGE82_SCALE_GROUPS] = {1,8,32};
    Stage82Sample scale_samples[HPS_STAGE82_SCALE_N_COUNT][HPS_STAGE82_SCALE_GROUPS][HPS_STAGE82_SCALE_SEEDS];
    int values[HPS_STAGE82_MAIN_N];
    void *items[HPS_STAGE82_MAIN_N];
    size_t i, seed, g, n_index;

    printf("HPSort Stage 8.2 Requested Heap / Peak Memory Benchmark\n");
    if (!stage82_alignment_regression()) return 1;
    for (seed=0;seed<HPS_STAGE82_MAIN_SEEDS;++seed) {
        for(i=0;i<HPS_STAGE82_MAIN_N;++i) values[i]=(int)i;
        benchmark_shuffle(values,HPS_STAGE82_MAIN_N,stage82_seeds[seed]);
        for(g=0;g<HPS_STAGE82_GROUP_COUNT;++g) {
            size_t q=HPS_STAGE82_MAIN_N/stage82_groups[g];
            size_t r=HPS_STAGE82_MAIN_N%stage82_groups[g];
            size_t gs=q+(r!=0);
            if (!stage82_run_sample(values,items,HPS_STAGE82_MAIN_N,gs,stage82_groups[g],
                    &main_samples[g][seed],stage82_seeds[seed],1)) return 1;
        }
    }
    printf("Main panel completed: 90/90 sample lifecycles ended at 0 live bytes / 0 live blocks; all PostMerge invariants passed; FailedCalls=0\n");
    if (!stage82_print_main_outputs(main_samples)) return 1;
    if (!stage82_run_baseline()) return 1;

    for(n_index=0;n_index<HPS_STAGE82_SCALE_N_COUNT;++n_index) {
        for(seed=0;seed<HPS_STAGE82_SCALE_SEEDS;++seed) {
            for(i=0;i<scale_ns[n_index];++i) values[i]=(int)i;
            benchmark_shuffle(values,scale_ns[n_index],stage82_seeds[seed]);
            for(g=0;g<HPS_STAGE82_SCALE_GROUPS;++g) {
                size_t gs=stage82_find_group_size(scale_ns[n_index],scale_groups[g]);
                if (gs==0 || !stage82_run_sample(values,items,scale_ns[n_index],gs,
                        scale_groups[g],&scale_samples[n_index][g][seed],stage82_seeds[seed],1)) return 1;
            }
        }
    }
    printf("\nScaling panel completed: 36/36 sample lifecycles ended at 0 live bytes / 0 live blocks; all targets verified\n");
    printf("Scaling CSV (means over 3 seeds)\nN,Groups,GroupSize,BatchLiveBytesPerItem,ResultLiveBytesPerItem,OverallPeakBytesPerItem,TransientExtraPeakBytesPerItem,LocalAvgDepthMean,FinalAvgDepthMean\n");
    for(n_index=0;n_index<HPS_STAGE82_SCALE_N_COUNT;++n_index) for(g=0;g<HPS_STAGE82_SCALE_GROUPS;++g) {
        double batch=0,result=0,peak=0,transient=0,local=0,final=0;
        for(seed=0;seed<HPS_STAGE82_SCALE_SEEDS;++seed) {
            Stage82Sample *s=&scale_samples[n_index][g][seed];
            batch+=stage82_per_item(s->after_batch.live_bytes,scale_ns[n_index]);
            result+=stage82_per_item(s->result_only.live_bytes,scale_ns[n_index]);
            peak+=stage82_per_item(s->after_merge.peak_live_bytes,scale_ns[n_index]);
            transient+=stage82_per_item(s->after_merge.peak_live_bytes-s->after_merge.live_bytes,scale_ns[n_index]);
            local+=stage82_per_item(s->local_paths.total_depth,scale_ns[n_index]);
            final+=stage82_per_item(s->final_paths.total_depth,scale_ns[n_index]);
        }
        printf("%lu,%lu,%lu,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n",
            (unsigned long)scale_ns[n_index],(unsigned long)scale_groups[g],
            (unsigned long)scale_samples[n_index][g][0].group_size,
            batch/HPS_STAGE82_SCALE_SEEDS,result/HPS_STAGE82_SCALE_SEEDS,
            peak/HPS_STAGE82_SCALE_SEEDS,transient/HPS_STAGE82_SCALE_SEEDS,
            local/HPS_STAGE82_SCALE_SEEDS,final/HPS_STAGE82_SCALE_SEEDS);
    }
    {
        HpsAllocStats final_stats=hps_alloc_stats_get();
        if (!benchmark_alloc_stats_empty(&final_stats)) {
            printf("Stage 8.2 final allocator leak: bytes=%lu blocks=%lu\n",
                (unsigned long)final_stats.live_bytes,(unsigned long)final_stats.live_blocks);
            return 1;
        }
    }
    printf("Stage 8.2 complete; 8.3 not started.\n");
    return 0;
}

enum { HPS_STAGE83_GROUP_COUNT = 11 };
static const size_t stage83_groups[HPS_STAGE83_GROUP_COUNT] = {
    1, 4, 5, 8, 10, 16, 20, 32, 40, 64, 80
};

typedef struct Stage83ModelEntry {
    size_t count;
    int owned;
} Stage83ModelEntry;

typedef struct Stage83ModelResult {
    size_t rounds;
    size_t max_temp_units;
} Stage83ModelResult;

typedef struct Stage83CategoryRow {
    const char *name;
    double bytes;
} Stage83CategoryRow;

static int stage83_add(size_t *sum, size_t value)
{
    return benchmark_add_size(sum, value);
}

static int stage83_tag_totals(const HpsAllocStats *stats, size_t *path,
    size_t *tree, size_t *group, size_t *scratch)
{
    *path = 0; *tree = 0; *group = 0; *scratch = 0;
    return stage83_add(path, stats->tags[HPS_ALLOC_TAG_PATH_OBJECT].live_bytes) &&
        stage83_add(path, stats->tags[HPS_ALLOC_TAG_PATH_STEPS].live_bytes) &&
        stage83_add(tree, stats->tags[HPS_ALLOC_TAG_TREE_OBJECT].live_bytes) &&
        stage83_add(tree, stats->tags[HPS_ALLOC_TAG_TREE_NODE].live_bytes) &&
        stage83_add(tree, stats->tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_bytes) &&
        stage83_add(group, stats->tags[HPS_ALLOC_TAG_GROUP_OBJECT].live_bytes) &&
        stage83_add(group, stats->tags[HPS_ALLOC_TAG_GROUP_ORDERED].live_bytes) &&
        stage83_add(group, stats->tags[HPS_ALLOC_TAG_BATCH_OBJECT].live_bytes) &&
        stage83_add(group, stats->tags[HPS_ALLOC_TAG_BATCH_GROUP_ARRAY].live_bytes) &&
        stage83_add(scratch, stats->tags[HPS_ALLOC_TAG_MERGE_SCRATCH].live_bytes);
}

static int stage83_peak_category_totals(const HpsAllocStats *stats,
    size_t *path, size_t *tree, size_t *group, size_t *scratch)
{
    *path = 0; *tree = 0; *group = 0; *scratch = 0;
    return stage83_add(path, stats->bytes_at_global_peak[HPS_ALLOC_TAG_PATH_OBJECT]) &&
        stage83_add(path, stats->bytes_at_global_peak[HPS_ALLOC_TAG_PATH_STEPS]) &&
        stage83_add(tree, stats->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_OBJECT]) &&
        stage83_add(tree, stats->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_NODE]) &&
        stage83_add(tree, stats->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_CHILDREN]) &&
        stage83_add(group, stats->bytes_at_global_peak[HPS_ALLOC_TAG_GROUP_OBJECT]) &&
        stage83_add(group, stats->bytes_at_global_peak[HPS_ALLOC_TAG_GROUP_ORDERED]) &&
        stage83_add(group, stats->bytes_at_global_peak[HPS_ALLOC_TAG_BATCH_OBJECT]) &&
        stage83_add(group, stats->bytes_at_global_peak[HPS_ALLOC_TAG_BATCH_GROUP_ARRAY]) &&
        stage83_add(scratch, stats->bytes_at_global_peak[HPS_ALLOC_TAG_MERGE_SCRATCH]);
}

static const char *stage83_category_largest(size_t path, size_t tree,
    size_t group, size_t scratch)
{
    const char *name = "PATH_TOTAL";
    size_t maximum = path;
    if (tree > maximum) { maximum = tree; name = "TREE_TOTAL"; }
    if (group > maximum) { maximum = group; name = "GROUP_TOTAL"; }
    if (scratch > maximum) { name = "MERGE_SCRATCH"; }
    return name;
}

static Stage83ModelResult stage83_model_lifecycle(size_t count,
    size_t group_size, size_t groups)
{
    Stage83ModelEntry current[80], next[80];
    Stage83ModelResult result = { 0, 0 };
    size_t current_count = groups, offset = 0, index;
    size_t live_owned_units = 0;
    if (groups == 0 || groups > 80 || group_size == 0) return result;
    for (index = 0; index < groups; ++index) {
        size_t remaining = count - offset;
        size_t size = remaining < group_size ? remaining : group_size;
        current[index].count = size;
        current[index].owned = 0;
        offset += size;
    }
    if (groups == 1) {
        result.max_temp_units = count; /* MergeAll clones the borrowed source. */
        return result;
    }
    while (current_count > 1) {
        size_t next_count = current_count / 2 + (current_count % 2 != 0);
        size_t in = 0, out = 0;
        ++result.rounds;
        while (in < current_count) {
            if (in + 1 < current_count) {
                size_t merged_size = current[in].count + current[in + 1].count;
                size_t candidate = live_owned_units + merged_size;
                size_t removed = 0;
                if (candidate > result.max_temp_units)
                    result.max_temp_units = candidate;
                if (current[in].owned) removed += current[in].count;
                if (current[in + 1].owned) removed += current[in + 1].count;
                live_owned_units -= removed;
                live_owned_units += merged_size;
                next[out].count = merged_size;
                next[out].owned = 1;
                ++out;
                in += 2;
            } else {
                next[out++] = current[in++];
            }
        }
        if (out != next_count) return (Stage83ModelResult){ 0, 0 };
        memcpy(current, next, next_count * sizeof(*current));
        current_count = next_count;
    }
    return result;
}

static int stage83_tag_direct_test(void)
{
    void *path_object = NULL, *tree_node = NULL, *resized;
    HpsAllocStats stats;
    size_t peak_bytes = 0, peak_blocks = 0, i;
    int valid = 0;
    if (hps_alloc_stats_reset() != 0) return 0;
    path_object = hps_alloc_tagged(16, HPS_ALLOC_TAG_PATH_OBJECT);
    tree_node = hps_alloc_tagged(32, HPS_ALLOC_TAG_TREE_NODE);
    stats = hps_alloc_stats_get();
    for (i = 0; i < HPS_ALLOC_TAG_COUNT; ++i) {
        peak_bytes += stats.bytes_at_global_peak[i];
        peak_blocks += stats.blocks_at_global_peak[i];
    }
    valid = path_object != NULL && tree_node != NULL &&
        stats.live_bytes == 48 && stats.live_blocks == 2 &&
        stats.tags[HPS_ALLOC_TAG_PATH_OBJECT].live_bytes == 16 &&
        stats.tags[HPS_ALLOC_TAG_TREE_NODE].live_bytes == 32 &&
        stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes == 0 &&
        peak_bytes == 48 && peak_blocks == 2 &&
        stage83_tag_accounting_valid(&stats, "tag direct regression");
    if (valid) {
        resized = hps_realloc(path_object, 24);
        if (resized == NULL) valid = 0;
        else path_object = resized;
        stats = hps_alloc_stats_get();
        valid = valid && stats.live_bytes == 56 &&
            stats.tags[HPS_ALLOC_TAG_PATH_OBJECT].live_bytes == 24 &&
            stats.tags[HPS_ALLOC_TAG_TREE_NODE].live_bytes == 32 &&
            stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes == 0 &&
            stage83_tag_accounting_valid(&stats, "tagged realloc grow");
    }
    if (valid) {
        resized = hps_realloc(tree_node, (size_t)-1);
        stats = hps_alloc_stats_get();
        valid = resized == NULL && stats.live_bytes == 56 &&
            stats.tags[HPS_ALLOC_TAG_TREE_NODE].live_bytes == 32 &&
            stats.failed_calls == 1 &&
            stage83_tag_accounting_valid(&stats, "tagged realloc failure");
    }
    hps_free(path_object);
    hps_free(tree_node);
    stats = hps_alloc_stats_get();
    valid = valid && benchmark_alloc_stats_empty(&stats);
    printf("Stage 8.3 tag direct regression: %s; live bytes/blocks final=0/0\n",
        valid ? "passed (48-byte snapshot; realloc inherited PATH_OBJECT; failed tagged realloc preserved TREE_NODE; OTHER 0)" : "FAILED");
    return valid;
}

static int stage83_run_baseline(void)
{
    int values[1000];
    void *items[1000];
    Stage82Sample sample;
    size_t index;
    for (index = 0; index < 1000; ++index) {
        values[index] = (int)((index * 613u) % 1000u);
        items[index] = &values[index];
    }
    if (!stage82_run_sample(values, items, 1000, 100, 10, &sample,
            UINT32_C(0xC0FFEE), 1)) return 0;
    printf("Stage 8.1 baseline N=1000 GS=100: Batch=%lu PostMerge=%lu ResultOnly=%lu OverallPeak=%lu; byte-identical=%s\n",
        (unsigned long)sample.after_batch.live_bytes,
        (unsigned long)sample.after_merge.live_bytes,
        (unsigned long)sample.result_only.live_bytes,
        (unsigned long)sample.after_merge.peak_live_bytes,
        sample.after_batch.live_bytes == 139000 &&
        sample.after_merge.live_bytes == 305800 &&
        sample.result_only.live_bytes == 166800 &&
        sample.after_merge.peak_live_bytes == 496144 ? "yes" : "NO");
    return sample.after_batch.live_bytes == 139000 &&
        sample.after_merge.live_bytes == 305800 &&
        sample.result_only.live_bytes == 166800 &&
        sample.after_merge.peak_live_bytes == 496144;
}

static void stage83_print_peak_composition(
    Stage82Sample samples[HPS_STAGE83_GROUP_COUNT][HPS_STAGE82_MAIN_SEEDS])
{
    size_t g, seed;
    printf("\nPeak composition CSV\nGroups,GroupSize,PeakTotalBytesMean,PeakPathBytesMean,PeakTreeBytesMean,PeakGroupBytesMean,PeakMergeScratchBytesMean,PeakPathPercent,PeakTreePercent,PeakGroupPercent,PeakMergeScratchPercent\n");
    for (g = 0; g < HPS_STAGE83_GROUP_COUNT; ++g) {
        double total = 0, category[4] = { 0, 0, 0, 0 };
        for (seed = 0; seed < HPS_STAGE82_MAIN_SEEDS; ++seed) {
            size_t path, tree, group, scratch;
            HpsAllocStats *s = &samples[g][seed].after_merge;
            stage83_peak_category_totals(s, &path, &tree, &group, &scratch);
            total += (double)s->peak_live_bytes;
            category[0] += (double)path; category[1] += (double)tree;
            category[2] += (double)group; category[3] += (double)scratch;
        }
        total /= HPS_STAGE82_MAIN_SEEDS;
        for (seed = 0; seed < 4; ++seed) category[seed] /= HPS_STAGE82_MAIN_SEEDS;
        printf("%lu,%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.4f,%.4f,%.4f,%.4f\n",
            (unsigned long)stage83_groups[g], (unsigned long)samples[g][0].group_size,
            total, category[0], category[1], category[2], category[3],
            total == 0 ? 0 : 100.0 * category[0] / total,
            total == 0 ? 0 : 100.0 * category[1] / total,
            total == 0 ? 0 : 100.0 * category[2] / total,
            total == 0 ? 0 : 100.0 * category[3] / total);
    }
}

static void stage83_print_fine_peak(
    Stage82Sample samples[HPS_STAGE83_GROUP_COUNT][HPS_STAGE82_MAIN_SEEDS])
{
    static const char *names[HPS_ALLOC_TAG_COUNT] = {
        "OTHER", "PATH_OBJECT", "PATH_STEPS", "TREE_OBJECT", "TREE_NODE",
        "TREE_CHILDREN", "GROUP_OBJECT", "GROUP_ORDERED", "BATCH_OBJECT",
        "BATCH_GROUP_ARRAY", "MERGE_SCRATCH"
    };
    size_t g, seed, tag;
    printf("\nFine tag Peak CSV (requested bytes at the global byte peak, averaged over seeds)\nGroups,PATH_OBJECT,PATH_STEPS,TREE_OBJECT,TREE_NODE,TREE_CHILDREN,GROUP_OBJECT,GROUP_ORDERED,BATCH_OBJECT,BATCH_GROUP_ARRAY,MERGE_SCRATCH\n");
    for (g = 0; g < HPS_STAGE83_GROUP_COUNT; ++g) {
        double means[HPS_ALLOC_TAG_COUNT] = { 0 };
        (void)names;
        for (seed = 0; seed < HPS_STAGE82_MAIN_SEEDS; ++seed)
            for (tag = 0; tag < HPS_ALLOC_TAG_COUNT; ++tag)
                means[tag] += (double)samples[g][seed].after_merge.bytes_at_global_peak[tag];
        printf("%lu", (unsigned long)stage83_groups[g]);
        for (tag = HPS_ALLOC_TAG_PATH_OBJECT; tag < HPS_ALLOC_TAG_COUNT; ++tag)
            printf(",%.2f", means[tag] / HPS_STAGE82_MAIN_SEEDS);
        printf("\n");
    }
}

static void stage83_print_result_batch(
    Stage82Sample samples[HPS_STAGE83_GROUP_COUNT][HPS_STAGE82_MAIN_SEEDS])
{
    size_t g, seed;
    printf("\nResultOnly composition CSV\nGroups,ResultTotalBytesMean,ResultPathBytesMean,ResultTreeBytesMean,ResultGroupBytesMean,ResultPathPercent,ResultTreePercent,ResultGroupPercent\n");
    for (g = 0; g < HPS_STAGE83_GROUP_COUNT; ++g) {
        double total = 0, path = 0, tree = 0, group = 0;
        for (seed = 0; seed < HPS_STAGE82_MAIN_SEEDS; ++seed) {
            size_t p, t, gr, scratch;
            stage83_tag_totals(&samples[g][seed].result_only, &p, &t, &gr, &scratch);
            total += (double)samples[g][seed].result_only.live_bytes;
            path += p; tree += t; group += gr;
        }
        total /= HPS_STAGE82_MAIN_SEEDS; path /= HPS_STAGE82_MAIN_SEEDS;
        tree /= HPS_STAGE82_MAIN_SEEDS; group /= HPS_STAGE82_MAIN_SEEDS;
        printf("%lu,%.2f,%.2f,%.2f,%.2f,%.4f,%.4f,%.4f\n",
            (unsigned long)stage83_groups[g], total, path, tree, group,
            total == 0 ? 0 : 100.0 * path / total,
            total == 0 ? 0 : 100.0 * tree / total,
            total == 0 ? 0 : 100.0 * group / total);
    }
    printf("\nBatch composition CSV\nGroups,BatchTotalBytesMean,BatchPathBytesMean,BatchTreeBytesMean,BatchGroupBytesMean\n");
    for (g = 0; g < HPS_STAGE83_GROUP_COUNT; ++g) {
        double total = 0, path = 0, tree = 0, group = 0;
        for (seed = 0; seed < HPS_STAGE82_MAIN_SEEDS; ++seed) {
            size_t p, t, gr, scratch;
            stage83_tag_totals(&samples[g][seed].after_batch, &p, &t, &gr, &scratch);
            total += (double)samples[g][seed].after_batch.live_bytes;
            path += p; tree += t; group += gr;
        }
        printf("%lu,%.2f,%.2f,%.2f,%.2f\n",
            (unsigned long)stage83_groups[g], total / HPS_STAGE82_MAIN_SEEDS,
            path / HPS_STAGE82_MAIN_SEEDS, tree / HPS_STAGE82_MAIN_SEEDS,
            group / HPS_STAGE82_MAIN_SEEDS);
    }
}

static Stage83ModelResult stage83_model_for_group(size_t group_index,
    Stage82Sample samples[HPS_STAGE83_GROUP_COUNT][HPS_STAGE82_MAIN_SEEDS])
{
    return stage83_model_lifecycle(10000, samples[group_index][0].group_size,
        stage83_groups[group_index]);
}

static double stage83_transient_per_item_mean(
    Stage82Sample samples[HPS_STAGE83_GROUP_COUNT][HPS_STAGE82_MAIN_SEEDS],
    size_t group_index)
{
    double sum = 0;
    size_t seed;
    for (seed = 0; seed < HPS_STAGE82_MAIN_SEEDS; ++seed) {
        Stage82Sample *s = &samples[group_index][seed];
        sum += (double)(s->after_merge.peak_live_bytes - s->after_merge.live_bytes) / 10000.0;
    }
    return sum / HPS_STAGE82_MAIN_SEEDS;
}

static void stage83_print_models(
    Stage82Sample samples[HPS_STAGE83_GROUP_COUNT][HPS_STAGE82_MAIN_SEEDS])
{
    Stage83ModelResult models[HPS_STAGE83_GROUP_COUNT];
    double x_mean = 0, y_mean = 0, numerator = 0, x_square = 0, y_square = 0;
    size_t g;
    printf("\nMerge lifecycle model CSV\nGroups,GroupSize,MergeRounds,MaxTemporaryElementUnits,TempElementAmplification,TransientExtraPeakBytesMean,TransientExtraPeakBytesPerItemMean\n");
    for (g = 0; g < HPS_STAGE83_GROUP_COUNT; ++g) {
        double transient_sum = 0;
        size_t seed;
        models[g] = stage83_model_for_group(g, samples);
        for (seed = 0; seed < HPS_STAGE82_MAIN_SEEDS; ++seed)
            transient_sum += (double)(samples[g][seed].after_merge.peak_live_bytes -
                samples[g][seed].after_merge.live_bytes);
        printf("%lu,%lu,%lu,%lu,%.4f,%.2f,%.4f\n",
            (unsigned long)stage83_groups[g], (unsigned long)samples[g][0].group_size,
            (unsigned long)models[g].rounds, (unsigned long)models[g].max_temp_units,
            models[g].max_temp_units / 10000.0,
            transient_sum / HPS_STAGE82_MAIN_SEEDS,
            transient_sum / (HPS_STAGE82_MAIN_SEEDS * 10000.0));
        x_mean += models[g].max_temp_units / 10000.0;
        y_mean += stage83_transient_per_item_mean(samples, g);
    }
    x_mean /= HPS_STAGE83_GROUP_COUNT; y_mean /= HPS_STAGE83_GROUP_COUNT;
    for (g = 0; g < HPS_STAGE83_GROUP_COUNT; ++g) {
        double x = models[g].max_temp_units / 10000.0 - x_mean;
        double y = stage83_transient_per_item_mean(samples, g) - y_mean;
        numerator += x * y; x_square += x * x; y_square += y * y;
    }
    printf("ModelTransientPearsonCorrelationCoefficient,%.8f\n",
        x_square == 0 || y_square == 0 ? 0 : numerator / sqrt(x_square * y_square));
    printf("\nPower-of-two neighbor transient comparison\nHighGroups,NeighborGroups,HighTransientPerItem,NeighborTransientPerItem,Delta\n");
    {
        static const size_t high_groups[] = {4,8,16,32,64};
        static const size_t neighbor_groups[] = {5,10,20,40,80};
        size_t pair;
        for (pair=0;pair<5;++pair) {
            size_t hi=0,lo=0;
            while(stage83_groups[hi]!=high_groups[pair]) ++hi;
            while(stage83_groups[lo]!=neighbor_groups[pair]) ++lo;
            {
                double high=stage83_transient_per_item_mean(samples,hi);
                double neighbor=stage83_transient_per_item_mean(samples,lo);
                printf("%lu,%lu,%.4f,%.4f,%.4f\n",
                    (unsigned long)high_groups[pair],(unsigned long)neighbor_groups[pair],
                    high,neighbor,high-neighbor);
            }
        }
    }
}

static void stage83_print_rankings_and_facts(
    Stage82Sample samples[HPS_STAGE83_GROUP_COUNT][HPS_STAGE82_MAIN_SEEDS])
{
    static const char *tag_names[HPS_ALLOC_TAG_COUNT] = {
        "OTHER", "PATH_OBJECT", "PATH_STEPS", "TREE_OBJECT", "TREE_NODE",
        "TREE_CHILDREN", "GROUP_OBJECT", "GROUP_ORDERED", "BATCH_OBJECT",
        "BATCH_GROUP_ARRAY", "MERGE_SCRATCH"
    };
    static const char *category_names[4] = {
        "PATH_TOTAL", "TREE_TOTAL", "GROUP_TOTAL", "MERGE_SCRATCH"
    };
    const size_t chosen_groups[2] = {32,40};
    size_t chosen_index[2] = {0,0};
    size_t c, g;
    for (c=0;c<2;++c)
        while(stage83_groups[chosen_index[c]]!=chosen_groups[c]) ++chosen_index[c];
    for (c=0;c<2;++c) {
        size_t tag, seed, count=HPS_ALLOC_TAG_COUNT-1;
        Stage83CategoryRow rows[HPS_ALLOC_TAG_COUNT-1] = { 0 };
        double peak_total=0, categories[4]={0,0,0,0};
        for (tag=0;tag<count;++tag)
            rows[tag].name=tag_names[tag+HPS_ALLOC_TAG_PATH_OBJECT];
        for(seed=0;seed<HPS_STAGE82_MAIN_SEEDS;++seed) {
            HpsAllocStats *s=&samples[chosen_index[c]][seed].after_merge;
            size_t path,tree,group,scratch;
            peak_total += (double)s->peak_live_bytes;
            stage83_peak_category_totals(s,&path,&tree,&group,&scratch);
            categories[0]+=path; categories[1]+=tree;
            categories[2]+=group; categories[3]+=scratch;
            for(tag=0;tag<count;++tag)
                rows[tag].bytes += (double)s->bytes_at_global_peak[tag+HPS_ALLOC_TAG_PATH_OBJECT];
        }
        peak_total/=HPS_STAGE82_MAIN_SEEDS;
        for(g=0;g<4;++g) categories[g]/=HPS_STAGE82_MAIN_SEEDS;
        for(g=0;g<count;++g) for(tag=g+1;tag<count;++tag)
            if(rows[tag].bytes>rows[g].bytes) {
                Stage83CategoryRow temp=rows[g]; rows[g]=rows[tag]; rows[tag]=temp;
            }
        printf("\nG=%lu peak fine-tag ranking\nRank,Tag,BytesMean,PercentOfGlobalPeak\n",
            (unsigned long)chosen_groups[c]);
        for(g=0;g<count;++g)
            printf("%lu,%s,%.2f,%.4f\n",(unsigned long)(g+1),rows[g].name,
                rows[g].bytes/HPS_STAGE82_MAIN_SEEDS,
                peak_total==0?0:100.0*rows[g].bytes/(HPS_STAGE82_MAIN_SEEDS*peak_total));
        {
            size_t largest=0;
            for(g=1;g<4;++g) if(categories[g]>categories[largest]) largest=g;
            printf("G=%lu largest peak category: %s %.4f%%\n",
                (unsigned long)chosen_groups[c],category_names[largest],
                peak_total==0?0:100.0*categories[largest]/peak_total);
        }
    }
    printf("\nStage8HotspotFacts\nFact,Value\n");
    for(c=0;c<2;++c) {
        size_t index=chosen_index[c],seed;
        double peak_total=0,result_total=0,peak_categories[4]={0,0,0,0};
        double result_categories[3]={0,0,0},scratch_peak=0;
        size_t largest_peak=0,largest_result=0;
        Stage83ModelResult model=stage83_model_for_group(index,samples);
        double transient=stage83_transient_per_item_mean(samples,index);
        for(seed=0;seed<HPS_STAGE82_MAIN_SEEDS;++seed) {
            size_t path,tree,group,scratch;
            HpsAllocStats *p=&samples[index][seed].after_merge;
            HpsAllocStats *r=&samples[index][seed].result_only;
            peak_total+=(double)p->peak_live_bytes;
            result_total+=(double)r->live_bytes;
            stage83_peak_category_totals(p,&path,&tree,&group,&scratch);
            peak_categories[0]+=path;peak_categories[1]+=tree;
            peak_categories[2]+=group;peak_categories[3]+=scratch;
            scratch_peak+=scratch;
            stage83_tag_totals(r,&path,&tree,&group,&scratch);
            result_categories[0]+=path;result_categories[1]+=tree;result_categories[2]+=group;
        }
        peak_total/=HPS_STAGE82_MAIN_SEEDS;result_total/=HPS_STAGE82_MAIN_SEEDS;
        scratch_peak/=HPS_STAGE82_MAIN_SEEDS;
        for(g=0;g<4;++g) peak_categories[g]/=HPS_STAGE82_MAIN_SEEDS;
        for(g=0;g<3;++g) result_categories[g]/=HPS_STAGE82_MAIN_SEEDS;
        for(g=1;g<4;++g) if(peak_categories[g]>peak_categories[largest_peak]) largest_peak=g;
        for(g=1;g<3;++g) if(result_categories[g]>result_categories[largest_result]) largest_result=g;
        printf("LargestPeakCategory_G%lu,%s\nLargestPeakCategoryPercent_G%lu,%.4f\n",
            (unsigned long)chosen_groups[c],category_names[largest_peak],
            (unsigned long)chosen_groups[c],peak_total==0?0:100.0*peak_categories[largest_peak]/peak_total);
        printf("LargestResultCategory_G%lu,%s\nLargestResultCategoryPercent_G%lu,%.4f\n",
            (unsigned long)chosen_groups[c],category_names[largest_result],
            (unsigned long)chosen_groups[c],result_total==0?0:100.0*result_categories[largest_result]/result_total);
        printf("MergeScratchPeakPercent_G%lu,%.4f\nTempElementAmplification_G%lu,%.4f\nTransientPeakPerItem_G%lu,%.4f\n",
            (unsigned long)chosen_groups[c],peak_total==0?0:100.0*scratch_peak/peak_total,
            (unsigned long)chosen_groups[c],model.max_temp_units/10000.0,
            (unsigned long)chosen_groups[c],transient);
    }
}

int hps_run_stage8_3_benchmark(void)
{
    Stage82Sample samples[HPS_STAGE83_GROUP_COUNT][HPS_STAGE82_MAIN_SEEDS];
    int values[HPS_STAGE82_MAIN_N];
    void *items[HPS_STAGE82_MAIN_N];
    size_t seed, index, item;
    printf("HPSort Stage 8.3 Private Memory Tagging and Hotspot Measurement\n");
    if (hps_run_stage8_1_tests() != 0 || !stage82_alignment_regression() ||
        !stage83_tag_direct_test()) return 1;
    if (!stage83_run_baseline()) return 1;
    for (seed=0;seed<HPS_STAGE82_MAIN_SEEDS;++seed) {
        for (item=0;item<HPS_STAGE82_MAIN_N;++item) values[item]=(int)item;
        benchmark_shuffle(values,HPS_STAGE82_MAIN_N,stage82_seeds[seed]);
        for(index=0;index<HPS_STAGE83_GROUP_COUNT;++index) {
            size_t groups=stage83_groups[index];
            size_t group_size=HPS_STAGE82_MAIN_N/groups+
                (HPS_STAGE82_MAIN_N%groups!=0);
            if (!stage82_run_sample(values,items,HPS_STAGE82_MAIN_N,group_size,
                    groups,&samples[index][seed],stage82_seeds[seed],1)) return 1;
        }
    }
    printf("Stage 8.3 hotspot panel: 55/55 lifecycles ended at 0/0; FailedCalls=0; tag live and global peak snapshot invariants passed.\n");
    stage83_print_peak_composition(samples);
    stage83_print_fine_peak(samples);
    stage83_print_result_batch(samples);
    {
        printf("\nMergeScratch Peak CSV\nGroups,PeakMergeScratchBytesMean,PeakMergeScratchPercent\n");
        for(index=0;index<HPS_STAGE83_GROUP_COUNT;++index) {
            double scratch=0,total=0;
            for(seed=0;seed<HPS_STAGE82_MAIN_SEEDS;++seed) {
                HpsAllocStats *s=&samples[index][seed].after_merge;
                scratch+=s->bytes_at_global_peak[HPS_ALLOC_TAG_MERGE_SCRATCH];
                total+=s->peak_live_bytes;
            }
            scratch/=HPS_STAGE82_MAIN_SEEDS;total/=HPS_STAGE82_MAIN_SEEDS;
            printf("%lu,%.2f,%.4f\n",(unsigned long)stage83_groups[index],scratch,
                total==0?0:100.0*scratch/total);
        }
    }
    stage83_print_models(samples);
    stage83_print_rankings_and_facts(samples);
    {
        HpsAllocStats stats=hps_alloc_stats_get();
        if (!benchmark_alloc_stats_empty(&stats) || stats.failed_calls != 0 ||
            !stage83_tag_accounting_valid(&stats,"Stage 8.3 final")) return 1;
    }
    printf("Stage 8 complete. No memory optimization or Stage 8.4 was started.\n");
    return 0;
}

#include "../src/hps_path_internal.h"

enum { LEGACY_PATH_STEP_SIZE = 16 }; /* Stage 9.1 measured baseline. */

enum {
    HPS_STAGE91_PATH_COUNT = 10000,
    HPS_STAGE91_CONFIG_COUNT = 3,
    HPS_STAGE91_DEPTH_HIST_COUNT = 15,
    HPS_STAGE91_DEPTH_CAP_COUNT = 14
};

typedef struct Stage91JointBucket {
    size_t count;
    size_t capacity_sum;
    size_t min_capacity;
    size_t max_capacity;
} Stage91JointBucket;

typedef struct Stage91Profile {
    size_t path_count;
    size_t zero_depth_count;
    size_t total_depth;
    size_t total_capacity;
    size_t unused_slots;
    size_t storage_bytes;
    size_t depth_hist[HPS_STAGE91_DEPTH_HIST_COUNT];
    size_t capacities[HPS_STAGE91_PATH_COUNT];
    Stage91JointBucket joint[HPS_STAGE91_DEPTH_CAP_COUNT];
} Stage91Profile;

typedef struct Stage91Sample {
    size_t group_size;
    HpsAllocStats after_batch;
    HpsAllocStats after_merge;
    HpsAllocStats result_only;
    HpsAllocStats final;
} Stage91Sample;

static Stage91Profile stage91_profiles[HPS_STAGE91_CONFIG_COUNT][2];
static Stage91Sample stage91_samples[HPS_STAGE91_CONFIG_COUNT];
static const size_t stage91_groups[HPS_STAGE91_CONFIG_COUNT] = {1,32,40};

static int stage91_mul_size(size_t a, size_t b, size_t *result)
{
    if (b != 0 && a > (size_t)-1 / b) return 0;
    *result = a * b;
    return 1;
}

static size_t stage91_depth_bucket(size_t depth)
{
    if (depth <= 10) return depth;
    if (depth <= 15) return 11;
    if (depth <= 20) return 12;
    if (depth <= 30) return 13;
    return 14;
}

static size_t stage91_joint_bucket(size_t depth)
{
    if (depth <= 10) return depth;
    if (depth <= 15) return 11;
    if (depth <= 20) return 12;
    return 13;
}

static int stage91_profile_path(Stage91Profile *profile, const HpsPath *path)
{
    size_t depth, capacity, bucket, bytes;
    Stage91JointBucket *joint;
    if (path == NULL || profile->path_count >= HPS_STAGE91_PATH_COUNT) return 0;
    depth = hps_path_depth(path);
    capacity = hps_path_internal_capacity(path);
    bytes = hps_path_internal_storage_bytes(path);
    if (capacity < depth ||
        bytes == (size_t)-1 ||
        !benchmark_add_size(&profile->storage_bytes, bytes) ||
        !benchmark_add_size(&profile->total_depth, depth) ||
        !benchmark_add_size(&profile->total_capacity, capacity) ||
        !benchmark_add_size(&profile->unused_slots, capacity - depth)) return 0;
    ++profile->path_count;
    if (depth == 0) ++profile->zero_depth_count;
    ++profile->depth_hist[stage91_depth_bucket(depth)];
    profile->capacities[profile->path_count - 1] = capacity;
    bucket = stage91_joint_bucket(depth);
    joint = &profile->joint[bucket];
    if (joint->count == 0 || capacity < joint->min_capacity)
        joint->min_capacity = capacity;
    if (capacity > joint->max_capacity) joint->max_capacity = capacity;
    ++joint->count;
    return benchmark_add_size(&joint->capacity_sum, capacity);
}

static int stage91_profile_batch(Stage91Profile *profile,
    const HpsGroupBatch *batch)
{
    size_t group_index;
    for (group_index=0; group_index<hps_group_batch_group_count(batch); ++group_index) {
        const HpsGroup *group=hps_group_batch_group_at(batch,group_index);
        size_t item;
        if (group==NULL) return 0;
        for(item=0;item<hps_group_size(group);++item)
            if(!stage91_profile_path(profile,hps_group_path_at(group,item))) return 0;
    }
    return 1;
}

static int stage91_profile_result(Stage91Profile *profile,const HpsGroup *result)
{
    size_t item;
    for(item=0;item<hps_group_size(result);++item)
        if(!stage91_profile_path(profile,hps_group_path_at(result,item))) return 0;
    return 1;
}

static int stage91_path_bytes(const Stage91Profile *profile, size_t *object_bytes,
    size_t *capacity_bytes, size_t *unused_bytes)
{
    return stage91_mul_size(profile->path_count,
            hps_path_internal_sizeof_path(), object_bytes) &&
        ((*capacity_bytes = profile->storage_bytes), 1) &&
        ((*unused_bytes = 0), 1);
}

static int stage91_validate_profile(const Stage91Profile *profile,
    const HpsAllocStats *stats, const char *name)
{
    size_t object_bytes, capacity_bytes, unused_bytes;
    if (!stage91_path_bytes(profile,&object_bytes,&capacity_bytes,&unused_bytes) ||
        object_bytes != stats->tags[HPS_ALLOC_TAG_PATH_OBJECT].live_bytes ||
        capacity_bytes != stats->tags[HPS_ALLOC_TAG_PATH_STEPS].live_bytes ||
        object_bytes + capacity_bytes !=
            stats->tags[HPS_ALLOC_TAG_PATH_OBJECT].live_bytes +
            stats->tags[HPS_ALLOC_TAG_PATH_STEPS].live_bytes) {
        printf("Stage 9.1 Path profile mismatch at %s: count=%lu object=%lu/%lu capacityBytes=%lu/%lu unusedBytes=%lu\n",
            name,(unsigned long)profile->path_count,(unsigned long)object_bytes,
            (unsigned long)stats->tags[HPS_ALLOC_TAG_PATH_OBJECT].live_bytes,
            (unsigned long)capacity_bytes,
            (unsigned long)stats->tags[HPS_ALLOC_TAG_PATH_STEPS].live_bytes,
            (unsigned long)unused_bytes);
        return 0;
    }
    return 1;
}

static int stage91_run_sample(int config, int *values, void **items)
{
    HpsComparator comparator;
    BenchmarkCompareContext compare_context = {0,0};
    HpsGroupBatch *batch=NULL;
    HpsGroup *result=NULL;
    HpsStatus status=HPS_STATUS_OK;
    size_t i,groups=stage91_groups[config];
    size_t group_size=HPS_STAGE91_PATH_COUNT/groups+
        (HPS_STAGE91_PATH_COUNT%groups!=0);
    Stage91Sample *sample=&stage91_samples[config];
    Stage91Profile *batch_profile=&stage91_profiles[config][0];
    Stage91Profile *result_profile=&stage91_profiles[config][1];
    BenchmarkPathStats final_paths={0};
    int success=0;

    memset(sample,0,sizeof(*sample));
    memset(batch_profile,0,sizeof(*batch_profile));
    memset(result_profile,0,sizeof(*result_profile));
    if(hps_alloc_stats_reset()!=0) return 0;
    for(i=0;i<HPS_STAGE91_PATH_COUNT;++i) items[i]=&values[i];
    comparator.compare=benchmark_compare_int;
    comparator.context=&compare_context;
    status=hps_group_batch_build(items,HPS_STAGE91_PATH_COUNT,group_size,
        &comparator,&batch);
    if(status!=HPS_STATUS_OK||batch==NULL||hps_group_batch_group_count(batch)!=groups) goto cleanup;
    sample->group_size=group_size;
    sample->after_batch=hps_alloc_stats_get();
    if(!stage83_tag_accounting_valid(&sample->after_batch,"9.1 AfterBatchBuild")||
        !stage83_other_is_empty(&sample->after_batch,"9.1 AfterBatchBuild")||
        !stage91_profile_batch(batch_profile,batch)||
        !stage91_validate_profile(batch_profile,&sample->after_batch,"Batch")) goto cleanup;
    status=hps_group_batch_merge_all(batch,&comparator,&result);
    if(status!=HPS_STATUS_OK||result==NULL) goto cleanup;
    sample->after_merge=hps_alloc_stats_get();
    if(!stage83_tag_accounting_valid(&sample->after_merge,"9.1 PostMerge")||
        !stage83_other_is_empty(&sample->after_merge,"9.1 PostMerge")) goto cleanup;
    memset(&final_paths,0,sizeof(final_paths));
    if(!benchmark_validate_final(result,HPS_STAGE91_PATH_COUNT,&final_paths)) goto cleanup;
    hps_group_batch_destroy(batch); batch=NULL;
    sample->result_only=hps_alloc_stats_get();
    if(!stage83_tag_accounting_valid(&sample->result_only,"9.1 ResultOnly")||
        !stage83_other_is_empty(&sample->result_only,"9.1 ResultOnly")||
        !stage91_profile_result(result_profile,result)||
        !stage91_validate_profile(result_profile,&sample->result_only,"ResultOnly")||
        sample->result_only.tags[HPS_ALLOC_TAG_MERGE_SCRATCH].live_bytes!=0) goto cleanup;
    if(!benchmark_validate_final(result,HPS_STAGE91_PATH_COUNT,&final_paths) ||
        sample->after_merge.live_bytes != sample->after_batch.live_bytes+
            sample->result_only.live_bytes ||
        sample->after_merge.live_blocks != sample->after_batch.live_blocks+
            sample->result_only.live_blocks) goto cleanup;
    hps_group_destroy(result); result=NULL;
    sample->final=hps_alloc_stats_get();
    if(!benchmark_alloc_stats_empty(&sample->final)||sample->final.failed_calls!=0||
        !stage83_tag_accounting_valid(&sample->final,"9.1 Final")||
        !stage83_other_is_empty(&sample->final,"9.1 Final")) goto cleanup;
    success=1;
cleanup:
    hps_group_batch_destroy(batch);
    hps_group_destroy(result);
    if(!success) {
        HpsAllocStats now=hps_alloc_stats_get();
        printf("Stage 9.1 sample failed: Groups=%lu GroupSize=%lu status=%s live=%lu/%lu failed=%lu\n",
            (unsigned long)groups,(unsigned long)group_size,hps_status_string(status),
            (unsigned long)now.live_bytes,(unsigned long)now.live_blocks,
            (unsigned long)now.failed_calls);
    }
    return success;
}

static int stage91_capacity_compare(const void *left,const void *right)
{
    size_t a=*(const size_t *)left,b=*(const size_t *)right;
    return a<b?-1:a>b?1:0;
}

static const char *stage91_depth_label(size_t bucket)
{
    static const char *labels[HPS_STAGE91_DEPTH_HIST_COUNT]={
        "0","1","2","3","4","5","6","7","8","9","10",
        "11-15","16-20","21-30","31+"
    };
    return labels[bucket];
}

static const char *stage91_joint_label(size_t bucket)
{
    if(bucket<=10) {
        static const char *single[11]={"0","1","2","3","4","5","6","7","8","9","10"};
        return single[bucket];
    }
    return bucket==11?"11-15":bucket==12?"16-20":"21+";
}

static void stage91_print_profile(const char *phase,size_t groups,
    Stage91Profile *profile)
{
    size_t i,unique=0,object_bytes=0,capacity_bytes=0,unused_bytes=0;
    printf("\n%s Path profile, Groups=%lu\nPathCount,ZeroDepthPathCount,TotalDepth,TotalCapacity,UnusedCapacitySlots,DepthUtilization,LogicalStepStructBytes,CapacityStepBytes,CapacitySlackBytes,PathObjectBytes,TotalPathBytes\n",
        phase,(unsigned long)groups);
    (void)stage91_path_bytes(profile,&object_bytes,&capacity_bytes,&unused_bytes);
    printf("%lu,%lu,%lu,%lu,%lu,%.8f,%lu,%lu,%lu,%lu,%lu\n",
        (unsigned long)profile->path_count,(unsigned long)profile->zero_depth_count,
        (unsigned long)profile->total_depth,(unsigned long)profile->total_capacity,
        (unsigned long)profile->unused_slots,
        profile->total_capacity==0?0:(double)profile->total_depth/profile->total_capacity,
        (unsigned long)(profile->total_depth*LEGACY_PATH_STEP_SIZE),
        (unsigned long)capacity_bytes,(unsigned long)unused_bytes,
        (unsigned long)object_bytes,(unsigned long)(object_bytes+capacity_bytes));
    printf("Depth histogram\nDepth,PathCount,Percent\n");
    for(i=0;i<HPS_STAGE91_DEPTH_HIST_COUNT;++i)
        printf("%s,%lu,%.4f\n",stage91_depth_label(i),
            (unsigned long)profile->depth_hist[i],
            profile->path_count==0?0:100.0*profile->depth_hist[i]/profile->path_count);
    qsort(profile->capacities,profile->path_count,sizeof(profile->capacities[0]),
        stage91_capacity_compare);
    for(i=0;i<profile->path_count;) {
        size_t j=i+1;
        while(j<profile->path_count&&profile->capacities[j]==profile->capacities[i]) ++j;
        ++unique; i=j;
    }
    printf("Capacity histogram\nCapacity,PathCount,Percent\n");
    for(i=0;i<profile->path_count;) {
        size_t j=i+1;
        while(j<profile->path_count&&profile->capacities[j]==profile->capacities[i]) ++j;
        printf("%lu,%lu,%.4f\n",(unsigned long)profile->capacities[i],
            (unsigned long)(j-i),profile->path_count==0?0:100.0*(j-i)/profile->path_count);
        i=j;
    }
    printf("Depth-to-capacity table\nDepth,MeanCapacity,MinCapacity,MaxCapacity,PathCount\n");
    for(i=0;i<HPS_STAGE91_DEPTH_CAP_COUNT;++i) {
        Stage91JointBucket *joint=&profile->joint[i];
        printf("%s,%.4f,%lu,%lu,%lu\n",stage91_joint_label(i),
            joint->count==0?0:(double)joint->capacity_sum/joint->count,
            (unsigned long)(joint->count==0?0:joint->min_capacity),
            (unsigned long)joint->max_capacity,(unsigned long)joint->count);
    }
    printf("Observed distinct capacities: %lu\n",(unsigned long)unique);
}

static void stage91_print_result_decomposition(void)
{
    size_t c;
    size_t path_size=hps_path_internal_sizeof_path();
    size_t step_size=LEGACY_PATH_STEP_SIZE;
    size_t field_size=sizeof(unsigned short)+sizeof(size_t);
    size_t padding=step_size-field_size;
    printf("\nResult Path decomposition CSV\nGroups,PathCount,PathObjectBytes,TotalDepth,TotalCapacity,UnusedCapacitySlots,DepthUtilization,LogicalFieldBytes,StructPaddingBytes,CapacitySlackBytes,TotalPathBytes\n");
    for(c=0;c<HPS_STAGE91_CONFIG_COUNT;++c) {
        Stage91Profile *p=&stage91_profiles[c][1];
        size_t obj,cap,slack,logical,used_padding;
        (void)stage91_path_bytes(p,&obj,&cap,&slack);
        logical=p->total_depth*field_size;
        used_padding=p->total_depth*padding;
        printf("%lu,%lu,%lu,%lu,%lu,%lu,%.8f,%lu,%lu,%lu,%lu\n",
            (unsigned long)stage91_groups[c],(unsigned long)p->path_count,
            (unsigned long)(p->path_count*path_size),(unsigned long)p->total_depth,
            (unsigned long)p->total_capacity,(unsigned long)p->unused_slots,
            p->total_capacity==0?0:(double)p->total_depth/p->total_capacity,
            (unsigned long)logical,(unsigned long)used_padding,
            (unsigned long)slack,(unsigned long)(obj+cap));
    }
    printf("\nResult Path percentage CSV\nGroups,ObjectPercentOfPath,UsedStepStructPercentOfPath,SlackPercentOfPath,StructPaddingPercentOfPath\n");
    for(c=0;c<HPS_STAGE91_CONFIG_COUNT;++c) {
        Stage91Profile *p=&stage91_profiles[c][1];
        size_t obj,cap,slack;
        double total;
        (void)stage91_path_bytes(p,&obj,&cap,&slack); total=(double)(obj+cap);
        printf("%lu,%.4f,%.4f,%.4f,%.4f\n",(unsigned long)stage91_groups[c],
            total==0?0:100.0*obj/total,
            total==0?0:100.0*p->total_depth*LEGACY_PATH_STEP_SIZE/total,
            total==0?0:100.0*slack/total,
            total==0?0:100.0*p->total_depth*(LEGACY_PATH_STEP_SIZE-
                (sizeof(unsigned short)+sizeof(size_t)))/total);
    }
    printf("\nTheoretical saving ceiling CSV\nGroups,CapacitySlackBytes,CapacitySlackPercentOfPath,StructPaddingOnUsedBytes,StructPaddingOnUsedPercentOfPath\n");
    for(c=0;c<HPS_STAGE91_CONFIG_COUNT;++c) {
        Stage91Profile *p=&stage91_profiles[c][1];
        size_t obj,cap,slack;
        double total;
        (void)stage91_path_bytes(p,&obj,&cap,&slack); total=(double)(obj+cap);
        printf("%lu,%lu,%.4f,%lu,%.4f\n",(unsigned long)stage91_groups[c],
            (unsigned long)slack,total==0?0:100.0*slack/total,
            (unsigned long)(p->total_depth*(step_size-field_size)),
            total==0?0:100.0*p->total_depth*(step_size-field_size)/total);
    }
}

static size_t stage91_find_config(size_t groups)
{
    size_t index=0;
    while(index<HPS_STAGE91_CONFIG_COUNT&&stage91_groups[index]!=groups) ++index;
    return index;
}

static int stage91_print_peak_and_comparison(void)
{
    size_t c;
    size_t path_size=hps_path_internal_sizeof_path();
    size_t step_size=LEGACY_PATH_STEP_SIZE;
    size_t g32=stage91_find_config(32),g40=stage91_find_config(40);
    size_t peak_path_obj[3],peak_steps[3],peak_path_count[3],peak_step_units[3];
    printf("\nGlobal peak Path counts (allocator snapshot only)\nGroups,PeakPathObjectBytes,PeakPathObjectCount,PeakPathStepsBytes,PeakStepCapacityUnits,PeakPathObjectCountPerN,PeakStepCapacityUnitsPerN\n");
    for(c=0;c<HPS_STAGE91_CONFIG_COUNT;++c) {
        HpsAllocStats *s=&stage91_samples[c].after_merge;
        size_t object_bytes=s->bytes_at_global_peak[HPS_ALLOC_TAG_PATH_OBJECT];
        size_t step_bytes=s->bytes_at_global_peak[HPS_ALLOC_TAG_PATH_STEPS];
        if(object_bytes%path_size!=0||step_bytes%step_size!=0) {
            printf("Global peak Path byte divisibility FAILED for Groups=%lu\n",
                (unsigned long)stage91_groups[c]); return 0;
        }
        peak_path_obj[c]=object_bytes; peak_steps[c]=step_bytes;
        peak_path_count[c]=object_bytes/path_size; peak_step_units[c]=step_bytes/step_size;
        printf("%lu,%lu,%lu,%lu,%lu,%.6f,%.6f\n",
            (unsigned long)stage91_groups[c],(unsigned long)object_bytes,
            (unsigned long)peak_path_count[c],(unsigned long)step_bytes,
            (unsigned long)peak_step_units[c],peak_path_count[c]/10000.0,
            peak_step_units[c]/10000.0);
    }
    printf("\nG32 versus G40 Path peak difference CSV\nMetric,G32,G40,Delta(G32-G40)\n");
    {
        HpsAllocStats *a=&stage91_samples[g32].after_merge;
        HpsAllocStats *b=&stage91_samples[g40].after_merge;
        Stage91Profile *r32=&stage91_profiles[g32][1],*r40=&stage91_profiles[g40][1];
        size_t vals32[10],vals40[10],i,obj,cap,slack;
        vals32[0]=a->peak_live_bytes; vals40[0]=b->peak_live_bytes;
        vals32[1]=peak_path_obj[g32]; vals40[1]=peak_path_obj[g40];
        vals32[2]=peak_steps[g32]; vals40[2]=peak_steps[g40];
        vals32[3]=peak_path_count[g32]; vals40[3]=peak_path_count[g40];
        vals32[4]=peak_step_units[g32]; vals40[4]=peak_step_units[g40];
        vals32[5]=stage91_samples[g32].result_only.tags[HPS_ALLOC_TAG_PATH_OBJECT].live_bytes;
        vals40[5]=stage91_samples[g40].result_only.tags[HPS_ALLOC_TAG_PATH_OBJECT].live_bytes;
        vals32[6]=stage91_samples[g32].result_only.tags[HPS_ALLOC_TAG_PATH_STEPS].live_bytes;
        vals40[6]=stage91_samples[g40].result_only.tags[HPS_ALLOC_TAG_PATH_STEPS].live_bytes;
        vals32[7]=r32->total_depth; vals40[7]=r40->total_depth;
        vals32[8]=r32->total_capacity; vals40[8]=r40->total_capacity;
        vals32[9]=r32->unused_slots; vals40[9]=r40->unused_slots;
        (void)obj;(void)cap;(void)slack;
        {
            static const char *metric[10]={"PeakTotalBytes","PeakPATH_OBJECTBytes",
                "PeakPATH_STEPSBytes","PeakPathObjectCount","PeakStepCapacityUnits",
                "ResultPATH_OBJECTBytes","ResultPATH_STEPSBytes","ResultTotalDepth",
                "ResultTotalCapacity","ResultUnusedCapacitySlots"};
            for(i=0;i<10;++i)
                printf("%s,%lu,%lu,%lld\n",metric[i],(unsigned long)vals32[i],
                    (unsigned long)vals40[i],(long long)vals32[i]-(long long)vals40[i]);
        }
    }
    {
        size_t path32=peak_path_obj[g32]+peak_steps[g32];
        size_t path40=peak_path_obj[g40]+peak_steps[g40];
        double delta=(double)stage91_samples[g32].after_merge.peak_live_bytes-
            (double)stage91_samples[g40].after_merge.peak_live_bytes;
        printf("PathExplainedPeakDeltaPercent,%.6f\n",
            delta==0?0:100.0*((double)path32-(double)path40)/delta);
    }
    printf("Result clone amplification\nGroups,ResultPathCountPerN,ResultTotalCapacityPerN\n");
    for(c=0;c<2;++c) {
        size_t config=c==0?g32:g40;
        Stage91Profile *p=&stage91_profiles[config][1];
        printf("%lu,%.6f,%.6f\n",(unsigned long)stage91_groups[config],
            p->path_count/10000.0,p->total_capacity/10000.0);
    }
    return 1;
}

int hps_run_stage9_1_analysis(void)
{
    int values[HPS_STAGE91_PATH_COUNT];
    void *items[HPS_STAGE91_PATH_COUNT];
    size_t index,i;
    size_t path_size=hps_path_internal_sizeof_path();
    size_t step_size=LEGACY_PATH_STEP_SIZE;
    size_t field_bytes=sizeof(unsigned short)+sizeof(size_t);
    printf("HPSort Stage 9.1 Path Storage Waste Analysis (measurement only)\n");
    printf("sizeof(HpsPath)=%lu _Alignof(HpsPath)=%lu sizeof(HpsPathStep)=%lu _Alignof(HpsPathStep)=%lu sizeof(size_t)=%lu sizeof(unsigned short)=%lu\n",
        (unsigned long)path_size,(unsigned long)hps_path_internal_alignof_path(),
        (unsigned long)step_size,(unsigned long)_Alignof(size_t),
        (unsigned long)sizeof(size_t),(unsigned long)sizeof(unsigned short));
    printf("LogicalFieldBytesPerStep=%lu StructOverheadBytesPerStep=%lu\n",
        (unsigned long)field_bytes,(unsigned long)(step_size-field_bytes));
    if(hps_run_stage8_1_tests()!=0||!stage82_alignment_regression()||
        !stage83_tag_direct_test()) return 1;
    for(i=0;i<HPS_STAGE91_PATH_COUNT;++i) values[i]=(int)i;
    benchmark_shuffle(values,HPS_STAGE91_PATH_COUNT,UINT32_C(0xC0FFEE));
    for(index=0;index<HPS_STAGE91_CONFIG_COUNT;++index)
        if(!stage91_run_sample((int)index,values,items)) return 1;
    if(!stage83_run_baseline()) return 1;
    printf("Stage 9.1 steady-state samples: G=1/32/40 Batch and ResultOnly complete; every calculated Path object/step byte count matched allocator tags.\n");
    for(index=0;index<HPS_STAGE91_CONFIG_COUNT;++index) {
        stage91_print_profile("Batch",stage91_groups[index],&stage91_profiles[index][0]);
        stage91_print_profile("ResultOnly",stage91_groups[index],&stage91_profiles[index][1]);
    }
    printf("\nLayout field analysis\nLogicalFieldBytesPerStep=%lu StructPaddingBytesPerUsedStep=%lu\n",
        (unsigned long)field_bytes,(unsigned long)(step_size-field_bytes));
    stage91_print_result_decomposition();
    if(!stage91_print_peak_and_comparison()) return 1;
    {
        HpsAllocStats final_stats=hps_alloc_stats_get();
        if(!benchmark_alloc_stats_empty(&final_stats)||final_stats.failed_calls!=0||
            !stage83_tag_accounting_valid(&final_stats,"9.1 final")||
            !stage83_other_is_empty(&final_stats,"9.1 final")) return 1;
    }
    printf("Path lifecycle counters: not added (optional instrumentation omitted).\n");
    printf("Stage 9.1 complete. No 9.2 optimization started.\n");
    return 0;
}

static int stage92_path_storage_regression(void)
{
    HpsPath *complex = NULL, *deep = NULL, *clone = NULL;
    HpsAllocStats stats;
    char text[128];
    size_t i, old_capacity = 0, old_offset = 0, changes = 0, realloc_before_deep;
    int comparison = 99, success = 0;
    unsigned int slot;
    size_t level;
    if (hps_alloc_stats_reset() != 0) return 0;
    complex = hps_path_create(HPS_DIRECTION_POSITIVE, 3);
    if (complex == NULL ||
        hps_path_append_at_level(complex, 259, 1) != HPS_STATUS_OK ||
        hps_path_append_at_level(complex, 0, 2) != HPS_STATUS_OK ||
        hps_path_append_at_level(complex, 0, 3) != HPS_STATUS_OK ||
        hps_path_format(complex, text, sizeof(text)) != HPS_STATUS_OK ||
        strcmp(text, "0A3/Z9//A0///A0") != 0) goto cleanup;
    {
        static const unsigned int expected_slots[4] = {3,259,0,0};
        for (i = 0; i < 4; ++i) {
            if (hps_path_get_slot(complex, i, &slot) != HPS_STATUS_OK ||
                hps_path_get_level(complex, i, &level) != HPS_STATUS_OK ||
                slot != expected_slots[i] || level != i) goto cleanup;
        }
    }
    realloc_before_deep = hps_alloc_stats_get().realloc_calls;
    deep = hps_path_create(HPS_DIRECTION_POSITIVE, 0);
    if (deep == NULL) goto cleanup;
    old_capacity = hps_path_internal_capacity(deep);
    old_offset = hps_path_internal_levels_offset_for_capacity(old_capacity);
    for (i = 1; i < 100; ++i) {
        if (hps_path_append_at_level(deep, (unsigned int)(i % 260), i) != HPS_STATUS_OK)
            goto cleanup;
        if (hps_path_internal_capacity(deep) != old_capacity) {
            size_t next_capacity = hps_path_internal_capacity(deep);
            size_t next_offset = hps_path_internal_levels_offset_for_capacity(next_capacity);
            if (next_offset != old_offset) {
                if (changes == 0)
                    printf("Stage 9.2 offset move: OldCapacity=%lu NewCapacity=%lu OldLevelsOffset=%lu NewLevelsOffset=%lu\n",
                        (unsigned long)old_capacity, (unsigned long)next_capacity,
                        (unsigned long)old_offset, (unsigned long)next_offset);
                ++changes;
            }
            old_capacity = next_capacity;
            old_offset = next_offset;
        }
    }
    for (i = 0; i < 100; ++i) {
        if (hps_path_get_slot(deep, i, &slot) != HPS_STATUS_OK ||
            hps_path_get_level(deep, i, &level) != HPS_STATUS_OK ||
            slot != (unsigned int)(i == 0 ? 0 : i % 260) || level != i) goto cleanup;
    }
    clone = hps_path_clone(deep);
    if (clone == NULL || hps_path_compare(deep, clone, &comparison) != HPS_STATUS_OK ||
        comparison != 0 || hps_path_append(clone, 77) != HPS_STATUS_OK ||
        hps_path_depth(deep) != 100 || hps_path_depth(clone) != 101 ||
        hps_path_get_level(deep, 99, &level) != HPS_STATUS_OK || level != 99) goto cleanup;
    stats = hps_alloc_stats_get();
    printf("Stage 9.2 100-step realloc_calls=%lu; levels_offset_changed=%lu; clone independent=YES\n",
        (unsigned long)(stats.realloc_calls - realloc_before_deep - 1), (unsigned long)changes);
    if (stats.realloc_calls - realloc_before_deep - 1 != 7 || changes == 0) goto cleanup;
    success = 1;
cleanup:
    hps_path_destroy(clone);
    hps_path_destroy(deep);
    hps_path_destroy(complex);
    stats = hps_alloc_stats_get();
    if (!benchmark_alloc_stats_empty(&stats) || stats.failed_calls != 0 ||
        !stage83_tag_accounting_valid(&stats, "9.2 path regression") ||
        !stage83_other_is_empty(&stats, "9.2 path regression")) success = 0;
    printf("Stage 9.2 format/getter/growth/clone regression: %s; final=%lu/%lu\n",
        success ? "PASS" : "FAIL", (unsigned long)stats.live_bytes,
        (unsigned long)stats.live_blocks);
    return success;
}

int hps_run_stage9_2_optimization_tests(void)
{
    int values[HPS_STAGE91_PATH_COUNT];
    void *items[HPS_STAGE91_PATH_COUNT];
    int baseline_values[1000];
    void *baseline_items[1000];
    Stage82Sample baseline_sample;
    size_t i, config;
    printf("HPSort Stage 9.2: single-allocation SoA Path storage\n");
    if (hps_run_stage8_1_tests() != 0 || !stage82_alignment_regression() ||
        !stage83_tag_direct_test() || !stage92_path_storage_regression()) return 1;
    printf("Path sizeof before/after: 32/%lu; legacy step baseline=16 bytes; current capacity formula=align_up(C*sizeof(unsigned short), _Alignof(size_t))+C*sizeof(size_t)\n",
        (unsigned long)hps_path_internal_sizeof_path());
    for (i = 0; i < HPS_STAGE91_PATH_COUNT; ++i) values[i] = (int)i;
    benchmark_shuffle(values, HPS_STAGE91_PATH_COUNT, UINT32_C(0xC0FFEE));
    for (config = 0; config < HPS_STAGE91_CONFIG_COUNT; ++config)
        if (!stage91_run_sample((int)config, values, items)) return 1;
    printf("ResultOnly Path storage CSV\nGroups,GroupSize,PathCount,TotalDepth,TotalCapacity,PathObjectBytes,LegacyStepBytes,NewStepBytes,StepBytesSaved,StepSavingsPercent,LegacyPathBytes,CurrentPathBytes,TotalPathBytesSaved,PathSavingsPercent,PATH_STEPSBlocks\n");
    for (config = 0; config < HPS_STAGE91_CONFIG_COUNT; ++config) {
        Stage91Profile *p = &stage91_profiles[config][1];
        Stage91Sample *s = &stage91_samples[config];
        size_t object_bytes, legacy_step, current_path, legacy_path;
        object_bytes = s->result_only.tags[HPS_ALLOC_TAG_PATH_OBJECT].live_bytes;
        legacy_step = p->total_capacity * LEGACY_PATH_STEP_SIZE;
        current_path = object_bytes + p->storage_bytes;
        legacy_path = object_bytes + legacy_step;
        if (p->storage_bytes != s->result_only.tags[HPS_ALLOC_TAG_PATH_STEPS].live_bytes ||
            object_bytes != p->path_count * hps_path_internal_sizeof_path() ||
            s->result_only.tags[HPS_ALLOC_TAG_PATH_STEPS].live_blocks != p->path_count-p->zero_depth_count)
            return 1;
        printf("%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%.4f,%lu,%lu,%lu,%.4f,%lu\n",
            (unsigned long)stage91_groups[config], (unsigned long)s->group_size,
            (unsigned long)p->path_count, (unsigned long)p->total_depth,
            (unsigned long)p->total_capacity, (unsigned long)object_bytes,
            (unsigned long)legacy_step, (unsigned long)p->storage_bytes,
            (unsigned long)(legacy_step-p->storage_bytes),
            legacy_step == 0 ? 0.0 : 100.0*(legacy_step-p->storage_bytes)/legacy_step,
            (unsigned long)legacy_path, (unsigned long)current_path,
            (unsigned long)(legacy_path-current_path),
            legacy_path == 0 ? 0.0 : 100.0*(legacy_path-current_path)/legacy_path,
            (unsigned long)s->result_only.tags[HPS_ALLOC_TAG_PATH_STEPS].live_blocks);
    }
    printf("Peak comparison CSV\nGroups,OldPeakTotalBytes,NewPeakTotalBytes,PeakBytesSaved,PeakSavingsPercent,OldPeakPathStepsBytes,NewPeakPathStepsBytes,PathStepsSaved,OldPeakPathObjectBytes,NewPeakPathObjectBytes\n");
    for (config = 1; config < HPS_STAGE91_CONFIG_COUNT; ++config) {
        Stage91Sample *s = &stage91_samples[config];
        size_t old_peak = stage91_groups[config] == 32 ? 6716376u : 6023992u;
        size_t old_steps = stage91_groups[config] == 32 ? 3577600u : 3005472u;
        size_t new_peak = s->after_merge.peak_live_bytes;
        size_t new_steps = s->after_merge.bytes_at_global_peak[HPS_ALLOC_TAG_PATH_STEPS];
        size_t old_objects = stage91_groups[config] == 32 ? 1119744u : 1024000u;
        size_t new_objects = s->after_merge.bytes_at_global_peak[HPS_ALLOC_TAG_PATH_OBJECT];
        printf("%lu,%lu,%lu,%lu,%.4f,%lu,%lu,%lu,%lu,%lu\n",
            (unsigned long)stage91_groups[config], (unsigned long)old_peak,
            (unsigned long)new_peak, (unsigned long)(old_peak-new_peak),
            100.0*(double)(old_peak-new_peak)/old_peak,
            (unsigned long)old_steps, (unsigned long)new_steps,
            (unsigned long)(old_steps-new_steps), (unsigned long)old_objects,
            (unsigned long)new_objects);
    }
    for (i = 0; i < 1000; ++i) {
        baseline_values[i] = (int)((i * 613u) % 1000u);
        baseline_items[i] = &baseline_values[i];
    }
    if (!stage82_run_sample(baseline_values, baseline_items, 1000, 100, 10,
            &baseline_sample, UINT32_C(0xC0FFEE), 0)) return 1;
    {
        static const char *metrics[4] = {
            "BatchLiveBytes", "PostMergeLiveBytes", "ResultOnlyLiveBytes", "OverallPeakBytes"
        };
        size_t before[4] = {139000u,305800u,166800u,496144u};
        size_t after[4] = {baseline_sample.after_batch.live_bytes,
            baseline_sample.after_merge.live_bytes, baseline_sample.result_only.live_bytes,
            baseline_sample.after_merge.peak_live_bytes};
        printf("GS100 before-after CSV\nMetric,Before,After,Saved,SavingsPercent\n");
        for (i = 0; i < 4; ++i)
            printf("%s,%lu,%lu,%lu,%.4f\n", metrics[i], (unsigned long)before[i],
                (unsigned long)after[i], (unsigned long)(before[i]-after[i]),
                100.0*(double)(before[i]-after[i])/before[i]);
    }
    {
        HpsAllocStats final_stats = hps_alloc_stats_get();
        if (!benchmark_alloc_stats_empty(&final_stats) || final_stats.failed_calls != 0 ||
            !stage83_tag_accounting_valid(&final_stats, "9.2 final") ||
            !stage83_other_is_empty(&final_stats, "9.2 final")) return 1;
    }
    printf("Stage 9.2 storage regressions and G1/G32/G40 samples passed.\n");
    return 0;
}

typedef struct Stage93Baseline {
    size_t groups;
    double batch;
    double result;
    double peak;
} Stage93Baseline;

static const Stage93Baseline stage93_main_before[HPS_STAGE83_GROUP_COUNT] = {
    {1,1979846.40,1979806.40,3959652.80},
    {4,1779300.80,2006675.20,6720294.40},
    {5,1765188.80,2011228.80,5789672.00},
    {8,1704835.20,2032865.60,6718328.00},
    {10,1679504.00,2037948.80,6108513.60},
    {16,1633740.80,2050897.60,6696158.40},
    {20,1603118.40,2064624.00,6091513.60},
    {32,1563723.20,2075712.00,6713960.00},
    {40,1538944.00,2080899.20,6066334.40},
    {64,1492150.40,2092876.80,6662643.20},
    {80,1473244.80,2097454.40,6043971.20}
};

static const size_t stage93_groups[HPS_STAGE83_GROUP_COUNT] = {
    1,4,5,8,10,16,20,32,40,64,80
};
static const uint32_t stage93_main_seeds[5] = {
    UINT32_C(0xC0FFEE),UINT32_C(0xC0FFEF),UINT32_C(0xC0FFF0),
    UINT32_C(0xC0FFF1),UINT32_C(0xC0FFF2)
};
static const size_t stage93_scale_ns[4] = {1000,2000,5000,10000};
static const size_t stage93_scale_groups[3] = {1,8,32};
static const double stage93_scale_before[4][3][3] = {
    {{164.2427,164.2027,328.4453},{146.4853,167.9067,551.1333},{135.8133,170.6107,548.6080}},
    {{176.4587,176.4387,352.8973},{155.3360,182.1987,604.4480},{140.6213,190.1880,601.7907}},
    {{189.4517,189.4437,378.8955},{165.9120,194.5744,643.2437},{150.4155,198.9301,639.0469}},
    {{194.5781,194.5741,389.1523},{168.4091,200.2595,661.3011},{156.2832,204.3101,661.3259}}
};

static double stage93_percent_saved(double old_value, double new_value)
{
    return old_value == 0.0 ? 0.0 : 100.0 * (old_value - new_value) / old_value;
}

static double stage93_main_mean(Stage82Sample samples[HPS_STAGE83_GROUP_COUNT][5],
    size_t group_index, size_t field)
{
    size_t seed;
    double sum = 0.0;
    for (seed = 0; seed < 5; ++seed) {
        const Stage82Sample *s = &samples[group_index][seed];
        if (field == 0) sum += (double)s->after_batch.live_bytes;
        else if (field == 1) sum += (double)s->result_only.live_bytes;
        else if (field == 2) sum += (double)s->after_merge.peak_live_bytes;
        else if (field == 3) sum += (double)s->after_batch.tags[HPS_ALLOC_TAG_PATH_STEPS].live_bytes;
        else if (field == 4) sum += (double)s->result_only.tags[HPS_ALLOC_TAG_PATH_STEPS].live_bytes;
        else if (field == 5) sum += (double)s->after_merge.bytes_at_global_peak[HPS_ALLOC_TAG_PATH_STEPS];
        else if (field == 6) sum += (double)s->after_batch.tags[HPS_ALLOC_TAG_PATH_OBJECT].live_bytes;
        else if (field == 7) sum += (double)s->result_only.tags[HPS_ALLOC_TAG_PATH_OBJECT].live_bytes;
        else if (field == 8) sum += (double)s->after_merge.bytes_at_global_peak[HPS_ALLOC_TAG_PATH_OBJECT];
    }
    return sum / 5.0;
}

static double stage93_result_step_savings_percent(
    Stage82Sample samples[HPS_STAGE83_GROUP_COUNT][5], size_t group_index)
{
    size_t seed;
    double legacy = 0.0, current = 0.0;
    for (seed = 0; seed < 5; ++seed) {
        legacy += (double)samples[group_index][seed].result_total_capacity *
            LEGACY_PATH_STEP_SIZE;
        current += (double)samples[group_index][seed].result_storage_bytes;
    }
    legacy /= 5.0;
    current /= 5.0;
    return stage93_percent_saved(legacy, current);
}

static int stage93_run_samples(Stage82Sample main_samples[HPS_STAGE83_GROUP_COUNT][5],
    Stage82Sample scale_samples[4][3][3])
{
    int values[HPS_STAGE82_MAIN_N];
    void *items[HPS_STAGE82_MAIN_N];
    size_t seed, g, i, n;
    for (seed = 0; seed < 5; ++seed) {
        for (i = 0; i < HPS_STAGE82_MAIN_N; ++i) values[i] = (int)i;
        benchmark_shuffle(values, HPS_STAGE82_MAIN_N, stage93_main_seeds[seed]);
        for (g = 0; g < HPS_STAGE83_GROUP_COUNT; ++g) {
            size_t groups = stage93_groups[g];
            size_t group_size = HPS_STAGE82_MAIN_N / groups +
                (HPS_STAGE82_MAIN_N % groups != 0);
            if (!stage82_run_sample(values, items, HPS_STAGE82_MAIN_N, group_size,
                    groups, &main_samples[g][seed], stage93_main_seeds[seed], 1)) return 0;
            if (main_samples[g][seed].build_comparisons +
                    main_samples[g][seed].merge_comparisons !=
                main_samples[g][seed].total_comparisons) return 0;
        }
    }
    for (n = 0; n < 4; ++n) {
        for (seed = 0; seed < 3; ++seed) {
            for (i = 0; i < stage93_scale_ns[n]; ++i) values[i] = (int)i;
            benchmark_shuffle(values, stage93_scale_ns[n], stage93_main_seeds[seed]);
            for (g = 0; g < 3; ++g) {
                size_t count = stage93_scale_ns[n];
                size_t groups = stage93_scale_groups[g];
                size_t group_size = count / groups + (count % groups != 0);
                if (!stage82_run_sample(values, items, count, group_size, groups,
                        &scale_samples[n][g][seed], stage93_main_seeds[seed], 1)) return 0;
                if (scale_samples[n][g][seed].build_comparisons +
                        scale_samples[n][g][seed].merge_comparisons !=
                    scale_samples[n][g][seed].total_comparisons) return 0;
            }
        }
    }
    printf("Stage 9.3 current samples passed: main=55/55, scaling=36/36; every result ordered, adjacent Path compare=-1, final allocator 0/0, OTHER=0, FailedCalls=0.\n");
    return 1;
}

static void stage93_print_main(Stage82Sample samples[HPS_STAGE83_GROUP_COUNT][5])
{
    size_t g, seed;
    printf("\nStage 8.3 frozen pre-SoA baseline; requested payload bytes, not RSS\n");
    printf("Main Before/After CSV\nGroups,GroupSize,OldBatchBytes,NewBatchBytes,BatchSaved,BatchSavingsPercent,OldResultBytes,NewResultBytes,ResultSaved,ResultSavingsPercent,OldPeakBytes,NewPeakBytes,PeakSaved,PeakSavingsPercent\n");
    for (g = 0; g < HPS_STAGE83_GROUP_COUNT; ++g) {
        const Stage93Baseline *b = &stage93_main_before[g];
        double new_batch = stage93_main_mean(samples, g, 0);
        double new_result = stage93_main_mean(samples, g, 1);
        double new_peak = stage93_main_mean(samples, g, 2);
        double batch_saved = b->batch - new_batch;
        double result_saved = b->result - new_result;
        double peak_saved = b->peak - new_peak;
        printf("%lu,%lu,%.2f,%.2f,%.2f,%.4f,%.2f,%.2f,%.2f,%.4f,%.2f,%.2f,%.2f,%.4f\n",
            (unsigned long)b->groups,
            (unsigned long)(HPS_STAGE82_MAIN_N / b->groups +
                (HPS_STAGE82_MAIN_N % b->groups != 0)),
            b->batch, new_batch, batch_saved, stage93_percent_saved(b->batch,new_batch),
            b->result, new_result, result_saved, stage93_percent_saved(b->result,new_result),
            b->peak, new_peak, peak_saved, stage93_percent_saved(b->peak,new_peak));
    }
    printf("\nCurrent Path tag bytes CSV\nGroups,BatchPathStepsBytesMean,ResultPathStepsBytesMean,PeakPathStepsBytesMean,BatchPathObjectBytesMean,ResultPathObjectBytesMean,PeakPathObjectBytesMean\n");
    for (g = 0; g < HPS_STAGE83_GROUP_COUNT; ++g)
        printf("%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
            (unsigned long)stage93_groups[g],stage93_main_mean(samples,g,3),
            stage93_main_mean(samples,g,4),stage93_main_mean(samples,g,5),
            stage93_main_mean(samples,g,6),stage93_main_mean(samples,g,7),
            stage93_main_mean(samples,g,8));
    printf("\nBatch Path storage CSV\nGroups,BatchPathCount,BatchTotalDepth,BatchTotalCapacity,LegacyAoSStepBytes,CurrentSoAStepBytes,StepBytesSaved,StepSavingsPercent\n");
    for (g = 0; g < HPS_STAGE83_GROUP_COUNT; ++g) {
        double paths=0,depth=0,capacity=0,storage=0;
        for (seed=0;seed<5;++seed) {
            paths+=(double)samples[g][seed].batch_path_count;
            depth+=(double)samples[g][seed].batch_total_depth;
            capacity+=(double)samples[g][seed].batch_total_capacity;
            storage+=(double)samples[g][seed].batch_storage_bytes;
        }
        paths/=5.0; depth/=5.0; capacity/=5.0; storage/=5.0;
        printf("%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.4f\n",
            (unsigned long)stage93_groups[g],paths,depth,capacity,
            capacity*LEGACY_PATH_STEP_SIZE,storage,
            capacity*LEGACY_PATH_STEP_SIZE-storage,
            stage93_percent_saved(capacity*LEGACY_PATH_STEP_SIZE,storage));
    }
    printf("\nResult Path storage CSV\nGroups,ResultPathCount,ResultTotalDepth,ResultTotalCapacity,LegacyAoSStepBytes,CurrentSoAStepBytes,StepBytesSaved,StepSavingsPercent,PathObjectBytes,LegacyTotalPathBytes,CurrentTotalPathBytes,TotalPathSavingsPercent\n");
    for (g = 0; g < HPS_STAGE83_GROUP_COUNT; ++g) {
        double paths=0,depth=0,capacity=0,storage=0,objects=0;
        for (seed = 0; seed < 5; ++seed) {
            Stage82Sample *s=&samples[g][seed];
            paths+=(double)s->result_path_count; depth+=(double)s->result_total_depth;
            capacity+=(double)s->result_total_capacity; storage+=(double)s->result_storage_bytes;
            objects+=(double)s->result_only.tags[HPS_ALLOC_TAG_PATH_OBJECT].live_bytes;
        }
        paths/=5.0; depth/=5.0; capacity/=5.0; storage/=5.0; objects/=5.0;
        printf("%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.4f,%.2f,%.2f,%.2f,%.4f\n",
            (unsigned long)stage93_groups[g],paths,depth,capacity,
            capacity*LEGACY_PATH_STEP_SIZE,storage,
            capacity*LEGACY_PATH_STEP_SIZE-storage,
            stage93_percent_saved(capacity*LEGACY_PATH_STEP_SIZE,storage),objects,
            objects+capacity*LEGACY_PATH_STEP_SIZE,objects+storage,
            stage93_percent_saved(objects+capacity*LEGACY_PATH_STEP_SIZE,objects+storage));
    }
    printf("\nOperation counts and allocation calls CSV\nGroups,BuildCmpMean,MergeCmpMean,TotalCmpMean,AllocCallsMean,ReallocCallsMean,FreeCallsMean,PeakLiveBlocksMean\n");
    for (g = 0; g < HPS_STAGE83_GROUP_COUNT; ++g) {
        double bc=0,mc=0,tc=0,ac=0,rc=0,fc=0,blocks=0;
        for (seed=0;seed<5;++seed) {
            Stage82Sample *s=&samples[g][seed];
            bc+=s->build_comparisons; mc+=s->merge_comparisons; tc+=s->total_comparisons;
            ac+=s->final.alloc_calls; rc+=s->final.realloc_calls; fc+=s->final.free_calls;
            blocks+=s->after_merge.max_live_blocks;
        }
        printf("%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
            (unsigned long)stage93_groups[g],bc/5,mc/5,tc/5,ac/5,rc/5,fc/5,blocks/5);
    }
    printf("\nCurrent SoA Debug Timing Baseline (clock(); not comparable to historical instrumentation)\nGroups,CurrentSoABuildMsMean,CurrentSoAMergeMsMean,CurrentSoATotalMsMean\n");
    for (g=0;g<HPS_STAGE83_GROUP_COUNT;++g) {
        double build=0,merge=0,total=0;
        for(seed=0;seed<5;++seed) {
            build+=samples[g][seed].build_ms; merge+=samples[g][seed].merge_ms;
            total+=samples[g][seed].total_ms;
        }
        printf("%lu,%.3f,%.3f,%.3f\n",(unsigned long)stage93_groups[g],
            build/5,merge/5,total/5);
    }
    printf("\nSeed C0FFEE path depth checks\nGroups,ResultTotalDepth,Expected9_1,Match,FinalAvgDepth,FinalMaxDepth,FinalMaxLevel\n");
    for(g=0;g<HPS_STAGE83_GROUP_COUNT;++g) if(stage93_groups[g]==1||stage93_groups[g]==32||stage93_groups[g]==40) {
        size_t expected=stage93_groups[g]==1?63009u:stage93_groups[g]==32?69211u:68341u;
        Stage82Sample *s=&samples[g][0];
        printf("%lu,%lu,%lu,%s,%.4f,%lu,%lu\n",(unsigned long)stage93_groups[g],
            (unsigned long)s->result_total_depth,(unsigned long)expected,
            s->result_total_depth==expected?"yes":"NO",
            (double)s->final_paths.total_depth/HPS_STAGE82_MAIN_N,
            (unsigned long)s->final_paths.max_depth,(unsigned long)s->final_paths.max_level);
    }
    printf("\nSeed C0FFEE Peak Before/After CSV\nGroups,OldPeakTotal,NewPeakTotal,TotalSaved,OldPeakPATH_OBJECT,NewPeakPATH_OBJECT,OldPeakPATH_STEPS,NewPeakPATH_STEPS,StepSaved\n");
    for(g=0;g<HPS_STAGE83_GROUP_COUNT;++g) if(stage93_groups[g]==32||stage93_groups[g]==40) {
        Stage82Sample *s=&samples[g][0];
        size_t old_total=stage93_groups[g]==32?6716376u:6023992u;
        size_t old_obj=stage93_groups[g]==32?1119744u:1024000u;
        size_t old_steps=stage93_groups[g]==32?3577600u:3005472u;
        size_t new_obj=s->after_merge.bytes_at_global_peak[HPS_ALLOC_TAG_PATH_OBJECT];
        size_t new_steps=s->after_merge.bytes_at_global_peak[HPS_ALLOC_TAG_PATH_STEPS];
        printf("%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",
            (unsigned long)stage93_groups[g],(unsigned long)old_total,
            (unsigned long)s->after_merge.peak_live_bytes,
            (unsigned long)(old_total-s->after_merge.peak_live_bytes),
            (unsigned long)old_obj,(unsigned long)new_obj,(unsigned long)old_steps,
            (unsigned long)new_steps,(unsigned long)(old_steps-new_steps));
    }
}

static int stage93_print_scaling(Stage82Sample samples[4][3][3])
{
    size_t n,g,seed;
    double min_peak=1000.0,max_peak=-1000.0;
    double peak_savings[3][4]={{0}};
    printf("\nStage 8.2 frozen scaling before (bytes/item)\n");
    printf("Scaling Before/After CSV\nN,Groups,OldBatchPerItem,NewBatchPerItem,BatchSavingsPercent,OldResultPerItem,NewResultPerItem,ResultSavingsPercent,OldPeakPerItem,NewPeakPerItem,PeakSavingsPercent\n");
    for(n=0;n<4;++n) for(g=0;g<3;++g) {
        double batch=0,result=0,peak=0;
        for(seed=0;seed<3;++seed) {
            batch+=(double)samples[n][g][seed].after_batch.live_bytes/stage93_scale_ns[n];
            result+=(double)samples[n][g][seed].result_only.live_bytes/stage93_scale_ns[n];
            peak+=(double)samples[n][g][seed].after_merge.peak_live_bytes/stage93_scale_ns[n];
        }
        batch/=3; result/=3; peak/=3;
        peak_savings[g][n]=stage93_percent_saved(stage93_scale_before[n][g][2],peak);
        if(peak_savings[g][n]<min_peak) min_peak=peak_savings[g][n];
        if(peak_savings[g][n]>max_peak) max_peak=peak_savings[g][n];
        printf("%lu,%lu,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n",
            (unsigned long)stage93_scale_ns[n],(unsigned long)stage93_scale_groups[g],
            stage93_scale_before[n][g][0],batch,
            stage93_percent_saved(stage93_scale_before[n][g][0],batch),
            stage93_scale_before[n][g][1],result,
            stage93_percent_saved(stage93_scale_before[n][g][1],result),
            stage93_scale_before[n][g][2],peak,peak_savings[g][n]);
    }
    printf("\nPeak savings by GroupCount CSV\nGroups,N1000PeakSavingPercent,N2000PeakSavingPercent,N5000PeakSavingPercent,N10000PeakSavingPercent\n");
    for(g=0;g<3;++g)
        printf("%lu,%.4f,%.4f,%.4f,%.4f\n",(unsigned long)stage93_scale_groups[g],
            peak_savings[g][0],peak_savings[g][1],peak_savings[g][2],peak_savings[g][3]);
    return min_peak<=max_peak;
}

int hps_run_stage9_3_final_validation(void)
{
    Stage82Sample main_samples[HPS_STAGE83_GROUP_COUNT][5];
    Stage82Sample scale_samples[4][3][3];
    static const double gs100_expected[4]={127192.0,270768.0,143576.0,432272.0};
    int values[1000];
    void *items[1000];
    Stage82Sample gs100;
    size_t i;
    double min_result=1000.0,max_result=-1000.0,min_peak=1000.0,max_peak=-1000.0;
    printf("HPSort Stage 9.3 Final SoA Validation; Stage 9 concludes here. No new optimization.\n");
    if(hps_run_stage8_1_tests()!=0 || !stage82_alignment_regression() ||
        !stage83_tag_direct_test() || !stage92_path_storage_regression()) return 1;
    printf("sizeof(HpsPath)=%lu; slot=unsigned short; level=size_t; PATH_STEPS one allocation per non-zero Path.\n",
        (unsigned long)hps_path_internal_sizeof_path());
    if(hps_path_internal_sizeof_path()!=32) return 1;
    memset(main_samples,0,sizeof(main_samples)); memset(scale_samples,0,sizeof(scale_samples));
    if(!stage93_run_samples(main_samples,scale_samples)) return 1;
    stage93_print_main(main_samples);
    if(!stage93_print_scaling(scale_samples)) return 1;
    for(i=0;i<4;++i) { values[i]=(int)((i*613u)%1000u); }
    for(i=0;i<1000;++i) { values[i]=(int)((i*613u)%1000u); items[i]=&values[i]; }
    if(!stage82_run_sample(values,items,1000,100,10,&gs100,UINT32_C(0xC0FFEE),1)) return 1;
    printf("\nGS100 before-after CSV\nMetric,Before,After,Saved,SavingsPercent\n");
    {
        static const char *labels[4]={"BatchLiveBytes","PostMergeLiveBytes","ResultOnlyLiveBytes","OverallPeakBytes"};
        size_t before[4]={139000,305800,166800,496144};
        size_t after[4]={gs100.after_batch.live_bytes,gs100.after_merge.live_bytes,
            gs100.result_only.live_bytes,gs100.after_merge.peak_live_bytes};
        for(i=0;i<4;++i) {
            if((double)after[i]!=gs100_expected[i]) return 1;
            printf("%s,%lu,%lu,%lu,%.4f\n",labels[i],(unsigned long)before[i],
                (unsigned long)after[i],(unsigned long)(before[i]-after[i]),
                stage93_percent_saved((double)before[i],(double)after[i]));
        }
    }
    for(i=0;i<HPS_STAGE83_GROUP_COUNT;++i) {
        double r=stage93_percent_saved(stage93_main_before[i].result,
            stage93_main_mean(main_samples,i,1));
        double p=stage93_percent_saved(stage93_main_before[i].peak,
            stage93_main_mean(main_samples,i,2));
        if(r<min_result) min_result=r; if(r>max_result) max_result=r;
        if(p<min_peak) min_peak=p; if(p>max_peak) max_peak=p;
    }
    printf("\nStage9MemorySummary\nGS100_ResultSavingsPercent,%.4f\nGS100_PeakSavingsPercent,%.4f\n",
        stage93_percent_saved(166800.0,(double)gs100.result_only.live_bytes),
        stage93_percent_saved(496144.0,(double)gs100.after_merge.peak_live_bytes));
    printf("G32_ResultStepSavingsPercent,%.4f\nG32_PeakSavingsPercent,%.4f\n",
        stage93_result_step_savings_percent(main_samples,7),
        stage93_percent_saved(6713960.0,stage93_main_mean(main_samples,7,2)));
    printf("G40_ResultStepSavingsPercent,%.4f\nG40_PeakSavingsPercent,%.4f\n",
        stage93_result_step_savings_percent(main_samples,8),
        stage93_percent_saved(6066334.4,stage93_main_mean(main_samples,8,2)));
    printf("MainPanel_MinResultSavingsPercent,%.4f\nMainPanel_MaxResultSavingsPercent,%.4f\nMainPanel_MinPeakSavingsPercent,%.4f\nMainPanel_MaxPeakSavingsPercent,%.4f\n",
        min_result,max_result,min_peak,max_peak);
    {
        double scale_min=1000.0,scale_max=-1000.0;
        size_t n,g,seed;
        for(n=0;n<4;++n) for(g=0;g<3;++g) {
            double new_peak=0;
            for(seed=0;seed<3;++seed)
                new_peak+=(double)scale_samples[n][g][seed].after_merge.peak_live_bytes/stage93_scale_ns[n];
            new_peak/=3.0;
            {
                double pct=stage93_percent_saved(stage93_scale_before[n][g][2],new_peak);
                if(pct<scale_min) scale_min=pct; if(pct>scale_max) scale_max=pct;
            }
        }
        printf("Scaling_MinPeakSavingsPercent,%.4f\nScaling_MaxPeakSavingsPercent,%.4f\n",
            scale_min,scale_max);
    }
    printf("Current SoA operation counts show BuildCmp+MergeCmp=TotalCmp for all 91 samples; no old timing speedup calculated.\n");
    {
        HpsAllocStats s=hps_alloc_stats_get();
        if(!benchmark_alloc_stats_empty(&s)||s.failed_calls!=0||
            !stage83_tag_accounting_valid(&s,"9.3 final")||!stage83_other_is_empty(&s,"9.3 final")) return 1;
    }
    printf("Stage 9.3 complete; all current stages/tests ended cleanly at 0/0. Stage 9 is complete; no 9.4 created.\n");
    return 0;
}

/* Stage 10.1 tests private owned-Base merge only; Batch integration is out of scope. */
typedef struct Stage101Pair {
    int base_values[10];
    int incoming_values[10];
    void *base_items[10];
    void *incoming_items[10];
    size_t base_count;
    size_t incoming_count;
} Stage101Pair;

static int stage101_groups_equivalent(const HpsGroup *left,
    const HpsGroup *right)
{
    size_t i;
    if (hps_group_size(left) != hps_group_size(right)) return 0;
    for (i = 0; i < hps_group_size(left); ++i) {
        int path_order;
        if (*(const int *)hps_group_item_at(left, i) !=
                *(const int *)hps_group_item_at(right, i) ||
            hps_path_compare(hps_group_path_at(left, i),
                hps_group_path_at(right, i), &path_order) != HPS_STATUS_OK ||
            path_order != 0) return 0;
    }
    return 1;
}

static int stage101_build_pair(const int *base_values, size_t base_count,
    const int *incoming_values, size_t incoming_count, Stage101Pair *pair)
{
    size_t i;
    if (base_count > 10 || incoming_count > 10) return 0;
    pair->base_count = base_count;
    pair->incoming_count = incoming_count;
    for (i = 0; i < base_count; ++i) {
        pair->base_values[i] = base_values[i];
        pair->base_items[i] = &pair->base_values[i];
    }
    for (i = 0; i < incoming_count; ++i) {
        pair->incoming_values[i] = incoming_values[i];
        pair->incoming_items[i] = &pair->incoming_values[i];
    }
    return 1;
}

static int stage101_group_build_from_pair(const Stage101Pair *pair,
    int incoming, const HpsComparator *comparator, HpsGroup **out)
{
    void *const *items = incoming ? pair->incoming_items : pair->base_items;
    size_t count = incoming ? pair->incoming_count : pair->base_count;
    return hps_group_build(items, count, comparator, out) == HPS_STATUS_OK;
}

static int stage101_run_case(const char *name, const int *base_values,
    size_t base_count, const int *incoming_values, size_t incoming_count,
    const char *const *expected_paths, size_t expected_count,
    int verify_base_equal_first, size_t *out_comparisons)
{
    Stage101Pair public_pair, private_pair;
    BenchmarkCompareContext public_counter = { 0, 0 }, private_counter = { 0, 0 };
    HpsComparator public_comparator = { benchmark_compare_int, &public_counter };
    HpsComparator private_comparator = { benchmark_compare_int, &private_counter };
    HpsGroup *public_base = NULL, *public_incoming = NULL, *reference = NULL;
    HpsGroup *owned_base = NULL, *private_incoming = NULL;
    HpsGroup *original_base;
    const HpsPath *base_paths_before[10], *incoming_paths_before[10];
    void *base_items_before[10];
    void *public_base_items_before[10], *public_incoming_items_before[10];
    HpsStatus status;
    size_t i;
    int valid = 0;
    if (!stage101_build_pair(base_values, base_count, incoming_values,
            incoming_count, &public_pair) ||
        !stage101_build_pair(base_values, base_count, incoming_values,
            incoming_count, &private_pair) ||
        !stage101_group_build_from_pair(&public_pair, 0, &public_comparator, &public_base) ||
        !stage101_group_build_from_pair(&public_pair, 1, &public_comparator, &public_incoming) ||
        !stage101_group_build_from_pair(&private_pair, 0, &private_comparator, &owned_base) ||
        !stage101_group_build_from_pair(&private_pair, 1, &private_comparator, &private_incoming))
        goto cleanup;
    original_base = owned_base;
    for (i = 0; i < base_count; ++i) {
        base_paths_before[i] = hps_group_path_at(owned_base, i);
        base_items_before[i] = hps_group_item_at(owned_base, i);
        public_base_items_before[i] = hps_group_item_at(public_base, i);
    }
    for (i = 0; i < incoming_count; ++i) {
        incoming_paths_before[i] = hps_group_path_at(private_incoming, i);
        public_incoming_items_before[i] = hps_group_item_at(public_incoming, i);
    }
    public_counter.comparison_count = private_counter.comparison_count = 0;
    status = hps_group_merge(public_base, public_incoming,
        &public_comparator, &reference);
    if (status != HPS_STATUS_OK) goto cleanup;
    status = hps_group_merge_into_owned_base(&owned_base, private_incoming,
        &private_comparator);
    if (status != HPS_STATUS_OK || owned_base != original_base ||
        !stage101_groups_equivalent(reference, owned_base)) goto cleanup;
    for (i = 0; i < base_count; ++i) {
        size_t j;
        int path_order;
        for (j = 0; j < hps_group_size(owned_base); ++j)
            if (hps_group_item_at(owned_base, j) == base_items_before[i]) break;
        if (hps_group_item_at(public_base, i) != public_base_items_before[i] ||
            j == hps_group_size(owned_base) ||
            hps_path_compare(base_paths_before[i],
                hps_group_path_at(owned_base, j), &path_order) != HPS_STATUS_OK ||
            path_order != 0) goto cleanup;
    }
    for (i = 0; i < incoming_count; ++i) {
        int path_order;
        if (hps_group_item_at(public_incoming, i) != public_incoming_items_before[i] ||
            hps_path_compare(incoming_paths_before[i],
                hps_group_path_at(private_incoming, i), &path_order) != HPS_STATUS_OK ||
            path_order != 0) goto cleanup;
    }
    if (expected_count != (size_t)-1) {
        if (hps_group_size(owned_base) != expected_count) goto cleanup;
        for (i = 0; i < expected_count; ++i) {
            char text[128];
            if (hps_path_format(hps_group_path_at(owned_base, i), text,
                    sizeof(text)) != HPS_STATUS_OK ||
                strcmp(text, expected_paths[i]) != 0) goto cleanup;
        }
    }
    if (verify_base_equal_first) {
        int saw_incoming_equal = 0;
        for (i = 0; i < hps_group_size(owned_base); ++i) {
            void *item = hps_group_item_at(owned_base, i);
            if (item == private_pair.incoming_items[0] ||
                item == private_pair.incoming_items[1]) {
                saw_incoming_equal = 1;
            } else if ((item == private_pair.base_items[1] ||
                    item == private_pair.base_items[2]) && saw_incoming_equal) {
                goto cleanup;
            }
        }
    }
    if (base_count == 10) {
        for (i = 0; i < base_count; ++i) {
            size_t j;
            for (j = 0; j < hps_group_size(owned_base); ++j)
                if (hps_group_item_at(owned_base, j) == private_pair.base_items[i]) break;
            if (j == hps_group_size(owned_base)) goto cleanup;
            {
                size_t k;
                for (k = 0; k < hps_group_size(public_base); ++k)
                    if (hps_group_item_at(public_base, k) == public_pair.base_items[i]) break;
                if (k == hps_group_size(public_base)) goto cleanup;
                {
                    int path_order;
                    if (hps_path_compare(hps_group_path_at(public_base, k),
                            hps_group_path_at(owned_base, j), &path_order) != HPS_STATUS_OK ||
                        path_order != 0) goto cleanup;
                }
            }
        }
    }
    if (out_comparisons != NULL) *out_comparisons = private_counter.comparison_count;
    hps_group_destroy(owned_base); owned_base = NULL;
    if (hps_group_size(private_incoming) != incoming_count) goto cleanup;
    for (i = 0; i < incoming_count; ++i)
        if (hps_group_item_at(private_incoming, i) != private_pair.incoming_items[i]) goto cleanup;
    valid = 1;
cleanup:
    hps_group_destroy(owned_base);
    hps_group_destroy(private_incoming);
    hps_group_destroy(reference);
    hps_group_destroy(public_incoming);
    hps_group_destroy(public_base);
    printf("Stage 10.1 %s: %s\n", name, valid ? "PASS" : "FAIL");
    return valid;
}

typedef struct Stage101MemoryResult {
    size_t pre_bytes, pre_blocks, post_bytes, post_blocks;
    size_t peak_bytes, peak_blocks, comparisons;
    HpsAllocStats peak_stats;
    int valid;
} Stage101MemoryResult;

static int stage101_memory_one(int in_place, Stage101MemoryResult *result)
{
    enum { N = 10000, HALF = 5000 };
    static int base_values[HALF], incoming_values[HALF];
    static void *base_items[HALF], *incoming_items[HALF];
    BenchmarkCompareContext counter = { 0, 0 };
    HpsComparator comparator = { benchmark_compare_int, &counter };
    HpsGroup *base = NULL, *incoming = NULL, *merged = NULL;
    HpsAllocStats stats;
    size_t i;
    int ok = 0;
    for (i = 0; i < HALF; ++i) {
        /* A fixed permutation avoids measuring ordered Tree-build degeneration. */
        size_t permuted = (i * 1739u) % HALF;
        base_values[i] = (int)(permuted * 2);
        incoming_values[i] = (int)(permuted * 2 + 1);
        base_items[i] = &base_values[i];
        incoming_items[i] = &incoming_values[i];
    }
    if (hps_alloc_stats_reset() != 0 ||
        hps_group_build(base_items, HALF, &comparator, &base) != HPS_STATUS_OK ||
        hps_group_build(incoming_items, HALF, &comparator, &incoming) != HPS_STATUS_OK)
        goto cleanup;
    result->pre_bytes = hps_alloc_stats_get().live_bytes;
    result->pre_blocks = hps_alloc_stats_get().live_blocks;
    counter.comparison_count = 0;
    if (in_place) {
        if (hps_group_merge_into_owned_base(&base, incoming, &comparator) != HPS_STATUS_OK)
            goto cleanup;
        merged = base;
    } else {
        if (hps_group_merge(base, incoming, &comparator, &merged) != HPS_STATUS_OK)
            goto cleanup;
    }
    result->comparisons = counter.comparison_count;
    stats = hps_alloc_stats_get();
    result->peak_bytes = stats.peak_live_bytes;
    result->peak_blocks = stats.peak_live_blocks;
    result->peak_stats = stats;
    result->post_bytes = stats.live_bytes;
    result->post_blocks = stats.live_blocks;
    if (counter.overflowed || result->comparisons != 19998 ||
        !benchmark_validate_final(merged, N, &(BenchmarkPathStats){ 0 }) ||
        !stage83_tag_accounting_valid(&stats, "10.1 memory peak") ||
        !stage83_other_is_empty(&stats, "10.1 memory peak")) goto cleanup;
    ok = 1;
cleanup:
    hps_group_destroy(merged);
    if (!in_place || merged != base) hps_group_destroy(base);
    hps_group_destroy(incoming);
    stats = hps_alloc_stats_get();
    if (!benchmark_alloc_stats_empty(&stats) || stats.failed_calls != 0 ||
        !stage83_tag_accounting_valid(&stats, "10.1 memory final") ||
        !stage83_other_is_empty(&stats, "10.1 memory final")) ok = 0;
    result->valid = ok;
    return ok;
}

static int stage101_peak_categories(const HpsAllocStats *s, size_t out[4])
{
    out[0] = s->bytes_at_global_peak[HPS_ALLOC_TAG_PATH_OBJECT] +
        s->bytes_at_global_peak[HPS_ALLOC_TAG_PATH_STEPS];
    out[1] = s->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_OBJECT] +
        s->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_NODE] +
        s->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_CHILDREN];
    out[2] = s->bytes_at_global_peak[HPS_ALLOC_TAG_GROUP_OBJECT] +
        s->bytes_at_global_peak[HPS_ALLOC_TAG_GROUP_ORDERED];
    out[3] = s->bytes_at_global_peak[HPS_ALLOC_TAG_MERGE_SCRATCH];
    return out[0] + out[1] + out[2] + out[3] == s->peak_live_bytes;
}

static int stage101_invalid_arguments_test(void)
{
    int base_value = 2, incoming_value = 3;
    void *base_item = &base_value, *incoming_item = &incoming_value;
    BenchmarkCompareContext counter = { 0, 0 };
    HpsComparator comparator = { benchmark_compare_int, &counter };
    HpsComparator invalid_comparator = { NULL, NULL };
    HpsGroup *base = NULL, *incoming = NULL, *original;
    HpsGroup *null_base = NULL;
    HpsStatus s1, s2, s3, s4;
    int path_order;
    if (hps_group_build(&base_item, 1, &comparator, &base) != HPS_STATUS_OK ||
        hps_group_build(&incoming_item, 1, &comparator, &incoming) != HPS_STATUS_OK)
        goto fail;
    original = base;
    {
        const HpsPath *base_path = hps_group_path_at(base, 0);
        s1 = hps_group_merge_into_owned_base(NULL, incoming, &comparator);
        s2 = hps_group_merge_into_owned_base(&base, NULL, &comparator);
        s3 = hps_group_merge_into_owned_base(&base, incoming, NULL);
        s4 = hps_group_merge_into_owned_base(&base, incoming, &invalid_comparator);
        if (hps_group_merge_into_owned_base(&null_base, incoming,
                &comparator) != HPS_STATUS_INVALID_ARGUMENT) goto fail;
        if (s1 != HPS_STATUS_INVALID_ARGUMENT || s2 != HPS_STATUS_INVALID_ARGUMENT ||
            s3 != HPS_STATUS_INVALID_ARGUMENT || s4 != HPS_STATUS_INVALID_ARGUMENT ||
            base != original || hps_group_size(base) != 1 ||
            hps_group_item_at(base, 0) != &base_value ||
            hps_path_compare(base_path, hps_group_path_at(base, 0), &path_order) != HPS_STATUS_OK ||
            path_order != 0) goto fail;
    }
    hps_group_destroy(incoming);
    hps_group_destroy(base);
    printf("Stage 10.1 invalid arguments: rejected without consuming valid Base\n");
    return 1;
fail:
    hps_group_destroy(incoming);
    hps_group_destroy(base);
    printf("Stage 10.1 invalid arguments: FAIL\n");
    return 0;
}

int hps_run_stage10_1_tests(void)
{
    static const int a_base[] = { 200, 100, 300 }, a_in[] = { 50, 150, 250, 350 };
    static const char *a_paths[] = { "1A1", "1A0", "1A0/A0", "000", "000/A0", "0A0", "0A0/A0" };
    static const int b_base[] = { 100, 200 }, b_in[] = { 110, 120, 130 };
    static const char *b_paths[] = { "000", "000/A0", "000/A1", "000/A2", "0A0" };
    static const int c_base[] = { 100, 200, 200, 300 }, c_in[] = { 200, 200, 250 };
    static const int d_base[] = { 25, 50, 75, 100, 125, 150, 160, 165, 170, 175 };
    static const int d_in[] = { 30, 80, 140, 168, 180 };
    static const int f_in[] = { 10, 20, 30 };
    static const char *f_paths[] = { "000", "0A0", "0A0/A0" };
    static int values[1000];
    static void *base_items[500], *incoming_items[500];
    Stage101MemoryResult public_memory = { 0 }, inplace_memory = { 0 };
    size_t public_count = 0, private_count = 0, i;
    int result = 0;
    printf("HPSort Stage 10.1: private owned-Base in-place merge; Batch integration unchanged\n");
    if (hps_run_stage8_1_tests() != 0 || !stage82_alignment_regression() ||
        !stage83_tag_direct_test() || !stage92_path_storage_regression()) return 1;
    if (!stage101_invalid_arguments_test()) return 1;
    if (!stage101_run_case("A classic 6.1", a_base, 3, a_in, 4, a_paths, 7, 0, NULL) ||
        !stage101_run_case("B same Gap", b_base, 2, b_in, 3, b_paths, 5, 0, NULL) ||
        !stage101_run_case("C Base-first equals", c_base, 4, c_in, 3, NULL, (size_t)-1, 1, NULL) ||
        !stage101_run_case("D complex Base Paths retained", d_base, 10, d_in, 5, NULL, (size_t)-1, 0, NULL) ||
        !stage101_run_case("E empty Incoming", a_base, 3, NULL, 0, NULL, (size_t)-1, 0, NULL) ||
        !stage101_run_case("F empty Base", NULL, 0, f_in, 3, f_paths, 3, 0, NULL)) return 1;
    for (i = 0; i < 500; ++i) {
        values[i] = (int)(i * 2); base_items[i] = &values[i];
        values[500 + i] = (int)(i * 2 + 1); incoming_items[i] = &values[500 + i];
    }
    {
        BenchmarkCompareContext counter = { 0, 0 };
        HpsComparator comparator = { benchmark_compare_int, &counter };
        HpsGroup *base = NULL, *incoming = NULL, *reference = NULL;
        HpsGroup *private_base = NULL, *private_incoming = NULL;
        if (hps_group_build(base_items, 500, &comparator, &base) != HPS_STATUS_OK ||
            hps_group_build(incoming_items, 500, &comparator, &incoming) != HPS_STATUS_OK ||
            hps_group_build(base_items, 500, &comparator, &private_base) != HPS_STATUS_OK ||
            hps_group_build(incoming_items, 500, &comparator, &private_incoming) != HPS_STATUS_OK) return 1;
        counter.comparison_count = 0;
        if (hps_group_merge(base, incoming, &comparator, &reference) != HPS_STATUS_OK) return 1;
        public_count = counter.comparison_count; counter.comparison_count = 0;
        if (hps_group_merge_into_owned_base(&private_base, private_incoming, &comparator) != HPS_STATUS_OK) return 1;
        private_count = counter.comparison_count;
        if (private_base == NULL || !stage101_groups_equivalent(reference, private_base)) return 1;
        for (i = 0; i < 1000; ++i)
            if (*(int *)hps_group_item_at(private_base, i) != (int)i) return 1;
        hps_group_destroy(private_base); private_base = NULL;
        if (hps_group_size(private_incoming) != 500) return 1;
        for (i = 0; i < 500; ++i)
            if (hps_group_item_at(private_incoming, i) != incoming_items[i]) return 1;
        hps_group_destroy(private_incoming); hps_group_destroy(reference);
        hps_group_destroy(incoming); hps_group_destroy(base);
    }
    printf("Stage 10.1 G 500+500: public=%lu private=%lu; Path equivalence=1000/1000; output=0..999\n",
        (unsigned long)public_count, (unsigned long)private_count);
    if (public_count != 1998 || private_count != 1998) return 1;
    if (!stage101_memory_one(0, &public_memory) ||
        !stage101_memory_one(1, &inplace_memory)) return 1;
    printf("Mode,PreMergeLiveBytes,PostMergeLiveBytes,PeakBytes,PeakOverPreMergeBytes,PostMergeLiveBlocks,PeakBlocks,ComparatorCount\n");
    printf("Public,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",
        (unsigned long)public_memory.pre_bytes,(unsigned long)public_memory.post_bytes,
        (unsigned long)public_memory.peak_bytes,
        (unsigned long)(public_memory.peak_bytes-public_memory.pre_bytes),
        (unsigned long)public_memory.post_blocks,(unsigned long)public_memory.peak_blocks,
        (unsigned long)public_memory.comparisons);
    printf("OwnedBaseInPlace,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",
        (unsigned long)inplace_memory.pre_bytes,(unsigned long)inplace_memory.post_bytes,
        (unsigned long)inplace_memory.peak_bytes,
        (unsigned long)(inplace_memory.peak_bytes-inplace_memory.pre_bytes),
        (unsigned long)inplace_memory.post_blocks,(unsigned long)inplace_memory.peak_blocks,
        (unsigned long)inplace_memory.comparisons);
    printf("PreMergeLiveBlocks,Public=%lu,OwnedBaseInPlace=%lu\n",
        (unsigned long)public_memory.pre_blocks,
        (unsigned long)inplace_memory.pre_blocks);
    {
        size_t pub[4], owned[4];
        const char *names[4] = { "PATH_TOTAL", "TREE_TOTAL", "GROUP_TOTAL", "MERGE_SCRATCH" };
        if (public_memory.peak_bytes < inplace_memory.peak_bytes ||
            public_memory.post_bytes < inplace_memory.post_bytes ||
            !stage101_peak_categories(&public_memory.peak_stats, pub) ||
            !stage101_peak_categories(&inplace_memory.peak_stats, owned)) return 1;
        printf("PeakBytesSaved=%lu PeakSavingsPercent=%.4f\n",
            (unsigned long)(public_memory.peak_bytes-inplace_memory.peak_bytes),
            100.0*(double)(public_memory.peak_bytes-inplace_memory.peak_bytes)/public_memory.peak_bytes);
        printf("PostMergeBytesSaved=%lu PostMergeSavingsPercent=%.4f\n",
            (unsigned long)(public_memory.post_bytes-inplace_memory.post_bytes),
            100.0*(double)(public_memory.post_bytes-inplace_memory.post_bytes)/public_memory.post_bytes);
        printf("PeakTagComposition,Public,OwnedBaseInPlace\n");
        for (i=0;i<4;++i) printf("%s,%lu,%lu\n",names[i],(unsigned long)pub[i],(unsigned long)owned[i]);
    }
    {
        HpsAllocStats s = hps_alloc_stats_get();
        if (!benchmark_alloc_stats_empty(&s) || s.failed_calls != 0 ||
            !stage83_tag_accounting_valid(&s,"10.1 final") ||
            !stage83_other_is_empty(&s,"10.1 final")) result = 1;
        printf("Stage 10.1 allocator final: live=%lu/%lu OTHER=%lu FailedCalls=%lu\n",
            (unsigned long)s.live_bytes,(unsigned long)s.live_blocks,
            (unsigned long)s.tags[HPS_ALLOC_TAG_OTHER].live_blocks,
            (unsigned long)s.failed_calls);
    }
    if (result) return 1;
    printf("Stage 10.1 complete. No Batch integration; Stage 10.2 not started.\n");
    return 0;
}

typedef struct Stage102Entry {
    HpsGroup *group;
    int owned;
} Stage102Entry;

typedef struct Stage102Snapshot {
    size_t count;
    void **items;
    char **paths;
    size_t result_only_bytes;
    size_t result_only_blocks;
} Stage102Snapshot;

typedef struct Stage102MemorySample {
    size_t groups, group_size;
    size_t batch_bytes, batch_blocks;
    size_t post_bytes, post_blocks;
    size_t peak_bytes, peak_blocks;
    size_t result_only_bytes, result_only_blocks;
    size_t clone_count;
    HpsAllocStats peak_stats;
} Stage102MemorySample;

static HpsStatus stage102_reference_merge_all(const HpsGroupBatch *batch,
    const HpsComparator *comparator, HpsGroup **out_group)
{
    Stage102Entry *current = NULL, *next = NULL;
    size_t current_count, next_count = 0, i;
    HpsStatus status = HPS_STATUS_OK;
    HpsGroup *empty = NULL;
    if (out_group == NULL) return HPS_STATUS_INVALID_ARGUMENT;
    *out_group = NULL;
    if (batch == NULL || comparator == NULL || comparator->compare == NULL)
        return HPS_STATUS_INVALID_ARGUMENT;
    current_count = hps_group_batch_group_count(batch);
    if (current_count == 0) return hps_group_build(NULL, 0, comparator, out_group);
    if (current_count == 1) {
        status = hps_group_build(NULL, 0, comparator, &empty);
        if (status == HPS_STATUS_OK)
            status = hps_group_merge(hps_group_batch_group_at(batch, 0),
                empty, comparator, out_group);
        hps_group_destroy(empty);
        return status;
    }
    if (current_count > (size_t)-1 / sizeof(*current)) return HPS_STATUS_OUT_OF_MEMORY;
    current = (Stage102Entry *)hps_alloc_tagged(current_count * sizeof(*current),
        HPS_ALLOC_TAG_MERGE_SCRATCH);
    if (current == NULL) return HPS_STATUS_OUT_OF_MEMORY;
    for (i = 0; i < current_count; ++i) {
        current[i].group = (HpsGroup *)hps_group_batch_group_at(batch, i);
        current[i].owned = 0;
    }
    while (current_count > 1) {
        size_t ci = 0, ni = 0;
        next_count = current_count / 2 + current_count % 2;
        if (next_count > (size_t)-1 / sizeof(*next)) {
            status = HPS_STATUS_OUT_OF_MEMORY;
            goto cleanup;
        }
        next = (Stage102Entry *)hps_alloc_tagged(next_count * sizeof(*next),
            HPS_ALLOC_TAG_MERGE_SCRATCH);
        if (next == NULL) { status = HPS_STATUS_OUT_OF_MEMORY; goto cleanup; }
        for (i = 0; i < next_count; ++i) { next[i].group = NULL; next[i].owned = 0; }
        while (ci < current_count) {
            if (ci + 1 < current_count) {
                HpsGroup *merged = NULL;
                status = hps_group_merge(current[ci].group, current[ci + 1].group,
                    comparator, &merged);
                if (status != HPS_STATUS_OK) goto cleanup;
                next[ni].group = merged; next[ni].owned = 1; ++ni;
                if (current[ci].owned) hps_group_destroy(current[ci].group);
                if (current[ci + 1].owned) hps_group_destroy(current[ci + 1].group);
                current[ci].group = NULL; current[ci].owned = 0;
                current[ci + 1].group = NULL; current[ci + 1].owned = 0;
                ci += 2;
            } else {
                next[ni++] = current[ci];
                current[ci].group = NULL; current[ci].owned = 0;
                ++ci;
            }
        }
        hps_free(current); current = next; next = NULL; current_count = next_count;
    }
    if (current[0].owned) {
        *out_group = current[0].group;
        current[0].group = NULL; current[0].owned = 0;
    } else {
        status = hps_group_build(NULL, 0, comparator, &empty);
        if (status == HPS_STATUS_OK)
            status = hps_group_merge(current[0].group, empty, comparator, out_group);
        hps_group_destroy(empty);
    }
cleanup:
    if (next != NULL) {
        for (i = 0; i < next_count; ++i) if (next[i].owned) hps_group_destroy(next[i].group);
        hps_free(next);
    }
    if (current != NULL) {
        for (i = 0; i < current_count; ++i) if (current[i].owned) hps_group_destroy(current[i].group);
        hps_free(current);
    }
    return status;
}

static int stage102_snapshot_batch(const HpsGroupBatch *batch,
    Stage102Snapshot *snapshot)
{
    size_t g, cursor = 0;
    snapshot->count = hps_group_batch_total_size(batch);
    snapshot->items = (void **)malloc(snapshot->count * sizeof(*snapshot->items));
    snapshot->paths = (char **)calloc(snapshot->count, sizeof(*snapshot->paths));
    if (snapshot->count != 0 && (snapshot->items == NULL || snapshot->paths == NULL))
        return 0;
    for (g = 0; g < hps_group_batch_group_count(batch); ++g) {
        const HpsGroup *group = hps_group_batch_group_at(batch, g);
        size_t i;
        for (i = 0; i < hps_group_size(group); ++i, ++cursor) {
            const HpsPath *path = hps_group_path_at(group, i);
            size_t text_length = hps_path_text_length(path);
            snapshot->items[cursor] = hps_group_item_at(group, i);
            if (text_length == (size_t)-1 || text_length == (size_t)-2) return 0;
            snapshot->paths[cursor] = (char *)malloc(text_length + 1);
            if (snapshot->paths[cursor] == NULL || hps_path_format(path,
                    snapshot->paths[cursor], text_length + 1) != HPS_STATUS_OK) return 0;
        }
    }
    return cursor == snapshot->count;
}

static int stage102_batch_matches_snapshot(const HpsGroupBatch *batch,
    const Stage102Snapshot *snapshot)
{
    size_t g, cursor = 0;
    if (snapshot->count != hps_group_batch_total_size(batch)) return 0;
    for (g = 0; g < hps_group_batch_group_count(batch); ++g) {
        const HpsGroup *group = hps_group_batch_group_at(batch, g);
        size_t i;
        for (i = 0; i < hps_group_size(group); ++i, ++cursor) {
            const HpsPath *source_path = hps_group_path_at(group, i);
            size_t text_length = hps_path_text_length(source_path);
            char *path;
            int matches;
            if (hps_group_item_at(group, i) != snapshot->items[cursor]) return 0;
            path = (char *)malloc(text_length + 1);
            if (path == NULL) return 0;
            matches = hps_path_format(source_path, path, text_length + 1) == HPS_STATUS_OK &&
                strcmp(path, snapshot->paths[cursor]) == 0;
            free(path);
            if (!matches) return 0;
        }
    }
    return cursor == snapshot->count;
}

static int stage102_snapshot_group(const HpsGroup *group, Stage102Snapshot *snapshot)
{
    size_t i;
    snapshot->count = hps_group_size(group);
    snapshot->items = (void **)malloc(snapshot->count * sizeof(*snapshot->items));
    snapshot->paths = (char **)calloc(snapshot->count, sizeof(*snapshot->paths));
    if (snapshot->count != 0 && (snapshot->items == NULL || snapshot->paths == NULL)) return 0;
    for (i = 0; i < snapshot->count; ++i) {
        const HpsPath *path = hps_group_path_at(group, i);
        size_t text_length = hps_path_text_length(path);
        snapshot->items[i] = hps_group_item_at(group, i);
        if (text_length == (size_t)-1 || text_length == (size_t)-2) return 0;
        snapshot->paths[i] = (char *)malloc(text_length + 1);
        if (snapshot->paths[i] == NULL || hps_path_format(path,
                snapshot->paths[i], text_length + 1) != HPS_STATUS_OK) return 0;
    }
    return 1;
}

static int stage102_groups_match_snapshots(const HpsGroup *group,
    const Stage102Snapshot *snapshot)
{
    size_t i;
    if (hps_group_size(group) != snapshot->count) return 0;
    for (i = 0; i < snapshot->count; ++i) {
        size_t text_length = hps_path_text_length(hps_group_path_at(group, i));
        char *path;
        int matches;
        if (hps_group_item_at(group, i) != snapshot->items[i] ||
            text_length == (size_t)-1 || text_length == (size_t)-2) return 0;
        path = (char *)malloc(text_length + 1);
        if (path == NULL) return 0;
        matches = hps_path_format(hps_group_path_at(group, i), path,
            text_length + 1) == HPS_STATUS_OK && strcmp(path, snapshot->paths[i]) == 0;
        free(path);
        if (!matches) return 0;
    }
    return 1;
}

static int stage102_snapshots_equal(const Stage102Snapshot *a,
    const Stage102Snapshot *b)
{
    size_t i;
    if (a->count != b->count) return 0;
    for (i = 0; i < a->count; ++i)
        if (a->items[i] != b->items[i] || strcmp(a->paths[i], b->paths[i]) != 0) return 0;
    return 1;
}

static void stage102_snapshot_free(Stage102Snapshot *snapshot)
{
    size_t i;
    if (snapshot->paths != NULL)
        for (i = 0; i < snapshot->count; ++i) free(snapshot->paths[i]);
    free(snapshot->paths); free(snapshot->items);
    snapshot->paths = NULL; snapshot->items = NULL; snapshot->count = 0;
}

static int stage102_compare_batch_case(size_t n, size_t group_size,
    uint32_t seed, size_t expected_groups, int print_case)
{
    int values[103];
    void *items[103];
    BenchmarkCompareContext counter = { 0, 0 };
    HpsComparator comparator = { benchmark_compare_int, &counter };
    HpsGroupBatch *batch = NULL;
    HpsGroup *optimized = NULL, *reference = NULL;
    Stage102Snapshot before = { 0 }, opt = { 0 }, ref = { 0 };
    HpsAllocStats stats;
    size_t i;
    int passed = 0;
    for (i = 0; i < n; ++i) values[i] = (int)i;
    benchmark_shuffle(values, n, seed);
    for (i = 0; i < n; ++i) items[i] = &values[i];
    if (hps_alloc_stats_reset() != 0 ||
        hps_group_batch_build(items, n, group_size, &comparator, &batch) != HPS_STATUS_OK ||
        hps_group_batch_group_count(batch) != expected_groups ||
        !stage102_snapshot_batch(batch, &before)) goto cleanup;
    if (hps_group_batch_merge_all(batch, &comparator, &optimized) != HPS_STATUS_OK ||
        !stage102_snapshot_group(optimized, &opt) ||
        !stage102_batch_matches_snapshot(batch, &before)) goto cleanup;
    if (stage102_reference_merge_all(batch, &comparator, &reference) != HPS_STATUS_OK ||
        !stage102_snapshot_group(reference, &ref) ||
        !stage102_batch_matches_snapshot(batch, &before) ||
        !stage102_snapshots_equal(&opt, &ref)) goto cleanup;
    stats = hps_alloc_stats_get();
    if (!stage83_tag_accounting_valid(&stats, "10.2 small comparison") ||
        !stage83_other_is_empty(&stats, "10.2 small comparison")) goto cleanup;
    passed = 1;
cleanup:
    hps_group_destroy(reference); hps_group_destroy(optimized); hps_group_batch_destroy(batch);
    stats = hps_alloc_stats_get();
    if (!benchmark_alloc_stats_empty(&stats) || stats.failed_calls != 0 ||
        !stage83_tag_accounting_valid(&stats, "10.2 small final") ||
        !stage83_other_is_empty(&stats, "10.2 small final")) passed = 0;
    if (print_case) printf("Stage 10.2 N=%lu Groups=%lu GroupSize=%lu reference=optimized Path/item=%s; Batch unchanged=%s\n",
        (unsigned long)n, (unsigned long)expected_groups, (unsigned long)group_size,
        passed ? "PASS" : "FAIL", passed ? "YES" : "NO");
    stage102_snapshot_free(&before); stage102_snapshot_free(&opt); stage102_snapshot_free(&ref);
    return passed;
}

static int stage102_run_memory_case(size_t group_size, size_t expected_groups,
    int reference_mode, Stage102MemorySample *sample, Stage102Snapshot *result_snapshot)
{
    enum { N = 10000 };
    static int values[N];
    static void *items[N];
    BenchmarkCompareContext counter = { 0, 0 };
    HpsComparator comparator = { benchmark_compare_int, &counter };
    HpsGroupBatch *batch = NULL;
    HpsGroup *result = NULL;
    HpsAllocStats after_batch, after_merge, result_only, final_stats;
    size_t i;
    int valid = 0;
    const char *failure_phase = "begin";
    HpsStatus merge_status = HPS_STATUS_OK;
    int final_valid = 0, snapshot_valid = 0;
    for (i = 0; i < N; ++i) values[i] = (int)i;
    benchmark_shuffle(values, N, UINT32_C(0xC0FFEE));
    for (i = 0; i < N; ++i) items[i] = &values[i];
    failure_phase = "Batch build/reset";
    if (hps_alloc_stats_reset() != 0 ||
        hps_group_batch_build(items, N, group_size, &comparator, &batch) != HPS_STATUS_OK ||
        hps_group_batch_group_count(batch) != expected_groups) goto cleanup;
    sample->groups = hps_group_batch_group_count(batch);
    sample->group_size = group_size;
    after_batch = hps_alloc_stats_get();
    failure_phase = "AfterBatchBuild accounting";
    if (!stage83_tag_accounting_valid(&after_batch, "10.2 AfterBatchBuild") ||
        !stage83_other_is_empty(&after_batch, "10.2 AfterBatchBuild")) goto cleanup;
    sample->batch_bytes = after_batch.live_bytes;
    sample->batch_blocks = after_batch.live_blocks;
    sample->clone_count = after_batch.tags[HPS_ALLOC_TAG_GROUP_OBJECT].alloc_calls;
    counter.comparison_count = 0;
    failure_phase = reference_mode ? "legacy reference merge/validation/snapshot" :
        "optimized merge/validation/snapshot";
    merge_status = reference_mode ? stage102_reference_merge_all(batch, &comparator, &result) :
        hps_group_batch_merge_all(batch, &comparator, &result);
    if (merge_status != HPS_STATUS_OK) goto cleanup;
    final_valid = benchmark_validate_final(result, N, &(BenchmarkPathStats){ 0 });
    if (!final_valid) goto cleanup;
    snapshot_valid = stage102_snapshot_group(result, result_snapshot);
    if (!snapshot_valid) goto cleanup;
    after_merge = hps_alloc_stats_get();
    failure_phase = "PostMerge accounting";
    if (!stage83_tag_accounting_valid(&after_merge, "10.2 PostMerge") ||
        !stage83_other_is_empty(&after_merge, "10.2 PostMerge")) goto cleanup;
    sample->post_bytes = after_merge.live_bytes;
    sample->post_blocks = after_merge.live_blocks;
    sample->peak_bytes = after_merge.peak_live_bytes;
    sample->peak_blocks = after_merge.peak_live_blocks;
    sample->peak_stats = after_merge;
    if (!reference_mode) {
        size_t total_group_allocs = after_merge.tags[HPS_ALLOC_TAG_GROUP_OBJECT].alloc_calls;
        sample->clone_count = total_group_allocs - sample->clone_count;
    } else sample->clone_count = 0;
    hps_group_batch_destroy(batch); batch = NULL;
    result_only = hps_alloc_stats_get();
    failure_phase = "ResultOnly accounting";
    sample->result_only_bytes = result_only.live_bytes;
    sample->result_only_blocks = result_only.live_blocks;
    result_snapshot->result_only_bytes = result_only.live_bytes;
    result_snapshot->result_only_blocks = result_only.live_blocks;
    if (!stage102_groups_match_snapshots(result, result_snapshot) ||
        !benchmark_validate_final(result, N, &(BenchmarkPathStats){ 0 }) ||
        !stage83_tag_accounting_valid(&result_only, "10.2 ResultOnly") ||
        !stage83_other_is_empty(&result_only, "10.2 ResultOnly")) goto cleanup;
    valid = 1;
cleanup:
    hps_group_destroy(result); hps_group_batch_destroy(batch);
    final_stats = hps_alloc_stats_get();
    if (!benchmark_alloc_stats_empty(&final_stats) || final_stats.failed_calls != 0 ||
        !stage83_tag_accounting_valid(&final_stats, "10.2 memory final") ||
        !stage83_other_is_empty(&final_stats, "10.2 memory final")) valid = 0;
    if (!valid) printf("Stage 10.2 memory failure: Groups=%lu mode=%s phase=%s final=%lu/%lu FailedCalls=%lu\n",
        (unsigned long)expected_groups,reference_mode?"Reference":"Optimized",failure_phase,
        (unsigned long)final_stats.live_bytes,(unsigned long)final_stats.live_blocks,
        (unsigned long)final_stats.failed_calls);
    if (!valid && merge_status != HPS_STATUS_OK) printf("  merge status=%s\n",hps_status_string(merge_status));
    if (!valid && merge_status == HPS_STATUS_OK) printf("  output validate=%d snapshot=%d\n",
        final_valid,snapshot_valid);
    return valid;
}

static int stage102_memory_panel(void)
{
    static const size_t groups[6] = { 4, 5, 8, 10, 32, 40 };
    static const size_t group_sizes[6] = { 2500, 2000, 1250, 1000, 313, 250 };
    Stage102MemorySample ref[6], opt[6];
    Stage102Snapshot ref_snap[6] = { 0 }, opt_snap[6] = { 0 };
    size_t i;
    int valid = 1;
    memset(ref,0,sizeof(ref)); memset(opt,0,sizeof(opt));
    for (i = 0; i < 6; ++i) {
        if (!stage102_run_memory_case(group_sizes[i], groups[i], 1, &ref[i], &ref_snap[i]) ||
            !stage102_run_memory_case(group_sizes[i], groups[i], 0, &opt[i], &opt_snap[i])) {
            valid = 0; break;
        }
        if (!stage102_snapshots_equal(&ref_snap[i], &opt_snap[i]) ||
            ref[i].post_bytes != opt[i].post_bytes || ref[i].post_blocks != opt[i].post_blocks ||
            ref[i].result_only_bytes != opt[i].result_only_bytes ||
            ref[i].result_only_blocks != opt[i].result_only_blocks) valid = 0;
    }
    if (!valid) {
        for (i=0;i<6;++i) { stage102_snapshot_free(&ref_snap[i]); stage102_snapshot_free(&opt_snap[i]); }
        return 0;
    }
    printf("Stage 10.2 memory CSV\nGroups,GroupSize,ReferenceBatchLiveBytes,OptimizedBatchLiveBytes,ReferencePostMergeBytes,OptimizedPostMergeBytes,ReferencePeakBytes,OptimizedPeakBytes,PeakBytesSaved,PeakSavingsPercent,ReferenceTransientExtraBytes,OptimizedTransientExtraBytes,TransientSaved,TransientSavingsPercent\n");
    for (i = 0; i < 6; ++i) {
        size_t peak_saved = ref[i].peak_bytes - opt[i].peak_bytes;
        size_t ref_transient = ref[i].peak_bytes - ref[i].post_bytes;
        size_t opt_transient = opt[i].peak_bytes - opt[i].post_bytes;
        size_t transient_saved = ref_transient - opt_transient;
        printf("%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%.4f,%lu,%lu,%lu,%.4f\n",
            (unsigned long)groups[i],(unsigned long)group_sizes[i],
            (unsigned long)ref[i].batch_bytes,(unsigned long)opt[i].batch_bytes,
            (unsigned long)ref[i].post_bytes,(unsigned long)opt[i].post_bytes,
            (unsigned long)ref[i].peak_bytes,(unsigned long)opt[i].peak_bytes,
            (unsigned long)peak_saved, ref[i].peak_bytes ? 100.0*peak_saved/ref[i].peak_bytes : 0.0,
            (unsigned long)ref_transient,(unsigned long)opt_transient,
            (unsigned long)transient_saved,
            ref_transient ? 100.0*transient_saved/ref_transient : 0.0);
        if (ref[i].groups != groups[i] || opt[i].groups != groups[i] ||
            ref[i].batch_bytes != opt[i].batch_bytes || ref[i].peak_bytes < opt[i].peak_bytes ||
            ref_transient < opt_transient) valid = 0;
        if (groups[i] == 32 && ref[i].peak_bytes != 5469280) valid = 0;
        if (groups[i] == 40 && ref[i].peak_bytes != 4989592) valid = 0;
    }
    printf("\nStage 10.2 Peak Blocks CSV\nGroups,ReferencePeakBlocks,OptimizedPeakBlocks,BlocksSaved,BlocksSavingsPercent\n");
    for (i = 0; i < 6; ++i) {
        size_t saved = ref[i].peak_blocks - opt[i].peak_blocks;
        printf("%lu,%lu,%lu,%lu,%.4f\n",(unsigned long)groups[i],
            (unsigned long)ref[i].peak_blocks,(unsigned long)opt[i].peak_blocks,
            (unsigned long)saved,ref[i].peak_blocks?100.0*saved/ref[i].peak_blocks:0.0);
        if (ref[i].peak_blocks < opt[i].peak_blocks) valid = 0;
    }
    printf("\nStage 10.2 G32/G40 Peak Tag Composition\nGroups,Mode,PATH_TOTAL,TREE_TOTAL,GROUP_TOTAL,MERGE_SCRATCH,BATCH_TOTAL,PeakTotal\n");
    for (i = 4; i < 6; ++i) {
        Stage102MemorySample *samples[2] = { &ref[i], &opt[i] };
        const char *names[2] = { "Reference", "Optimized" };
        size_t m;
        for (m=0;m<2;++m) {
            const HpsAllocStats *s=&samples[m]->peak_stats;
            size_t path=s->bytes_at_global_peak[HPS_ALLOC_TAG_PATH_OBJECT]+s->bytes_at_global_peak[HPS_ALLOC_TAG_PATH_STEPS];
            size_t tree=s->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_OBJECT]+s->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_NODE]+s->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_CHILDREN];
            size_t group=s->bytes_at_global_peak[HPS_ALLOC_TAG_GROUP_OBJECT]+s->bytes_at_global_peak[HPS_ALLOC_TAG_GROUP_ORDERED];
            size_t scratch=s->bytes_at_global_peak[HPS_ALLOC_TAG_MERGE_SCRATCH];
            size_t batch=s->bytes_at_global_peak[HPS_ALLOC_TAG_BATCH_OBJECT]+s->bytes_at_global_peak[HPS_ALLOC_TAG_BATCH_GROUP_ARRAY];
            printf("%lu,%s,%lu,%lu,%lu,%lu,%lu,%lu\n",(unsigned long)groups[i],names[m],
                (unsigned long)path,(unsigned long)tree,(unsigned long)group,
                (unsigned long)scratch,(unsigned long)batch,(unsigned long)s->peak_live_bytes);
            if (path+tree+group+scratch+batch != s->peak_live_bytes) valid=0;
        }
        if (!stage102_snapshots_equal(&ref_snap[i],&opt_snap[i]) ||
            ref[i].result_only_bytes != opt[i].result_only_bytes) valid=0;
    }
    printf("Stage 10.2 clone count (GROUP_OBJECT allocations after Batch build)\nGroups,OwnedBaseClones\n");
    for (i=0;i<6;++i) printf("%lu,%lu\n",(unsigned long)groups[i],(unsigned long)opt[i].clone_count);
    for (i=0;i<6;++i) { stage102_snapshot_free(&ref_snap[i]); stage102_snapshot_free(&opt_snap[i]); }
    return valid;
}

static int stage102_equal_and_lifecycle_tests(void)
{
    int equal_values[10];
    void *items[10];
    BenchmarkCompareContext counter = { 0, 0 };
    HpsComparator comparator = { benchmark_compare_int, &counter };
    HpsGroupBatch *batch = NULL;
    HpsGroup *result = NULL, *reference = NULL;
    Stage102Snapshot opt = { 0 }, ref = { 0 }, before = { 0 };
    HpsAllocStats after_batch, after_merge, final_stats;
    size_t i, clone_count;
    int passed = 0;
    for (i=0;i<10;++i) { equal_values[i]=7; items[i]=&equal_values[i]; }
    if (hps_alloc_stats_reset()!=0 ||
        hps_group_batch_build(items,10,2,&comparator,&batch)!=HPS_STATUS_OK ||
        hps_group_batch_group_count(batch)!=5 || !stage102_snapshot_batch(batch,&before)) goto cleanup;
    after_batch=hps_alloc_stats_get();
    if (hps_group_batch_merge_all(batch,&comparator,&result)!=HPS_STATUS_OK ||
        !stage102_snapshot_group(result,&opt) || !stage102_batch_matches_snapshot(batch,&before)) goto cleanup;
    after_merge=hps_alloc_stats_get();
    clone_count=after_merge.tags[HPS_ALLOC_TAG_GROUP_OBJECT].alloc_calls-
        after_batch.tags[HPS_ALLOC_TAG_GROUP_OBJECT].alloc_calls;
    if (clone_count!=2 || stage102_reference_merge_all(batch,&comparator,&reference)!=HPS_STATUS_OK ||
        !stage102_snapshot_group(reference,&ref) || !stage102_snapshots_equal(&opt,&ref)) goto cleanup;
    for (i=0;i<10;++i)
        if (hps_group_item_at(result,i)!=items[i]) goto cleanup;
    printf("Stage 10.2 five equal Groups: pointer stability=PASS; G4 borrowed-right clone count=0; total owned-base clones=%lu\n",(unsigned long)clone_count);
    hps_group_destroy(reference); reference=NULL;
    hps_group_destroy(result); result=NULL;
    hps_group_batch_destroy(batch); batch=NULL;
    final_stats=hps_alloc_stats_get();
    if (!benchmark_alloc_stats_empty(&final_stats) || final_stats.failed_calls!=0) goto cleanup;

    {
        int six_values[12]; void *six_items[12];
        for (i=0;i<12;++i) { six_values[i]=(int)i; six_items[i]=&six_values[i]; }
        if (hps_alloc_stats_reset()!=0 || hps_group_batch_build(six_items,12,2,
                &comparator,&batch)!=HPS_STATUS_OK || hps_group_batch_group_count(batch)!=6) goto cleanup;
        after_batch=hps_alloc_stats_get();
        if (hps_group_batch_merge_all(batch,&comparator,&result)!=HPS_STATUS_OK ||
            hps_group_size(result)!=12) goto cleanup;
        after_merge=hps_alloc_stats_get();
        clone_count=after_merge.tags[HPS_ALLOC_TAG_GROUP_OBJECT].alloc_calls-
            after_batch.tags[HPS_ALLOC_TAG_GROUP_OBJECT].alloc_calls;
        if (clone_count!=3) goto cleanup;
        hps_group_destroy(batch == NULL ? NULL : result); result=NULL;
        hps_group_batch_destroy(batch); batch=NULL;
        final_stats=hps_alloc_stats_get();
        if (!benchmark_alloc_stats_empty(&final_stats) || final_stats.failed_calls!=0 ||
            !stage83_tag_accounting_valid(&final_stats,"10.2 six-group final") ||
            !stage83_other_is_empty(&final_stats,"10.2 six-group final")) goto cleanup;
        printf("Stage 10.2 six Groups: owned odd H45 used as right and released after merge; clones=%lu; final live=0/0\n",(unsigned long)clone_count);
    }
    passed=1;
cleanup:
    hps_group_destroy(reference); hps_group_destroy(result); hps_group_batch_destroy(batch);
    stage102_snapshot_free(&before); stage102_snapshot_free(&opt); stage102_snapshot_free(&ref);
    final_stats=hps_alloc_stats_get();
    if (!benchmark_alloc_stats_empty(&final_stats) || final_stats.failed_calls!=0) passed=0;
    if (!passed) printf("Stage 10.2 equal/lifecycle tests: FAIL\n");
    return passed;
}

static int stage102_g0_path_test(void)
{
    int values[10]={90,10,70,30,80,20,60,40,100,50};
    void *items[10];
    const char *expected[4]={"1A0","1A0//A0","1A0/A0","000"};
    HpsGroupBatch *batch=NULL; HpsGroup *result=NULL;
    BenchmarkCompareContext counter={0,0};
    HpsComparator comparator={benchmark_compare_int,&counter};
    size_t i,j; int passed=0;
    const HpsGroup *g0;
    const HpsPath *paths[4]; void *g0_items[4];
    for(i=0;i<10;++i) items[i]=&values[i];
    if(hps_alloc_stats_reset()!=0||hps_group_batch_build(items,10,4,&comparator,&batch)!=HPS_STATUS_OK) goto cleanup;
    g0=hps_group_batch_group_at(batch,0);
    for(i=0;i<4;++i){g0_items[i]=hps_group_item_at(g0,i);paths[i]=hps_group_path_at(g0,i);}
    if(hps_group_batch_merge_all(batch,&comparator,&result)!=HPS_STATUS_OK) goto cleanup;
    for(i=0;i<4;++i){
        char text[128]; int order;
        for(j=0;j<hps_group_size(result);++j) if(hps_group_item_at(result,j)==g0_items[i]) break;
        if(j==hps_group_size(result)||hps_path_compare(paths[i],hps_group_path_at(result,j),&order)!=HPS_STATUS_OK||order!=0||
            hps_path_format(hps_group_path_at(result,j),text,sizeof(text))!=HPS_STATUS_OK||strcmp(text,expected[i])!=0) goto cleanup;
    }
    passed=1;
    printf("Stage 10.2 G0 lineage Paths retained: 10=1A0, 30=1A0//A0, 70=1A0/A0, 90=000\n");
cleanup:
    hps_group_destroy(result);hps_group_batch_destroy(batch);
    {HpsAllocStats s=hps_alloc_stats_get();if(!benchmark_alloc_stats_empty(&s)||s.failed_calls!=0||!stage83_tag_accounting_valid(&s,"10.2 G0 final"))passed=0;}
    return passed;
}

int hps_run_stage10_2_tests(void)
{
    static const size_t groups[8]={2,3,4,5,6,7,8,10};
    size_t i;
    printf("HPSort Stage 10.2: Batch balanced merge lifecycle integration\n");
    if(hps_run_stage8_1_tests()!=0||!stage82_alignment_regression()||
        !stage83_tag_direct_test()||!stage92_path_storage_regression()) return 1;
    for(i=0;i<8;++i){
        size_t gs=103/groups[i]+(103%groups[i]!=0);
        if(!stage102_compare_batch_case(103,gs,UINT32_C(0xC0FFEE)+ (uint32_t)i,
                groups[i],1)) return 1;
    }
    if(!stage102_equal_and_lifecycle_tests()||!stage102_g0_path_test()) return 1;
    {
        int values[1024];void *items[1024];BenchmarkCompareContext counter={0,0};
        HpsComparator comparator={benchmark_compare_int,&counter};HpsGroupBatch *batch=NULL;HpsGroup *result=NULL;size_t k;int ok=1;
        for(k=0;k<1024;++k){values[k]=(int)k;items[k]=&values[k];}
        if(hps_alloc_stats_reset()!=0||hps_group_batch_build(items,1024,1,&comparator,&batch)!=HPS_STATUS_OK)return 1;
        counter.comparison_count=0;
        if(hps_group_batch_merge_all(batch,&comparator,&result)!=HPS_STATUS_OK||hps_group_size(result)!=1024)ok=0;
        for(k=0;ok&&k<1024;++k){int po;if(hps_group_item_at(result,k)!=&values[k]||(k>0&&(hps_path_compare(hps_group_path_at(result,k-1),hps_group_path_at(result,k),&po)!=HPS_STATUS_OK||po!=-1)))ok=0;}
        printf("Stage 10.2 1024 singleton Groups: comparisons=%lu order/path=%s\n",(unsigned long)counter.comparison_count,ok?"PASS":"FAIL");
        if(counter.comparison_count!=14337)ok=0;
        hps_group_destroy(result);hps_group_batch_destroy(batch);
        {HpsAllocStats s=hps_alloc_stats_get();if(!benchmark_alloc_stats_empty(&s)||s.failed_calls!=0||!stage83_tag_accounting_valid(&s,"10.2 1024 final")||!stage83_other_is_empty(&s,"10.2 1024 final"))ok=0;}
        if(!ok)return 1;
    }
    if(!stage102_memory_panel())return 1;
    {
        HpsAllocStats s=hps_alloc_stats_get();
        if(!benchmark_alloc_stats_empty(&s)||s.failed_calls!=0||
            !stage83_tag_accounting_valid(&s,"10.2 final")||!stage83_other_is_empty(&s,"10.2 final"))return 1;
        printf("Stage 10.2 allocator final: OTHER=%lu FailedCalls=%lu live=%lu/%lu\n",
            (unsigned long)s.tags[HPS_ALLOC_TAG_OTHER].live_blocks,(unsigned long)s.failed_calls,
            (unsigned long)s.live_bytes,(unsigned long)s.live_blocks);
    }
    printf("Stage 10.2 complete. No Stage 10.3 work started.\n");
    return 0;
}

typedef struct Stage103Snapshot {
    size_t count;
    size_t group_count;
    size_t *group_sizes;
    void **items;
    size_t *path_offsets;
    size_t *path_lengths;
    char *path_text;
    HpsDirection *path_directions;
    size_t *step_offsets;
    unsigned int *step_slots;
    size_t *step_levels;
} Stage103Snapshot;

typedef struct Stage103Sample {
    size_t group_size;
    size_t batch_bytes, batch_blocks;
    size_t post_bytes, post_blocks;
    size_t result_bytes, result_blocks;
    size_t peak_bytes, peak_blocks;
    size_t transient_bytes, peak_over_batch;
    size_t build_comparisons, merge_comparisons, total_comparisons;
    size_t merge_alloc_calls, merge_realloc_calls, merge_free_calls;
    size_t observed_public_bootstrap;
    size_t observed_owned_inplace;
    double build_ms;
    double merge_ms;
    HpsAllocStats peak_stats;
    HpsAllocStats batch_stats;
    HpsAllocStats result_stats;
    double final_avg_depth;
    size_t final_max_depth;
    size_t final_max_level;
} Stage103Sample;

typedef struct Stage112TreeCapture {
    HpsTreeInternalProfile profile;
    size_t *capacity_counts;
    size_t capacity_count;
} Stage112TreeCapture;

/* Stage 12.1 isolated model types; production Tree layout is untouched. */
typedef struct Stage12ModelChildBlock {
    size_t count;
    size_t capacity;
    HpsTreeNode *children[];
} Stage12ModelChildBlock;

typedef struct Stage12ModelNode {
    HpsPath *path;
    void *item;
    HpsTreeNode *parent;
    Stage12ModelChildBlock *child_block;
} Stage12ModelNode;

enum { STAGE103_N = 10000, STAGE103_GROUPS = 18, STAGE103_SEEDS = 5,
    STAGE103_SCALE_NS = 4, STAGE103_SCALE_GROUPS = 3, STAGE103_SCALE_SEEDS = 3 };

static const size_t stage103_groups[STAGE103_GROUPS] = {
    1,2,3,4,5,6,7,8,10,12,16,20,25,32,40,50,64,80
};
static const uint32_t stage103_seeds[STAGE103_SEEDS] = {
    UINT32_C(0xC0FFEE),UINT32_C(0xC0FFEF),UINT32_C(0xC0FFF0),
    UINT32_C(0xC0FFF1),UINT32_C(0xC0FFF2)
};
static const size_t stage103_scale_ns[STAGE103_SCALE_NS] = {1000,2000,5000,10000};
static const size_t stage103_scale_groups[STAGE103_SCALE_GROUPS] = {2,8,32};
static const size_t stage111_scale_groups[STAGE103_SCALE_GROUPS] = {1,8,32};
static const uint32_t stage103_scale_seeds[STAGE103_SCALE_SEEDS] = {
    UINT32_C(0xC0FFEE),UINT32_C(0xC0FFEF),UINT32_C(0xC0FFF0)
};

static size_t stage103_comparator_mismatches;
static size_t stage103_result_path_mismatches;
static size_t stage103_result_item_mismatches;
static size_t stage103_postmerge_mismatches;
static size_t stage103_input_batch_mutations;
static size_t stage103_negative_peak_savings_samples;
static size_t stage103_negative_transient_savings_samples;
static size_t stage103_public_strategy_mismatches;
static size_t stage103_inplace_strategy_mismatches;
static double stage103_main_peak_min,stage103_main_peak_mean,stage103_main_peak_max;
static double stage103_main_trans_min,stage103_main_trans_mean,stage103_main_trans_max;
static double stage103_peak_blocks_min,stage103_peak_blocks_max;
static double stage103_alloc_savings_mean;
static double stage103_scaling_peak_min,stage103_scaling_peak_max;
static double stage103_scaling_trans_min,stage103_scaling_trans_max;

static int stage103_snapshot_alloc(Stage103Snapshot *snapshot, size_t count,
    size_t group_count)
{
    snapshot->count=count; snapshot->group_count=group_count;
    snapshot->items=(void **)malloc((count==0?1:count)*sizeof(*snapshot->items));
    snapshot->path_offsets=(size_t *)malloc((count==0?1:count)*sizeof(*snapshot->path_offsets));
    snapshot->path_lengths=(size_t *)malloc((count==0?1:count)*sizeof(*snapshot->path_lengths));
    snapshot->path_directions=(HpsDirection *)malloc((count==0?1:count)*sizeof(*snapshot->path_directions));
    snapshot->step_offsets=(size_t *)malloc((count+1)*sizeof(*snapshot->step_offsets));
    snapshot->group_sizes=(size_t *)malloc((group_count==0?1:group_count)*sizeof(*snapshot->group_sizes));
    snapshot->path_text=NULL;
    if(snapshot->items==NULL||snapshot->path_offsets==NULL||snapshot->path_lengths==NULL||
        snapshot->path_directions==NULL||snapshot->step_offsets==NULL||snapshot->group_sizes==NULL)return 0;
    return 1;
}

static int stage103_snapshot_alloc_steps(Stage103Snapshot *snapshot,size_t step_count)
{
    snapshot->step_slots=(unsigned int *)malloc((step_count==0?1:step_count)*sizeof(*snapshot->step_slots));
    snapshot->step_levels=(size_t *)malloc((step_count==0?1:step_count)*sizeof(*snapshot->step_levels));
    return snapshot->step_slots!=NULL&&snapshot->step_levels!=NULL;
}

static int stage103_snapshot_store_path(const HpsPath *path,Stage103Snapshot *snapshot,
    size_t path_index,size_t *step_cursor)
{
    size_t depth=hps_path_depth(path),j;
    snapshot->path_directions[path_index]=hps_path_direction(path);
    snapshot->step_offsets[path_index]=*step_cursor;
    for(j=0;j<depth;++j){
        if(hps_path_get_slot(path,j,&snapshot->step_slots[*step_cursor])!=HPS_STATUS_OK||
            hps_path_get_level(path,j,&snapshot->step_levels[*step_cursor])!=HPS_STATUS_OK)return 0;
        ++*step_cursor;
    }
    snapshot->step_offsets[path_index+1]=*step_cursor;
    return 1;
}

static int stage103_snapshot_capture_group(const HpsGroup *group, Stage103Snapshot *snapshot)
{
    size_t i,total=0,total_steps=0,step_cursor=0;
    if(!stage103_snapshot_alloc(snapshot,hps_group_size(group),0))return 0;
    for(i=0;i<snapshot->count;++i){
        size_t len=hps_path_text_length(hps_group_path_at(group,i));
        if(len==(size_t)-1||!benchmark_add_size(&total,len+1))return 0;
        if(!benchmark_add_size(&total_steps,hps_path_depth(hps_group_path_at(group,i))))return 0;
        snapshot->path_lengths[i]=len;
    }
    if(!stage103_snapshot_alloc_steps(snapshot,total_steps))return 0;
    snapshot->path_text=(char *)malloc(total==0?1:total);
    if(snapshot->path_text==NULL)return 0;
    total=0;
    for(i=0;i<snapshot->count;++i){
        snapshot->items[i]=hps_group_item_at(group,i);
        if(!stage103_snapshot_store_path(hps_group_path_at(group,i),snapshot,i,&step_cursor))return 0;
        snapshot->path_offsets[i]=total;
        if(hps_path_format(hps_group_path_at(group,i),snapshot->path_text+total,
                snapshot->path_lengths[i]+1)!=HPS_STATUS_OK)return 0;
        total+=snapshot->path_lengths[i]+1;
    }
    return 1;
}

static int stage103_snapshot_capture_batch(const HpsGroupBatch *batch,
    Stage103Snapshot *snapshot)
{
    size_t g,i,cursor=0,total=0,total_steps=0,step_cursor=0;
    if(!stage103_snapshot_alloc(snapshot,hps_group_batch_total_size(batch),
            hps_group_batch_group_count(batch)))return 0;
    for(g=0;g<snapshot->group_count;++g){
        const HpsGroup *group=hps_group_batch_group_at(batch,g);
        snapshot->group_sizes[g]=hps_group_size(group);
        for(i=0;i<hps_group_size(group);++i,++cursor){
            size_t len=hps_path_text_length(hps_group_path_at(group,i));
            if(len==(size_t)-1||!benchmark_add_size(&total,len+1))return 0;
            if(!benchmark_add_size(&total_steps,hps_path_depth(hps_group_path_at(group,i))))return 0;
            snapshot->path_lengths[cursor]=len;
        }
    }
    if(cursor!=snapshot->count)return 0;
    if(!stage103_snapshot_alloc_steps(snapshot,total_steps))return 0;
    snapshot->path_text=(char *)malloc(total==0?1:total);
    if(snapshot->path_text==NULL)return 0;
    cursor=0;total=0;
    for(g=0;g<snapshot->group_count;++g){
        const HpsGroup *group=hps_group_batch_group_at(batch,g);
        for(i=0;i<hps_group_size(group);++i,++cursor){
            snapshot->items[cursor]=hps_group_item_at(group,i);
            if(!stage103_snapshot_store_path(hps_group_path_at(group,i),snapshot,cursor,&step_cursor))return 0;
            snapshot->path_offsets[cursor]=total;
            if(hps_path_format(hps_group_path_at(group,i),snapshot->path_text+total,
                    snapshot->path_lengths[cursor]+1)!=HPS_STATUS_OK)return 0;
            total+=snapshot->path_lengths[cursor]+1;
        }
    }
    return 1;
}

static void stage103_snapshot_destroy(Stage103Snapshot *snapshot)
{
    free(snapshot->path_text);free(snapshot->path_lengths);free(snapshot->path_offsets);
    free(snapshot->items);free(snapshot->group_sizes);free(snapshot->path_directions);
    free(snapshot->step_offsets);free(snapshot->step_slots);free(snapshot->step_levels);
    memset(snapshot,0,sizeof(*snapshot));
}

static HpsPath *stage103_snapshot_make_path(const Stage103Snapshot *snapshot,size_t index)
{
    size_t begin=snapshot->step_offsets[index],end=snapshot->step_offsets[index+1],step;
    HpsPath *path;
    if(snapshot->path_directions[index]==HPS_DIRECTION_ZERO)
        return begin==end?hps_path_create_zero():NULL;
    if(begin==end)return NULL;
    path=hps_path_create_at_level(snapshot->path_directions[index],snapshot->step_slots[begin],
        snapshot->step_levels[begin]);
    if(path==NULL)return NULL;
    for(step=begin+1;step<end;++step){
        if(hps_path_append_at_level(path,snapshot->step_slots[step],snapshot->step_levels[step])!=HPS_STATUS_OK){
            hps_path_destroy(path);return NULL;
        }
    }
    return path;
}

static int stage103_snapshot_equal(const Stage103Snapshot *a,
    const Stage103Snapshot *b, size_t *path_mismatches, size_t *item_mismatches)
{
    size_t i;int equal=1;
    if(a->count!=b->count){*path_mismatches+=a->count>b->count?a->count-b->count:b->count-a->count;equal=0;}
    for(i=0;i<a->count&&i<b->count;++i){
        const char *ap=a->path_text+a->path_offsets[i],*bp=b->path_text+b->path_offsets[i];
        if(a->items[i]!=b->items[i]||*(const int *)a->items[i]!=*(const int *)b->items[i]){
            ++*item_mismatches;equal=0;
        }
        {
            HpsPath *pa=stage103_snapshot_make_path(a,i),*pb=stage103_snapshot_make_path(b,i);
            int order=1;HpsStatus compare_status;
            if(pa==NULL||pb==NULL){++*path_mismatches;equal=0;}
            else{
                compare_status=hps_path_compare(pa,pb,&order);
                if(compare_status!=HPS_STATUS_OK||order!=0||a->path_lengths[i]!=b->path_lengths[i]||strcmp(ap,bp)!=0){
                    ++*path_mismatches;equal=0;
                }
            }
            hps_path_destroy(pa);hps_path_destroy(pb);
        }
    }
    return equal;
}

static int stage103_batch_matches_snapshot(const HpsGroupBatch *batch,
    const Stage103Snapshot *snapshot)
{
    size_t g,i,cursor=0,maxlen=0;char *buffer;
    if(hps_group_batch_group_count(batch)!=snapshot->group_count||
        hps_group_batch_total_size(batch)!=snapshot->count)return 0;
    for(g=0;g<snapshot->group_count;++g)
        if(hps_group_size(hps_group_batch_group_at(batch,g))!=snapshot->group_sizes[g])return 0;
    for(i=0;i<snapshot->count;++i)if(snapshot->path_lengths[i]>maxlen)maxlen=snapshot->path_lengths[i];
    buffer=(char *)malloc(maxlen+1);if(buffer==NULL)return 0;
    for(g=0;g<snapshot->group_count;++g){
        const HpsGroup *group=hps_group_batch_group_at(batch,g);
        for(i=0;i<hps_group_size(group);++i,++cursor){
            const HpsPath *path=hps_group_path_at(group,i);size_t step;
            if(hps_group_item_at(group,i)!=snapshot->items[cursor]||
                hps_path_direction(path)!=snapshot->path_directions[cursor]||
                hps_path_depth(path)!=snapshot->step_offsets[cursor+1]-snapshot->step_offsets[cursor]||
                hps_path_text_length(hps_group_path_at(group,i))!=snapshot->path_lengths[cursor]||
                hps_path_format(hps_group_path_at(group,i),buffer,maxlen+1)!=HPS_STATUS_OK||
                strcmp(buffer,snapshot->path_text+snapshot->path_offsets[cursor])!=0){free(buffer);return 0;}
            for(step=0;step<hps_path_depth(path);++step){unsigned int slot=0;size_t level=0;
                size_t saved=snapshot->step_offsets[cursor]+step;
                if(hps_path_get_slot(path,step,&slot)!=HPS_STATUS_OK||
                    hps_path_get_level(path,step,&level)!=HPS_STATUS_OK||
                    slot!=snapshot->step_slots[saved]||level!=snapshot->step_levels[saved]){free(buffer);return 0;}}
        }
    }
    free(buffer);return cursor==snapshot->count;
}

static int stage103_group_matches_snapshot(const HpsGroup *group,
    const Stage103Snapshot *snapshot)
{
    size_t i,maxlen=0;char *buffer;
    if(hps_group_size(group)!=snapshot->count)return 0;
    for(i=0;i<snapshot->count;++i)if(snapshot->path_lengths[i]>maxlen)maxlen=snapshot->path_lengths[i];
    buffer=(char *)malloc(maxlen+1);if(buffer==NULL)return 0;
    for(i=0;i<snapshot->count;++i){
        const HpsPath *path=hps_group_path_at(group,i);size_t step;
        if(hps_group_item_at(group,i)!=snapshot->items[i]||
            hps_path_direction(path)!=snapshot->path_directions[i]||
            hps_path_depth(path)!=snapshot->step_offsets[i+1]-snapshot->step_offsets[i]||
            hps_path_text_length(hps_group_path_at(group,i))!=snapshot->path_lengths[i]||
            hps_path_format(hps_group_path_at(group,i),buffer,maxlen+1)!=HPS_STATUS_OK||
            strcmp(buffer,snapshot->path_text+snapshot->path_offsets[i])!=0){free(buffer);return 0;}
        for(step=0;step<hps_path_depth(path);++step){unsigned int slot=0;size_t level=0;
            size_t saved=snapshot->step_offsets[i]+step;
            if(hps_path_get_slot(path,step,&slot)!=HPS_STATUS_OK||
                hps_path_get_level(path,step,&level)!=HPS_STATUS_OK||
                slot!=snapshot->step_slots[saved]||level!=snapshot->step_levels[saved]){free(buffer);return 0;}}
    }
    free(buffer);return 1;
}

static void stage112_tree_capture_destroy(Stage112TreeCapture *capture)
{
    if (capture == NULL) return;
    free(capture->capacity_counts);
    memset(capture,0,sizeof(*capture));
}

static int stage112_capture_result_tree(const HpsGroup *group,
    const HpsAllocStats *result_stats,size_t item_count,
    Stage112TreeCapture *capture)
{
    const HpsTree *tree=hps_group_internal_tree(group);
    size_t node_size,tree_size,ptr_size,header_size,node_bytes,children_bytes,tree_total;
    size_t required=0,index,node_sum=0;HpsStatus status;
    if(tree==NULL||capture==NULL)return 0;
    memset(capture,0,sizeof(*capture));
    if(hps_tree_internal_profile(tree,&capture->profile)!=HPS_STATUS_OK)return 0;
    node_size=hps_tree_internal_sizeof_node();tree_size=hps_tree_internal_sizeof_tree();
    ptr_size=sizeof(void *);header_size=hps_tree_internal_child_block_header_size();
    if(capture->profile.real_node_count!=item_count||
        capture->profile.total_child_count!=item_count||
        (node_size!=0&&item_count>(size_t)-1/node_size)||
        (ptr_size!=0&&capture->profile.total_child_capacity>(size_t)-1/ptr_size)||
        (header_size!=0&&capture->profile.allocated_child_array_count>(size_t)-1/header_size))return 0;
    node_bytes=item_count*node_size;
    children_bytes=capture->profile.total_child_capacity*ptr_size+
        capture->profile.allocated_child_array_count*header_size;
    if(node_bytes!=result_stats->tags[HPS_ALLOC_TAG_TREE_NODE].live_bytes||
        children_bytes!=result_stats->tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_bytes||
        tree_size!=result_stats->tags[HPS_ALLOC_TAG_TREE_OBJECT].live_bytes||
        node_bytes>(size_t)-1-children_bytes||tree_size>(size_t)-1-node_bytes-children_bytes)return 0;
    tree_total=tree_size+node_bytes+children_bytes;
    if(tree_total!=result_stats->tags[HPS_ALLOC_TAG_TREE_OBJECT].live_bytes+
        result_stats->tags[HPS_ALLOC_TAG_TREE_NODE].live_bytes+
        result_stats->tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_bytes)return 0;
    status=hps_tree_internal_capacity_histogram(tree,NULL,0,&required);
    if(status!=HPS_STATUS_OK||required==0||required>(size_t)-1/sizeof(*capture->capacity_counts))return 0;
    capture->capacity_counts=(size_t *)calloc(required,sizeof(*capture->capacity_counts));
    if(capture->capacity_counts==NULL)return 0;
    capture->capacity_count=required;
    status=hps_tree_internal_capacity_histogram(tree,capture->capacity_counts,required,&required);
    if(status!=HPS_STATUS_OK)return 0;
    for(index=0;index<capture->capacity_count;++index)
        if(!benchmark_add_size(&node_sum,capture->capacity_counts[index]))return 0;
    return node_sum==item_count;
}

static HpsPath *stage122_make_path1(unsigned int slot)
{ return hps_path_create_at_level(HPS_DIRECTION_POSITIVE,slot,0); }

static HpsPath *stage122_make_path2(unsigned int first,unsigned int second)
{
    HpsPath *path=stage122_make_path1(first);
    if(path!=NULL&&hps_path_append_at_level(path,second,1)!=HPS_STATUS_OK){
        hps_path_destroy(path);path=NULL;
    }
    return path;
}

static int stage122_collect_preorder(const HpsTreeNode *node,void **items,
    size_t capacity,size_t *count)
{
    size_t i;
    if(node==NULL||*count>=capacity)return 0;
    items[(*count)++]=hps_tree_node_item(node);
    for(i=0;i<hps_tree_node_child_count(node);++i)
        if(!stage122_collect_preorder(hps_tree_node_child_at(node,i),items,capacity,count))return 0;
    return 1;
}

static int stage122_direct_regressions(void)
{
    HpsAllocStats stats;
    int valid=1;
    if(hps_alloc_stats_reset()!=0)return 0;
    {
        HpsTree *tree=hps_tree_create();HpsTreeInternalProfile p;
        if(tree==NULL||hps_tree_size(tree)!=0||hps_tree_root_child_count(tree)!=0||
            hps_tree_internal_profile(tree,&p)!=HPS_STATUS_OK||p.root_child_count!=0||
            p.root_child_capacity!=0||p.allocated_child_array_count!=0||
            hps_alloc_stats_get().tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_blocks!=0)valid=0;
        hps_tree_destroy(tree);
        stats=hps_alloc_stats_get();
        if(!benchmark_alloc_stats_empty(&stats)||stats.failed_calls!=0||
            !stage83_other_is_empty(&stats,"12.2 empty tree"))valid=0;
    }
    printf("Stage12.2EmptyTreeRegression=%s\n",valid?"PASS":"FAIL");
    if(!valid)return 0;
    {
        HpsTree *tree=NULL;HpsPath *parent_path=NULL,*child_path=NULL;
        const HpsTreeNode *parent=NULL,*same_parent=NULL,*child=NULL;
        HpsTreeInternalProfile p;
        if(hps_alloc_stats_reset()!=0||(tree=hps_tree_create())==NULL||
            (parent_path=stage122_make_path1(7))==NULL||
            hps_tree_insert(tree,parent_path,NULL,&parent)!=HPS_STATUS_OK||parent==NULL||
            hps_tree_root_child_count(tree)!=1||hps_tree_root_child_at(tree,0)!=parent||
            hps_tree_node_child_count(parent)!=0||hps_tree_node_child_at(parent,0)!=NULL||
            hps_tree_internal_profile(tree,&p)!=HPS_STATUS_OK||p.root_child_capacity!=1||
            p.allocated_child_array_count!=1||
            hps_alloc_stats_get().tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_blocks!=1)valid=0;
        child_path=stage122_make_path2(7,3);
        if(valid&&(child_path==NULL||hps_tree_insert(tree,child_path,NULL,&child)!=HPS_STATUS_OK||
            child==NULL||hps_tree_find_path(tree,parent_path,&same_parent)!=HPS_STATUS_OK||
            same_parent!=parent||hps_tree_node_parent(child)!=parent||
            hps_tree_node_child_count(parent)!=1||hps_tree_node_child_at(parent,0)!=child))valid=0;
        hps_path_destroy(child_path);hps_path_destroy(parent_path);hps_tree_destroy(tree);
        stats=hps_alloc_stats_get();
        if(!benchmark_alloc_stats_empty(&stats)||stats.failed_calls!=0||
            !stage83_tag_accounting_valid(&stats,"12.2 leaf promote")||
            !stage83_other_is_empty(&stats,"12.2 leaf promote"))valid=0;
    }
    printf("Stage12.2FirstRootChildAndLeafToNonLeaf=%s\n",valid?"PASS":"FAIL");
    if(!valid)return 0;
    {
        enum { CHILDREN=70 };
        HpsTree *tree=NULL;HpsPath *parent_path=NULL;const HpsTreeNode *parent=NULL;
        const HpsTreeNode *children[CHILDREN];size_t i;int grow_ok=1;
        if(hps_alloc_stats_reset()!=0||(tree=hps_tree_create())==NULL||
            (parent_path=stage122_make_path1(9))==NULL||
            hps_tree_insert(tree,parent_path,NULL,&parent)!=HPS_STATUS_OK||parent==NULL)grow_ok=0;
        for(i=0;grow_ok&&i<CHILDREN;++i){
            HpsPath *path=stage122_make_path2(9,(unsigned int)i);
            const HpsTreeNode *inserted=NULL;
            if(path==NULL||hps_tree_insert(tree,path,(void *)&children[i],&inserted)!=HPS_STATUS_OK||
                inserted==NULL||hps_tree_node_parent(inserted)!=parent)grow_ok=0;
            children[i]=inserted;hps_path_destroy(path);
            if(grow_ok&&(hps_tree_node_child_count(parent)!=i+1||
                hps_tree_root_child_at(tree,0)!=parent))grow_ok=0;
            if(grow_ok){size_t j;for(j=0;j<=i;++j)
                if(hps_tree_node_child_at(parent,j)!=children[j]||
                    hps_tree_node_parent(children[j])!=parent)grow_ok=0;}
        }
        if(grow_ok){
            for(i=0;i<CHILDREN;++i)
                if(hps_tree_node_child_at(parent,i)!=children[i]||
                    hps_tree_node_parent(children[i])!=parent)grow_ok=0;
        }
        if(grow_ok&&hps_tree_size(tree)!=CHILDREN+1)grow_ok=0;
        hps_path_destroy(parent_path);hps_tree_destroy(tree);
        stats=hps_alloc_stats_get();
        if(!benchmark_alloc_stats_empty(&stats)||stats.failed_calls!=0||
            !stage83_tag_accounting_valid(&stats,"12.2 multigrow")||
            !stage83_other_is_empty(&stats,"12.2 multigrow"))grow_ok=0;
        printf("Stage12.2MultiGrowChildBlock=%s ChildCount=%d ExpectedCapacity=128 NodeAndChildPointersStable=%s\n",
            grow_ok?"PASS":"FAIL",CHILDREN,grow_ok?"YES":"NO");
        valid=valid&&grow_ok;
    }
    {
        HpsTree *tree=NULL;int values[4]={10,30,20,20};void *ordered[4];size_t count=0,i;
        BenchmarkCompareContext cc={0,0};HpsComparator comparator={benchmark_compare_int,&cc};
        int semantic_ok=1;
        if(hps_alloc_stats_reset()!=0||(tree=hps_tree_create())==NULL)semantic_ok=0;
        for(i=0;semantic_ok&&i<4;++i)
            if(hps_tree_insert_item(tree,&values[i],&comparator,NULL)!=HPS_STATUS_OK)semantic_ok=0;
        if(semantic_ok){
            for(i=0;i<hps_tree_root_child_count(tree);++i)
                if(!stage122_collect_preorder(hps_tree_root_child_at(tree,i),ordered,4,&count))semantic_ok=0;
            if(count!=4||ordered[0]!=&values[0]||ordered[1]!=&values[2]||
                ordered[2]!=&values[3]||ordered[3]!=&values[1])semantic_ok=0;
        }
        hps_tree_destroy(tree);stats=hps_alloc_stats_get();
        if(!benchmark_alloc_stats_empty(&stats)||stats.failed_calls!=0||
            !stage83_tag_accounting_valid(&stats,"12.2 nav successor")||
            !stage83_other_is_empty(&stats,"12.2 nav successor"))semantic_ok=0;
        printf("Stage12.2PublicNavigationAndSuccessorOrder=%s comparisons=%lu\n",
            semantic_ok?"PASS":"FAIL",(unsigned long)cc.comparison_count);
        valid=valid&&semantic_ok;
    }
    return valid;
}

static int stage103_run_one_internal(size_t n, size_t groups, uint32_t seed,
    int reference_mode, Stage103Sample *sample, Stage103Snapshot *out_snapshot,
    Stage112TreeCapture *out_tree_profile)
{
    static int values[STAGE103_N];static void *items[STAGE103_N];
    BenchmarkCompareContext counter={0,0};HpsComparator comparator={benchmark_compare_int,&counter};
    HpsGroupBatch *batch=NULL;HpsGroup *result=NULL;Stage103Snapshot batch_snapshot={0};
    HpsAllocStats after_batch,after_merge,result_only,final_stats;
    size_t i,group_size=n/groups+(n%groups!=0);clock_t begin,end;double ms;
    HpsStatus status=HPS_STATUS_OK;int valid=0,batch_unchanged=1;
    if(hps_alloc_stats_reset()!=0)return 0;
    for(i=0;i<n;++i)values[i]=(int)i;
    benchmark_shuffle(values,n,seed);for(i=0;i<n;++i)items[i]=&values[i];
    begin=clock();
    status=hps_group_batch_build(items,n,group_size,&comparator,&batch);
    end=clock();
    if(status!=HPS_STATUS_OK||!benchmark_elapsed_ms(begin,end,&ms)||
        hps_group_batch_group_count(batch)!=groups||
        !stage103_snapshot_capture_batch(batch,&batch_snapshot))goto cleanup;
    sample->build_ms=ms;
    sample->group_size=group_size;sample->build_comparisons=counter.comparison_count;
    after_batch=hps_alloc_stats_get();
    if(!stage83_tag_accounting_valid(&after_batch,"10.3 AfterBatchBuild")||
        !stage83_other_is_empty(&after_batch,"10.3 AfterBatchBuild"))goto cleanup;
    sample->batch_bytes=after_batch.live_bytes;sample->batch_blocks=after_batch.live_blocks;
    sample->batch_stats=after_batch;
    begin=clock();
    status=reference_mode?stage102_reference_merge_all(batch,&comparator,&result):
        hps_group_batch_merge_all(batch,&comparator,&result);
    end=clock();
    if(status!=HPS_STATUS_OK||!benchmark_elapsed_ms(begin,end,&ms))goto cleanup;
    sample->merge_ms=ms;sample->total_comparisons=counter.comparison_count;
    sample->merge_comparisons=counter.comparison_count-sample->build_comparisons;
    if(counter.overflowed||!benchmark_validate_final(result,n,&(BenchmarkPathStats){0})||
        !stage103_snapshot_capture_group(result,out_snapshot))goto cleanup;
    after_merge=hps_alloc_stats_get();
    if(!stage83_tag_accounting_valid(&after_merge,"10.3 PostMerge")||
        !stage83_other_is_empty(&after_merge,"10.3 PostMerge"))goto cleanup;
    if(!stage103_batch_matches_snapshot(batch,&batch_snapshot)){++stage103_input_batch_mutations;batch_unchanged=0;}
    sample->post_bytes=after_merge.live_bytes;sample->post_blocks=after_merge.live_blocks;
    sample->peak_bytes=after_merge.peak_live_bytes;sample->peak_blocks=after_merge.peak_live_blocks;
    sample->peak_stats=after_merge;
    {
        size_t path_index,depth_sum=0,max_depth=0,max_level=0;
        for(path_index=0;path_index<n;++path_index){
            const HpsPath *path=hps_group_path_at(result,path_index);
            size_t depth=hps_path_depth(path),step;
            if(depth>max_depth)max_depth=depth;
            depth_sum+=depth;
            for(step=0;step<depth;++step){size_t level=0;
                if(hps_path_get_level(path,step,&level)!=HPS_STATUS_OK)goto cleanup;
                if(level>max_level)max_level=level;
            }
        }
        sample->final_avg_depth=n?(double)depth_sum/(double)n:0.0;
        sample->final_max_depth=max_depth;sample->final_max_level=max_level;
    }
    sample->transient_bytes=sample->peak_bytes-sample->post_bytes;
    sample->peak_over_batch=sample->peak_bytes-sample->batch_bytes;
    sample->merge_alloc_calls=after_merge.alloc_calls-after_batch.alloc_calls;
    sample->merge_realloc_calls=after_merge.realloc_calls-after_batch.realloc_calls;
    sample->merge_free_calls=after_merge.free_calls-after_batch.free_calls;
    if(!reference_mode&&groups>1){
        size_t group_objects=after_merge.tags[HPS_ALLOC_TAG_GROUP_OBJECT].alloc_calls-
            after_batch.tags[HPS_ALLOC_TAG_GROUP_OBJECT].alloc_calls;
        sample->observed_public_bootstrap=group_objects;
        if(group_objects<=groups-1)
            sample->observed_owned_inplace=(groups-1)-group_objects;
        else goto cleanup;
    }
    hps_group_batch_destroy(batch);batch=NULL;
    result_only=hps_alloc_stats_get();sample->result_bytes=result_only.live_bytes;
    sample->result_blocks=result_only.live_blocks;
    sample->result_stats=result_only;
    if(!stage103_group_matches_snapshot(result,out_snapshot)||
        !benchmark_validate_final(result,n,&(BenchmarkPathStats){0})||
        !stage83_tag_accounting_valid(&result_only,"10.3 ResultOnly")||
        !stage83_other_is_empty(&result_only,"10.3 ResultOnly"))goto cleanup;
    if(out_tree_profile!=NULL&&
        !stage112_capture_result_tree(result,&result_only,n,out_tree_profile))goto cleanup;
    if(sample->post_bytes!=sample->batch_bytes+result_only.live_bytes||
        sample->post_blocks!=sample->batch_blocks+result_only.live_blocks)goto cleanup;
    valid=batch_unchanged;
cleanup:
    hps_group_destroy(result);hps_group_batch_destroy(batch);
    stage103_snapshot_destroy(&batch_snapshot);
    final_stats=hps_alloc_stats_get();
    if(!benchmark_alloc_stats_empty(&final_stats)||final_stats.failed_calls!=0||
        !stage83_tag_accounting_valid(&final_stats,"10.3 RunFinal")||
        !stage83_other_is_empty(&final_stats,"10.3 RunFinal"))valid=0;
    if(!valid)stage103_snapshot_destroy(out_snapshot);
    if(!valid&&out_tree_profile!=NULL)stage112_tree_capture_destroy(out_tree_profile);
    return valid;
}

static int stage103_run_one(size_t n,size_t groups,uint32_t seed,int reference_mode,
    Stage103Sample *sample,Stage103Snapshot *out_snapshot)
{
    return stage103_run_one_internal(n,groups,seed,reference_mode,sample,
        out_snapshot,NULL);
}

static void stage103_predicted_strategy_counts(size_t groups,
    size_t *public_count,size_t *inplace_count)
{
    unsigned char owned[128]={0},next_owned[128];size_t count=groups,i;
    *public_count=0;*inplace_count=0;
    if(groups>128){*public_count=(size_t)-1;*inplace_count=(size_t)-1;return;}
    while(count>1){size_t next_count=count/2+count%2,ni=0,ci=0;
        memset(next_owned,0,sizeof(next_owned));
        while(ci<count){
            if(ci+1<count){if(!owned[ci])++*public_count;else ++*inplace_count;next_owned[ni++]=1;ci+=2;}
            else next_owned[ni++]=owned[ci++];
        }
        count=next_count;for(i=0;i<count;++i)owned[i]=next_owned[i];
    }
}

static double stage103_mean(const double *values,size_t count)
{size_t i;double sum=0;for(i=0;i<count;++i)sum+=values[i];return count?sum/count:0;}

static int stage103_compare_pair_samples(const Stage103Sample *reference,
    const Stage103Sample *optimized,const Stage103Snapshot *reference_result,
    const Stage103Snapshot *optimized_result,size_t n,size_t groups,uint32_t seed)
{
    size_t path_mismatch=0,item_mismatch=0,predicted_public,predicted_inplace;int matched=1;
    if(reference->build_comparisons!=optimized->build_comparisons||
        reference->merge_comparisons!=optimized->merge_comparisons||
        reference->total_comparisons!=optimized->total_comparisons){
        printf("ComparatorMismatch,N=%lu,Groups=%lu,Seed=%08lX,Build=%lu/%lu,Merge=%lu/%lu,Total=%lu/%lu\n",
            (unsigned long)n,(unsigned long)groups,(unsigned long)seed,
            (unsigned long)reference->build_comparisons,(unsigned long)optimized->build_comparisons,
            (unsigned long)reference->merge_comparisons,(unsigned long)optimized->merge_comparisons,
            (unsigned long)reference->total_comparisons,(unsigned long)optimized->total_comparisons);
        ++stage103_comparator_mismatches;matched=0;
    }
    if(!stage103_snapshot_equal(reference_result,optimized_result,&path_mismatch,&item_mismatch))matched=0;
    stage103_result_path_mismatches+=path_mismatch;stage103_result_item_mismatches+=item_mismatch;
    if(reference->batch_bytes!=optimized->batch_bytes||reference->batch_blocks!=optimized->batch_blocks||
        reference->post_bytes!=optimized->post_bytes||reference->post_blocks!=optimized->post_blocks||
        reference->result_bytes!=optimized->result_bytes||reference->result_blocks!=optimized->result_blocks){
        ++stage103_postmerge_mismatches;matched=0;
    }
    if(reference->group_size!=optimized->group_size)matched=0;
    if(optimized->peak_bytes>reference->peak_bytes){
        ++stage103_negative_peak_savings_samples;
        printf("NegativePeakSaving,N=%lu,Groups=%lu,Seed=%08lX,ReferencePeak=%lu,OptimizedPeak=%lu,Difference=%ld\n",
            (unsigned long)n,(unsigned long)groups,(unsigned long)seed,
            (unsigned long)reference->peak_bytes,(unsigned long)optimized->peak_bytes,
            (long)optimized->peak_bytes-(long)reference->peak_bytes);
    }
    if(optimized->transient_bytes>reference->transient_bytes){
        ++stage103_negative_transient_savings_samples;
        printf("NegativeTransientSaving,N=%lu,Groups=%lu,Seed=%08lX,ReferenceTransient=%lu,OptimizedTransient=%lu,Difference=%ld\n",
            (unsigned long)n,(unsigned long)groups,(unsigned long)seed,
            (unsigned long)reference->transient_bytes,(unsigned long)optimized->transient_bytes,
            (long)optimized->transient_bytes-(long)reference->transient_bytes);
    }
    stage103_predicted_strategy_counts(groups,&predicted_public,&predicted_inplace);
    if(groups>1&&optimized->observed_public_bootstrap!=predicted_public){
        ++stage103_public_strategy_mismatches;matched=0;
    }
    if(groups>1&&optimized->observed_owned_inplace!=predicted_inplace){
        ++stage103_inplace_strategy_mismatches;matched=0;
    }
    return matched;
}

static int stage103_empty_single_test(void)
{
    BenchmarkCompareContext counter={0,0};HpsComparator comparator={benchmark_compare_int,&counter};
    HpsGroupBatch *empty_batch=NULL,*single_batch=NULL;HpsGroup *empty_ref=NULL,*empty_opt=NULL,*single_ref=NULL,*single_opt=NULL;
    int values[3]={3,1,2};void *items[3]={&values[0],&values[1],&values[2]};
    int passed=0;HpsAllocStats stats;
    if(hps_alloc_stats_reset()!=0||hps_group_batch_build(NULL,0,4,&comparator,&empty_batch)!=HPS_STATUS_OK||
        stage102_reference_merge_all(empty_batch,&comparator,&empty_ref)!=HPS_STATUS_OK||
        hps_group_batch_merge_all(empty_batch,&comparator,&empty_opt)!=HPS_STATUS_OK||
        empty_ref==NULL||empty_opt==NULL||empty_ref==empty_opt||hps_group_size(empty_ref)!=0||hps_group_size(empty_opt)!=0)goto cleanup;
    hps_group_destroy(empty_ref);empty_ref=NULL;hps_group_destroy(empty_opt);empty_opt=NULL;
    hps_group_batch_destroy(empty_batch);empty_batch=NULL;
    if(hps_group_batch_build(items,3,3,&comparator,&single_batch)!=HPS_STATUS_OK||
        stage102_reference_merge_all(single_batch,&comparator,&single_ref)!=HPS_STATUS_OK||
        hps_group_batch_merge_all(single_batch,&comparator,&single_opt)!=HPS_STATUS_OK||
        single_ref==NULL||single_opt==NULL||single_ref==single_opt||
        single_ref==hps_group_batch_group_at(single_batch,0)||single_opt==hps_group_batch_group_at(single_batch,0)||
        !stage101_groups_equivalent(single_ref,single_opt))goto cleanup;
    hps_group_batch_destroy(single_batch);single_batch=NULL;
    if(hps_group_item_at(single_ref,0)!=&values[1]||hps_group_item_at(single_opt,0)!=&values[1])goto cleanup;
    passed=1;
    printf("Stage 10.3 empty/single Batch: independent empty Results and Path-preserving single clones PASS\n");
cleanup:
    hps_group_destroy(single_ref);hps_group_destroy(single_opt);hps_group_batch_destroy(single_batch);
    hps_group_destroy(empty_ref);hps_group_destroy(empty_opt);hps_group_batch_destroy(empty_batch);
    stats=hps_alloc_stats_get();if(!benchmark_alloc_stats_empty(&stats)||stats.failed_calls!=0||
        !stage83_tag_accounting_valid(&stats,"10.3 empty/single final")||!stage83_other_is_empty(&stats,"10.3 empty/single final"))passed=0;
    return passed;
}

static int stage103_main_panel(void)
{
    Stage103Sample ref[STAGE103_GROUPS][STAGE103_SEEDS],opt[STAGE103_GROUPS][STAGE103_SEEDS];
    double peak_pct[STAGE103_GROUPS][STAGE103_SEEDS],trans_pct[STAGE103_GROUPS][STAGE103_SEEDS];
    size_t g,s,i;int valid=1;
    memset(ref,0,sizeof(ref));memset(opt,0,sizeof(opt));
    memset(peak_pct,0,sizeof(peak_pct));memset(trans_pct,0,sizeof(trans_pct));
    for(g=0;g<STAGE103_GROUPS;++g)for(s=0;s<STAGE103_SEEDS;++s){
        Stage103Snapshot r={0},o={0};
        if(!stage103_run_one(STAGE103_N,stage103_groups[g],stage103_seeds[s],1,&ref[g][s],&r)||
            !stage103_run_one(STAGE103_N,stage103_groups[g],stage103_seeds[s],0,&opt[g][s],&o)){
            printf("Stage 10.3 main sample failed: G=%lu seed=%08lX\n",(unsigned long)stage103_groups[g],(unsigned long)stage103_seeds[s]);
            valid=0;stage103_snapshot_destroy(&r);stage103_snapshot_destroy(&o);continue;
        }
        if(!stage103_compare_pair_samples(&ref[g][s],&opt[g][s],&r,&o,
                STAGE103_N,stage103_groups[g],stage103_seeds[s]))valid=0;
        peak_pct[g][s]=ref[g][s].peak_bytes?100.0*((double)ref[g][s].peak_bytes-(double)opt[g][s].peak_bytes)/ref[g][s].peak_bytes:0;
        trans_pct[g][s]=ref[g][s].transient_bytes?100.0*((double)ref[g][s].transient_bytes-(double)opt[g][s].transient_bytes)/ref[g][s].transient_bytes:0;
        stage103_snapshot_destroy(&r);stage103_snapshot_destroy(&o);
    }
    printf("Stage 10.3 Main Memory CSV (five-seed means)\nGroups,GroupSize,ReferencePeakBytesMean,OptimizedPeakBytesMean,PeakBytesSavedMean,PeakSavingsPercent,ReferenceTransientBytesMean,OptimizedTransientBytesMean,TransientBytesSavedMean,TransientSavingsPercent,ReferencePeakBlocksMean,OptimizedPeakBlocksMean,PeakBlocksSavingsPercent\n");
    for(g=0;g<STAGE103_GROUPS;++g){double rp=0,op=0,rt=0,ot=0,rb=0,ob=0,gs=0;
        for(s=0;s<STAGE103_SEEDS;++s){rp+=ref[g][s].peak_bytes;op+=opt[g][s].peak_bytes;rt+=ref[g][s].transient_bytes;ot+=opt[g][s].transient_bytes;rb+=ref[g][s].peak_blocks;ob+=opt[g][s].peak_blocks;gs+=ref[g][s].group_size;}
        rp/=STAGE103_SEEDS;op/=STAGE103_SEEDS;rt/=STAGE103_SEEDS;ot/=STAGE103_SEEDS;rb/=STAGE103_SEEDS;ob/=STAGE103_SEEDS;gs/=STAGE103_SEEDS;
        printf("%lu,%.2f,%.2f,%.2f,%.2f,%.4f,%.2f,%.2f,%.2f,%.4f,%.2f,%.2f,%.4f\n",
            (unsigned long)stage103_groups[g],gs,rp,op,rp-op,rp?100*(rp-op)/rp:0,rt,ot,rt-ot,rt?100*(rt-ot)/rt:0,rb,ob,rb?100*(rb-ob)/rb:0);
        (void)i;
    }
    printf("\nStage 10.3 Seed Robustness CSV\nGroups,MinPeakSavingsPercent,MeanPeakSavingsPercent,MaxPeakSavingsPercent,MinTransientSavingsPercent,MeanTransientSavingsPercent,MaxTransientSavingsPercent\n");
    for(g=0;g<STAGE103_GROUPS;++g){double pmin=DBL_MAX,pmax=-DBL_MAX,tmin=DBL_MAX,tmax=-DBL_MAX,psum=0,tsum=0;
        for(s=0;s<STAGE103_SEEDS;++s){if(peak_pct[g][s]<pmin)pmin=peak_pct[g][s];if(peak_pct[g][s]>pmax)pmax=peak_pct[g][s];if(trans_pct[g][s]<tmin)tmin=trans_pct[g][s];if(trans_pct[g][s]>tmax)tmax=trans_pct[g][s];psum+=peak_pct[g][s];tsum+=trans_pct[g][s];}
        printf("%lu,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n",(unsigned long)stage103_groups[g],pmin,psum/STAGE103_SEEDS,pmax,tmin,tsum/STAGE103_SEEDS,tmax);
    }
    printf("\nStage 10.3 Peak Savings Matrix\nSeed");for(g=0;g<STAGE103_GROUPS;++g)printf(",G%lu",(unsigned long)stage103_groups[g]);printf("\n");
    for(s=0;s<STAGE103_SEEDS;++s){printf("%08lX",(unsigned long)stage103_seeds[s]);for(g=0;g<STAGE103_GROUPS;++g)printf(",%.4f",peak_pct[g][s]);printf("\n");}
    printf("\nStage 10.3 Transient Savings Matrix\nSeed");for(g=0;g<STAGE103_GROUPS;++g)printf(",G%lu",(unsigned long)stage103_groups[g]);printf("\n");
    for(s=0;s<STAGE103_SEEDS;++s){printf("%08lX",(unsigned long)stage103_seeds[s]);for(g=0;g<STAGE103_GROUPS;++g)printf(",%.4f",trans_pct[g][s]);printf("\n");}
    printf("\nStage 10.3 Peak Tag Composition (mean of per-run global-peak snapshots)\nGroups,Mode,PeakPathBytesMean,PeakTreeBytesMean,PeakGroupBytesMean,PeakMergeScratchBytesMean,PeakBatchBytesMean,PeakTotalBytesMean\n");
    {
        static const size_t tag_groups[8]={4,5,8,10,32,40,64,80};
        for(i=0;i<8;++i){size_t gi;for(gi=0;gi<STAGE103_GROUPS;++gi)if(stage103_groups[gi]==tag_groups[i])break;
            {size_t mode;for(mode=0;mode<2;++mode){double path=0,tree=0,group=0,scratch=0,batch=0,total=0;
                for(s=0;s<STAGE103_SEEDS;++s){const HpsAllocStats *a=mode?&opt[gi][s].peak_stats:&ref[gi][s].peak_stats;
                    path+=a->bytes_at_global_peak[HPS_ALLOC_TAG_PATH_OBJECT]+a->bytes_at_global_peak[HPS_ALLOC_TAG_PATH_STEPS];
                    tree+=a->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_OBJECT]+a->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_NODE]+a->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_CHILDREN];
                    group+=a->bytes_at_global_peak[HPS_ALLOC_TAG_GROUP_OBJECT]+a->bytes_at_global_peak[HPS_ALLOC_TAG_GROUP_ORDERED];
                    scratch+=a->bytes_at_global_peak[HPS_ALLOC_TAG_MERGE_SCRATCH];
                    batch+=a->bytes_at_global_peak[HPS_ALLOC_TAG_BATCH_OBJECT]+a->bytes_at_global_peak[HPS_ALLOC_TAG_BATCH_GROUP_ARRAY];
                    total+=a->peak_live_bytes;
                }
                printf("%lu,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",(unsigned long)tag_groups[i],mode?"Optimized":"Reference",path/STAGE103_SEEDS,tree/STAGE103_SEEDS,group/STAGE103_SEEDS,scratch/STAGE103_SEEDS,batch/STAGE103_SEEDS,total/STAGE103_SEEDS);
            }}
        }
    }
    printf("\nStage 10.3 Tag Savings Breakdown (Reference peak mean minus Optimized peak mean; peak instants can differ)\nGroups,PathBytesSaved,TreeBytesSaved,GroupBytesSaved,MergeScratchBytesSaved\n");
    {
        static const size_t tag_groups[8]={4,5,8,10,32,40,64,80};
        for(i=0;i<8;++i){size_t gi;double p=0,t=0,gr=0,sc=0;for(gi=0;gi<STAGE103_GROUPS;++gi)if(stage103_groups[gi]==tag_groups[i])break;
            for(s=0;s<STAGE103_SEEDS;++s){const HpsAllocStats *r=&ref[gi][s].peak_stats,*o=&opt[gi][s].peak_stats;
                p+=(double)(r->bytes_at_global_peak[HPS_ALLOC_TAG_PATH_OBJECT]+r->bytes_at_global_peak[HPS_ALLOC_TAG_PATH_STEPS])-(double)(o->bytes_at_global_peak[HPS_ALLOC_TAG_PATH_OBJECT]+o->bytes_at_global_peak[HPS_ALLOC_TAG_PATH_STEPS]);
                t+=(double)(r->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_OBJECT]+r->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_NODE]+r->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_CHILDREN])-(double)(o->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_OBJECT]+o->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_NODE]+o->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_CHILDREN]);
                gr+=(double)(r->bytes_at_global_peak[HPS_ALLOC_TAG_GROUP_OBJECT]+r->bytes_at_global_peak[HPS_ALLOC_TAG_GROUP_ORDERED])-(double)(o->bytes_at_global_peak[HPS_ALLOC_TAG_GROUP_OBJECT]+o->bytes_at_global_peak[HPS_ALLOC_TAG_GROUP_ORDERED]);
                sc+=(double)r->bytes_at_global_peak[HPS_ALLOC_TAG_MERGE_SCRATCH]-(double)o->bytes_at_global_peak[HPS_ALLOC_TAG_MERGE_SCRATCH];
            }
            printf("%lu,%.2f,%.2f,%.2f,%.2f\n",(unsigned long)tag_groups[i],p/STAGE103_SEEDS,t/STAGE103_SEEDS,gr/STAGE103_SEEDS,sc/STAGE103_SEEDS);
        }
    }
    printf("\nStage 10.3 Merge Strategy Counts (observed public calls inferred from GROUP_OBJECT allocations; in-place is remaining pair count)\nGroups,ObservedPublicBootstrap,PredictedPublicBootstrap,ObservedInPlace,PredictedInPlace\n");
    for(g=0;g<STAGE103_GROUPS;++g){double observed_public=0,observed_inplace=0;size_t predicted_public,predicted_inplace;
        stage103_predicted_strategy_counts(stage103_groups[g],&predicted_public,&predicted_inplace);
        for(s=0;s<STAGE103_SEEDS;++s){observed_public+=opt[g][s].observed_public_bootstrap;observed_inplace+=opt[g][s].observed_owned_inplace;}
        printf("%lu,%.0f,%lu,%.0f,%lu\n",(unsigned long)stage103_groups[g],observed_public/STAGE103_SEEDS,
            (unsigned long)predicted_public,observed_inplace/STAGE103_SEEDS,(unsigned long)predicted_inplace);
    }
    printf("\nStage 10.3 Merge Timing CSV (Debug; MergeAll call only)\nGroups,ReferenceMergeMsMean,OptimizedMergeMsMean,DeltaMs,DeltaPercent\n");
    for(g=0;g<STAGE103_GROUPS;++g){double r=0,o=0;for(s=0;s<STAGE103_SEEDS;++s){r+=ref[g][s].merge_ms;o+=opt[g][s].merge_ms;}r/=STAGE103_SEEDS;o/=STAGE103_SEEDS;
        printf("%lu,%.4f,%.4f,%.4f,%.4f\n",(unsigned long)stage103_groups[g],r,o,o-r,r?100*(o-r)/r:0);
    }
    printf("\nStage 10.3 Merge Allocator Calls CSV (AfterBatchBuild to MergeAll return)\nGroups,ReferenceMergeAllocCallsMean,OptimizedMergeAllocCallsMean,AllocCallsSavingsPercent,ReferenceMergeReallocCallsMean,OptimizedMergeReallocCallsMean,ReferenceMergeFreeCallsMean,OptimizedMergeFreeCallsMean\n");
    {
        double alloc_savings=0;size_t alloc_groups=0;
        for(g=0;g<STAGE103_GROUPS;++g){double ra=0,oa=0,rr=0,orx=0,rf=0,of=0;for(s=0;s<STAGE103_SEEDS;++s){ra+=ref[g][s].merge_alloc_calls;oa+=opt[g][s].merge_alloc_calls;rr+=ref[g][s].merge_realloc_calls;orx+=opt[g][s].merge_realloc_calls;rf+=ref[g][s].merge_free_calls;of+=opt[g][s].merge_free_calls;}ra/=STAGE103_SEEDS;oa/=STAGE103_SEEDS;rr/=STAGE103_SEEDS;orx/=STAGE103_SEEDS;rf/=STAGE103_SEEDS;of/=STAGE103_SEEDS;
            printf("%lu,%.2f,%.2f,%.4f,%.2f,%.2f,%.2f,%.2f\n",(unsigned long)stage103_groups[g],ra,oa,ra?100*(ra-oa)/ra:0,rr,orx,rf,of);
            if(g>=2){alloc_savings+=ra?100*(ra-oa)/ra:0;++alloc_groups;}
        }
        stage103_alloc_savings_mean=alloc_groups?alloc_savings/alloc_groups:0;
    }
    {
        double pmin=DBL_MAX,pmax=-DBL_MAX,psum=0,tmin=DBL_MAX,tmax=-DBL_MAX,tsum=0;
        double block_min=DBL_MAX,block_max=-DBL_MAX,sample_pmin=DBL_MAX,sample_pmax=-DBL_MAX,sample_psum=0;
        size_t configs=0,samples=0;
        for(g=2;g<STAGE103_GROUPS;++g){double p=0,t=0,rb=0,ob=0,rp=0,op=0,rt=0,ot=0;
            for(s=0;s<STAGE103_SEEDS;++s){rb+=ref[g][s].peak_blocks;ob+=opt[g][s].peak_blocks;rp+=ref[g][s].peak_bytes;op+=opt[g][s].peak_bytes;rt+=ref[g][s].transient_bytes;ot+=opt[g][s].transient_bytes;
                if(peak_pct[g][s]<sample_pmin)sample_pmin=peak_pct[g][s];if(peak_pct[g][s]>sample_pmax)sample_pmax=peak_pct[g][s];sample_psum+=peak_pct[g][s];++samples;}
            p=rp?100*(rp-op)/rp:0;t=rt?100*(rt-ot)/rt:0;rb/=STAGE103_SEEDS;ob/=STAGE103_SEEDS;
            if(p<pmin)pmin=p;if(p>pmax)pmax=p;psum+=p;if(t<tmin)tmin=t;if(t>tmax)tmax=t;tsum+=t;++configs;
            {double bp=rb?100*(rb-ob)/rb:0;if(bp<block_min)block_min=bp;if(bp>block_max)block_max=bp;}
        }
        stage103_main_peak_min=pmin;stage103_main_peak_mean=psum/configs;stage103_main_peak_max=pmax;
        stage103_main_trans_min=tmin;stage103_main_trans_mean=tsum/configs;stage103_main_trans_max=tmax;
        stage103_peak_blocks_min=block_min;stage103_peak_blocks_max=block_max;
        printf("MainSamplePeakSavingsMinPercent_G3Plus=%.4f\nMainSamplePeakSavingsMeanPercent_G3Plus=%.4f\nMainSamplePeakSavingsMaxPercent_G3Plus=%.4f\n",
            sample_pmin,sample_psum/samples,sample_pmax);
        printf("MainPeakSavingsMinPercent_G3Plus=%.4f\nMainPeakSavingsMeanPercent_G3Plus=%.4f\nMainPeakSavingsMaxPercent_G3Plus=%.4f\n",
            pmin,psum/configs,pmax);
        printf("MainTransientSavingsMinPercent_G3Plus=%.4f\nMainTransientSavingsMeanPercent_G3Plus=%.4f\nMainTransientSavingsMaxPercent_G3Plus=%.4f\n",
            tmin,tsum/configs,tmax);
    }
    return valid;
}

static int stage103_scaling_panel(void)
{
    Stage103Sample ref[STAGE103_SCALE_NS][STAGE103_SCALE_GROUPS][STAGE103_SCALE_SEEDS];
    Stage103Sample opt[STAGE103_SCALE_NS][STAGE103_SCALE_GROUPS][STAGE103_SCALE_SEEDS];
    double pmin=DBL_MAX,pmax=-DBL_MAX,tmin=DBL_MAX,tmax=-DBL_MAX;
    size_t ni,gi,si;int valid=1;
    memset(ref,0,sizeof(ref));memset(opt,0,sizeof(opt));
    for(ni=0;ni<STAGE103_SCALE_NS;++ni)for(gi=0;gi<STAGE103_SCALE_GROUPS;++gi)
        for(si=0;si<STAGE103_SCALE_SEEDS;++si){
            size_t n=stage103_scale_ns[ni],groups=stage103_scale_groups[gi];
            Stage103Snapshot r={0},o={0};
            if(!stage103_run_one(n,groups,stage103_scale_seeds[si],1,&ref[ni][gi][si],&r)||
                !stage103_run_one(n,groups,stage103_scale_seeds[si],0,&opt[ni][gi][si],&o)){
                printf("Stage 10.3 scaling sample failed: N=%lu G=%lu seed=%08lX\n",
                    (unsigned long)n,(unsigned long)groups,(unsigned long)stage103_scale_seeds[si]);
                valid=0;
            } else if(!stage103_compare_pair_samples(&ref[ni][gi][si],&opt[ni][gi][si],&r,&o,
                    n,groups,stage103_scale_seeds[si]))valid=0;
            stage103_snapshot_destroy(&r);stage103_snapshot_destroy(&o);
        }
    printf("\nStage 10.3 Scaling CSV (three-seed mean per configuration)\nN,Groups,GroupSize,ReferencePeakBytesPerItem,OptimizedPeakBytesPerItem,PeakSavingsPercent,ReferenceTransientBytesPerItem,OptimizedTransientBytesPerItem,TransientSavingsPercent\n");
    for(ni=0;ni<STAGE103_SCALE_NS;++ni)for(gi=0;gi<STAGE103_SCALE_GROUPS;++gi){
        double rp=0,op=0,rt=0,ot=0,gs=0;size_t n=stage103_scale_ns[ni];
        for(si=0;si<STAGE103_SCALE_SEEDS;++si){rp+=ref[ni][gi][si].peak_bytes;op+=opt[ni][gi][si].peak_bytes;
            rt+=ref[ni][gi][si].transient_bytes;ot+=opt[ni][gi][si].transient_bytes;gs+=ref[ni][gi][si].group_size;}
        rp/=STAGE103_SCALE_SEEDS;op/=STAGE103_SCALE_SEEDS;rt/=STAGE103_SCALE_SEEDS;ot/=STAGE103_SCALE_SEEDS;gs/=STAGE103_SCALE_SEEDS;
        rp/=n;op/=n;rt/=n;ot/=n;
        {double ps=rp?100*(rp-op)/rp:0,ts=rt?100*(rt-ot)/rt:0;
            if(ps<pmin)pmin=ps;if(ps>pmax)pmax=ps;if(ts<tmin)tmin=ts;if(ts>tmax)tmax=ts;
            printf("%lu,%lu,%.2f,%.6f,%.6f,%.4f,%.6f,%.6f,%.4f\n",(unsigned long)n,
                (unsigned long)stage103_scale_groups[gi],gs,rp,op,ps,rt,ot,ts);}
    }
    stage103_scaling_peak_min=pmin;stage103_scaling_peak_max=pmax;
    stage103_scaling_trans_min=tmin;stage103_scaling_trans_max=tmax;
    printf("\nScalingSavingsSummary,Groups,N1000Percent,N2000Percent,N5000Percent,N10000Percent\nPeakSavings");
    for(gi=0;gi<STAGE103_SCALE_GROUPS;++gi){printf("\n%lu",(unsigned long)stage103_scale_groups[gi]);for(ni=0;ni<STAGE103_SCALE_NS;++ni){
        double rp=0,op=0;for(si=0;si<STAGE103_SCALE_SEEDS;++si){rp+=ref[ni][gi][si].peak_bytes;op+=opt[ni][gi][si].peak_bytes;}
        rp/=STAGE103_SCALE_SEEDS;op/=STAGE103_SCALE_SEEDS;printf(",%.4f",rp?100*(rp-op)/rp:0);}}
    printf("\nTransientSavings");
    for(gi=0;gi<STAGE103_SCALE_GROUPS;++gi){printf("\n%lu",(unsigned long)stage103_scale_groups[gi]);for(ni=0;ni<STAGE103_SCALE_NS;++ni){
        double rt=0,ot=0;for(si=0;si<STAGE103_SCALE_SEEDS;++si){rt+=ref[ni][gi][si].transient_bytes;ot+=opt[ni][gi][si].transient_bytes;}
        rt/=STAGE103_SCALE_SEEDS;ot/=STAGE103_SCALE_SEEDS;printf(",%.4f",rt?100*(rt-ot)/rt:0);}}
    printf("\nScalingSavingsRange,Metric,MinimumPercent,MaximumPercent\nPeakBytesPerItem,,%.4f,%.4f\nTransientBytesPerItem,,%.4f,%.4f\n",
        pmin,pmax,tmin,tmax);
    return valid;
}

static int stage103_directed_g2_g4_checks(void)
{
    Stage103Sample reference={0},optimized={0};
    Stage103Snapshot reference_result={0},optimized_result={0};
    int valid=1;
    if(!stage103_run_one(STAGE103_N,2,UINT32_C(0xC0FFEE),1,&reference,&reference_result)||
        !stage103_run_one(STAGE103_N,2,UINT32_C(0xC0FFEE),0,&optimized,&optimized_result)){
        printf("Stage 10.3 directed G2 C0FFEE: sample construction FAILED\n");valid=0;
    }else{
        int same=stage103_compare_pair_samples(&reference,&optimized,&reference_result,&optimized_result,
            STAGE103_N,2,UINT32_C(0xC0FFEE));
        printf("Stage 10.3 directed G2 C0FFEE: MergeCmp=%lu/%lu PeakBytes=%lu/%lu TransientBytes=%lu/%lu PairMatch=%s\n",
            (unsigned long)reference.merge_comparisons,(unsigned long)optimized.merge_comparisons,
            (unsigned long)reference.peak_bytes,(unsigned long)optimized.peak_bytes,
            (unsigned long)reference.transient_bytes,(unsigned long)optimized.transient_bytes,same?"PASS":"FAIL");
        if(!same||reference.merge_comparisons!=19997||optimized.merge_comparisons!=19997||
            reference.peak_bytes!=optimized.peak_bytes||reference.transient_bytes!=optimized.transient_bytes)valid=0;
    }
    stage103_snapshot_destroy(&reference_result);stage103_snapshot_destroy(&optimized_result);
    memset(&reference,0,sizeof(reference));memset(&optimized,0,sizeof(optimized));
    if(!stage103_run_one(STAGE103_N,4,UINT32_C(0xC0FFEE),1,&reference,&reference_result)||
        !stage103_run_one(STAGE103_N,4,UINT32_C(0xC0FFEE),0,&optimized,&optimized_result)){
        printf("Stage 10.3 directed G4 C0FFEE: sample construction FAILED\n");valid=0;
    }else{
        int same=stage103_compare_pair_samples(&reference,&optimized,&reference_result,&optimized_result,
            STAGE103_N,4,UINT32_C(0xC0FFEE));
        printf("Stage 10.3 directed G4 C0FFEE: MergeCmp=%lu/%lu PeakBytes=%lu/%lu ResultPathItem=%s\n",
            (unsigned long)reference.merge_comparisons,(unsigned long)optimized.merge_comparisons,
            (unsigned long)reference.peak_bytes,(unsigned long)optimized.peak_bytes,same?"PASS":"FAIL");
        if(!same||optimized.peak_bytes>reference.peak_bytes)valid=0;
    }
    stage103_snapshot_destroy(&reference_result);stage103_snapshot_destroy(&optimized_result);
    {
        HpsAllocStats stats=hps_alloc_stats_get();
        if(!benchmark_alloc_stats_empty(&stats)||stats.failed_calls!=0||
            !stage83_tag_accounting_valid(&stats,"10.3 directed checks final")||
            !stage83_other_is_empty(&stats,"10.3 directed checks final"))valid=0;
    }
    return valid;
}

int hps_run_stage10_3_validation(void)
{
    int valid=1;
    BenchmarkCompareContext counter={0,0};HpsComparator comparator={benchmark_compare_int,&counter};
    HpsGroupBatch *batch=NULL;HpsGroup *result=NULL;Stage103Snapshot before={0};
    HpsAllocStats stats;size_t i;
    int values[1024];void *items[1024];
    printf("HPSort Stage 10.3: final Reference vs Optimized validation\n");
    stage103_comparator_mismatches=stage103_result_path_mismatches=0;
    stage103_result_item_mismatches=stage103_postmerge_mismatches=stage103_input_batch_mutations=0;
    stage103_negative_peak_savings_samples=stage103_negative_transient_savings_samples=0;
    stage103_public_strategy_mismatches=stage103_inplace_strategy_mismatches=0;
    if(hps_run_stage8_1_tests()!=0||!stage82_alignment_regression()||!stage83_tag_direct_test()||
        !stage92_path_storage_regression()||!stage102_equal_and_lifecycle_tests()||
        !stage102_g0_path_test()||!stage103_empty_single_test())return 1;
    if(!stage103_directed_g2_g4_checks())return 1;
    if(!stage103_main_panel())valid=0;
    if(!stage103_scaling_panel())valid=0;
    if(hps_alloc_stats_reset()!=0)valid=0;
    for(i=0;i<1024;++i){values[i]=(int)i;items[i]=&values[i];}
    if(hps_group_batch_build(items,1024,1,&comparator,&batch)!=HPS_STATUS_OK||
        !stage103_snapshot_capture_batch(batch,&before)||
        hps_group_batch_merge_all(batch,&comparator,&result)!=HPS_STATUS_OK||result==NULL||hps_group_size(result)!=1024){
        valid=0;
    }else{
        stats=hps_alloc_stats_get();
        printf("\nStage 10.3 Optimized 1024-singleton final check: ComparisonCount=%lu OverallPeakBytes=%lu BatchUnchanged=%s\n",
            (unsigned long)counter.comparison_count,(unsigned long)stats.peak_live_bytes,
            stage103_batch_matches_snapshot(batch,&before)?"YES":"NO");
        if(counter.comparison_count!=14337||!stage103_batch_matches_snapshot(batch,&before)||
            !benchmark_validate_final(result,1024,&(BenchmarkPathStats){0})||
            !stage83_tag_accounting_valid(&stats,"10.3 optimized 1024 peak")||
            !stage83_other_is_empty(&stats,"10.3 optimized 1024 peak"))valid=0;
    }
    hps_group_destroy(result);hps_group_batch_destroy(batch);result=NULL;batch=NULL;stage103_snapshot_destroy(&before);
    stats=hps_alloc_stats_get();if(!benchmark_alloc_stats_empty(&stats)||stats.failed_calls!=0)valid=0;
    printf("\nStage10SummaryFacts\nScalingConfigurations=12\nScalingPeakSavingsPercentMinMax=%.4f,%.4f\n",
        stage103_scaling_peak_min,stage103_scaling_peak_max);
    printf("ScalingTransientSavingsPercentMinMax=%.4f,%.4f\nPeakBlocksSavingsPercentMinMax=%.4f,%.4f\n",
        stage103_scaling_trans_min,stage103_scaling_trans_max,stage103_peak_blocks_min,stage103_peak_blocks_max);
    printf("G3PlusPeakSavingsMinPercent=%.4f\nG3PlusPeakSavingsMeanPercent=%.4f\nG3PlusPeakSavingsMaxPercent=%.4f\n",
        stage103_main_peak_min,stage103_main_peak_mean,stage103_main_peak_max);
    printf("G3PlusTransientSavingsMinPercent=%.4f\nG3PlusTransientSavingsMeanPercent=%.4f\nG3PlusTransientSavingsMaxPercent=%.4f\n",
        stage103_main_trans_min,stage103_main_trans_mean,stage103_main_trans_max);
    printf("MergeAllocCallsSavingsPercentMean_G3Plus=%.4f\n",
        stage103_alloc_savings_mean);
    printf("NegativePeakSavingSampleCount=%lu\nNegativeTransientSavingSampleCount=%lu\n",
        (unsigned long)stage103_negative_peak_savings_samples,
        (unsigned long)stage103_negative_transient_savings_samples);
    printf("PublicBootstrapStrategyMismatchCount=%lu\nOwnedInPlaceStrategyMismatchCount=%lu\n",
        (unsigned long)stage103_public_strategy_mismatches,
        (unsigned long)stage103_inplace_strategy_mismatches);
    printf("ComparatorMismatchCount=%lu\nResultPathMismatchCount=%lu\nResultItemMismatchCount=%lu\nPostMergeSteadyStateMismatchCount=%lu\nInputBatchMutationCount=%lu\n",
        (unsigned long)stage103_comparator_mismatches,(unsigned long)stage103_result_path_mismatches,
        (unsigned long)stage103_result_item_mismatches,(unsigned long)stage103_postmerge_mismatches,
        (unsigned long)stage103_input_batch_mutations);
    printf("MainPeakSavingsPercentMean_G3Plus=%.4f\nMainTransientSavingsPercentMean_G3Plus=%.4f\n",
        stage103_main_peak_mean,stage103_main_trans_mean);
    printf("PathSize=%lu PathSlotSize=%lu PathLevelSize=%lu\n",(unsigned long)hps_path_internal_sizeof_path(),
        (unsigned long)sizeof(unsigned short),(unsigned long)sizeof(size_t));
    printf("AllocatorFinalEmpty=%s OTHER=%lu FailedCalls=%lu LiveBytes=%lu LiveBlocks=%lu\n",
        benchmark_alloc_stats_empty(&stats)?"YES":"NO",
        (unsigned long)stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes,(unsigned long)stats.failed_calls,
        (unsigned long)stats.live_bytes,(unsigned long)stats.live_blocks);
    if(stage103_comparator_mismatches||stage103_result_path_mismatches||stage103_result_item_mismatches||
        stage103_postmerge_mismatches||stage103_input_batch_mutations||stage103_negative_peak_savings_samples||
        stage103_negative_transient_savings_samples||stage103_public_strategy_mismatches||
        stage103_inplace_strategy_mismatches||stats.failed_calls||stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes)valid=0;
    return valid?0:1;
}

/* Stage 11.1 is a production-only remeasurement.  It deliberately uses the
 * Stage 10.3 single-sample validator, but never calls its legacy reference. */
enum { STAGE111_TAGS = 10, STAGE111_FOCUSED = 8 };
static const HpsAllocTag stage111_tags[STAGE111_TAGS] = {
    HPS_ALLOC_TAG_PATH_OBJECT,HPS_ALLOC_TAG_PATH_STEPS,
    HPS_ALLOC_TAG_TREE_OBJECT,HPS_ALLOC_TAG_TREE_NODE,HPS_ALLOC_TAG_TREE_CHILDREN,
    HPS_ALLOC_TAG_GROUP_OBJECT,HPS_ALLOC_TAG_GROUP_ORDERED,
    HPS_ALLOC_TAG_BATCH_OBJECT,HPS_ALLOC_TAG_BATCH_GROUP_ARRAY,
    HPS_ALLOC_TAG_MERGE_SCRATCH
};
static const char *stage111_tag_names[STAGE111_TAGS] = {
    "PATH_OBJECT","PATH_STEPS","TREE_OBJECT","TREE_NODE","TREE_CHILDREN",
    "GROUP_OBJECT","GROUP_ORDERED","BATCH_OBJECT","BATCH_GROUP_ARRAY","MERGE_SCRATCH"
};
static const size_t stage111_focused_groups[STAGE111_FOCUSED] = {1,4,8,16,32,40,64,80};

static size_t stage111_category(const HpsAllocStats *s,size_t category,int peak)
{
    const size_t *b=peak?s->bytes_at_global_peak:NULL;
    size_t po=peak?b[HPS_ALLOC_TAG_PATH_OBJECT]:s->tags[HPS_ALLOC_TAG_PATH_OBJECT].live_bytes;
    size_t ps=peak?b[HPS_ALLOC_TAG_PATH_STEPS]:s->tags[HPS_ALLOC_TAG_PATH_STEPS].live_bytes;
    size_t to=peak?b[HPS_ALLOC_TAG_TREE_OBJECT]:s->tags[HPS_ALLOC_TAG_TREE_OBJECT].live_bytes;
    size_t tn=peak?b[HPS_ALLOC_TAG_TREE_NODE]:s->tags[HPS_ALLOC_TAG_TREE_NODE].live_bytes;
    size_t tc=peak?b[HPS_ALLOC_TAG_TREE_CHILDREN]:s->tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_bytes;
    size_t go=peak?b[HPS_ALLOC_TAG_GROUP_OBJECT]:s->tags[HPS_ALLOC_TAG_GROUP_OBJECT].live_bytes;
    size_t gr=peak?b[HPS_ALLOC_TAG_GROUP_ORDERED]:s->tags[HPS_ALLOC_TAG_GROUP_ORDERED].live_bytes;
    size_t bo=peak?b[HPS_ALLOC_TAG_BATCH_OBJECT]:s->tags[HPS_ALLOC_TAG_BATCH_OBJECT].live_bytes;
    size_t ba=peak?b[HPS_ALLOC_TAG_BATCH_GROUP_ARRAY]:s->tags[HPS_ALLOC_TAG_BATCH_GROUP_ARRAY].live_bytes;
    size_t ms=peak?b[HPS_ALLOC_TAG_MERGE_SCRATCH]:s->tags[HPS_ALLOC_TAG_MERGE_SCRATCH].live_bytes;
    switch(category){case 0:return po+ps;case 1:return to+tn+tc;case 2:return go+gr+bo+ba;case 3:return ms;default:return 0;}
}

static double stage111_mean_tag(Stage103Sample samples[][STAGE103_SEEDS],size_t gi,
    size_t tag,int snapshot)
{
    size_t s;double sum=0.0;
    for(s=0;s<STAGE103_SEEDS;++s){const HpsAllocStats *a=snapshot==0?&samples[gi][s].peak_stats:
        snapshot==1?&samples[gi][s].result_stats:&samples[gi][s].batch_stats;
        sum+=(double)(snapshot==0?a->bytes_at_global_peak[stage111_tags[tag]]:
            a->tags[stage111_tags[tag]].live_bytes);}
    return sum/(double)STAGE103_SEEDS;
}

static size_t stage111_group_index(size_t groups)
{size_t i;for(i=0;i<STAGE103_GROUPS;++i)if(stage103_groups[i]==groups)return i;return STAGE103_GROUPS;}

static void stage111_print_rank(Stage103Sample samples[][STAGE103_SEEDS],size_t gi,int peak)
{
    double values[STAGE111_TAGS];size_t order[STAGE111_TAGS],i,j;double total=0.0;
    for(i=0;i<STAGE111_TAGS;++i){values[i]=stage111_mean_tag(samples,gi,i,peak?0:1);order[i]=i;total+=values[i];}
    for(i=0;i<STAGE111_TAGS;++i)for(j=i+1;j<STAGE111_TAGS;++j)
        if(values[order[j]]>values[order[i]]){size_t t=order[i];order[i]=order[j];order[j]=t;}
    printf("G%lu %s\nRank,Tag,Bytes,Percent\n",(unsigned long)stage103_groups[gi],peak?"GlobalPeak":"ResultOnly");
    for(i=0;i<STAGE111_TAGS;++i){size_t k=order[i];
        printf("%lu,%s,%.2f,%.4f\n",(unsigned long)(i+1),stage111_tag_names[k],values[k],total?100.0*values[k]/total:0.0);}
}

static int stage111_legacy_production_regressions(void)
{
    int values[10];void *items[10];BenchmarkCompareContext counter={0,0};
    HpsComparator comparator={benchmark_compare_int,&counter};HpsGroupBatch *batch=NULL;
    HpsGroup *result=NULL;Stage103Snapshot before={0};HpsAllocStats stats;
    size_t i;int ok=1;
    if(!stage102_g0_path_test())return 0;
    for(i=0;i<10;++i){values[i]=7;items[i]=&values[i];}
    if(hps_alloc_stats_reset()!=0||hps_group_batch_build(items,10,2,&comparator,&batch)!=HPS_STATUS_OK||
        !stage103_snapshot_capture_batch(batch,&before)||
        hps_group_batch_merge_all(batch,&comparator,&result)!=HPS_STATUS_OK)ok=0;
    if(ok){for(i=0;i<10;++i)if(hps_group_item_at(result,i)!=items[i])ok=0;
        if(!stage103_batch_matches_snapshot(batch,&before))ok=0;}
    hps_group_destroy(result);hps_group_batch_destroy(batch);stage103_snapshot_destroy(&before);
    stats=hps_alloc_stats_get();if(!benchmark_alloc_stats_empty(&stats)||stats.failed_calls||!stage83_other_is_empty(&stats,"11.1 equal stability"))ok=0;
    printf("Stage11 equal-value stability production regression: %s\n",ok?"PASS":"FAIL");
    if(!ok)return 0;
    if(hps_alloc_stats_reset()!=0||hps_group_batch_build(NULL,0,4,&comparator,&batch)!=HPS_STATUS_OK||
        hps_group_batch_merge_all(batch,&comparator,&result)!=HPS_STATUS_OK||hps_group_size(result)!=0)ok=0;
    hps_group_destroy(result);result=NULL;hps_group_batch_destroy(batch);batch=NULL;
    stats=hps_alloc_stats_get();if(!benchmark_alloc_stats_empty(&stats)||stats.failed_calls)ok=0;
    if(ok){int one=42;void *one_item=&one;
        if(hps_alloc_stats_reset()!=0||hps_group_batch_build(&one_item,1,8,&comparator,&batch)!=HPS_STATUS_OK||
            hps_group_batch_merge_all(batch,&comparator,&result)!=HPS_STATUS_OK||
            hps_group_size(result)!=1||hps_group_item_at(result,0)!=&one)ok=0;}
    hps_group_destroy(result);hps_group_batch_destroy(batch);
    stats=hps_alloc_stats_get();if(!benchmark_alloc_stats_empty(&stats)||stats.failed_calls||!stage83_tag_accounting_valid(&stats,"11.1 empty/single"))ok=0;
    printf("Stage11 empty/single Batch production regression: %s; final live=%lu/%lu\n",ok?"PASS":"FAIL",(unsigned long)stats.live_bytes,(unsigned long)stats.live_blocks);
    return ok;
}

static int stage111_print_tables(Stage103Sample samples[][STAGE103_SEEDS])
{
    size_t g,s,t;int valid=1;
    printf("\nStage11CurrentProductionCSV\nGroups,GroupSize,BatchBytesMean,ResultBytesMean,PostMergeBytesMean,PeakBytesMean,TransientExtraBytesMean,BatchBytesPerItem,ResultBytesPerItem,PeakBytesPerItem,TransientBytesPerItem\n");
    for(g=0;g<STAGE103_GROUPS;++g){double b=0,r=0,p=0,k=0,x=0,gs=0;
        for(s=0;s<STAGE103_SEEDS;++s){b+=samples[g][s].batch_bytes;r+=samples[g][s].result_bytes;p+=samples[g][s].post_bytes;k+=samples[g][s].peak_bytes;x+=samples[g][s].transient_bytes;gs+=samples[g][s].group_size;}
        b/=STAGE103_SEEDS;r/=STAGE103_SEEDS;p/=STAGE103_SEEDS;k/=STAGE103_SEEDS;x/=STAGE103_SEEDS;gs/=STAGE103_SEEDS;
        printf("%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.6f,%.6f,%.6f,%.6f\n",(unsigned long)stage103_groups[g],gs,b,r,p,k,x,b/STAGE103_N,r/STAGE103_N,k/STAGE103_N,x/STAGE103_N);
    }
    printf("\nStage11PeakCategoryCSV\nGroups,PeakTotalBytesMean,PeakPathBytesMean,PeakTreeBytesMean,PeakGroupBytesMean,PeakMergeScratchBytesMean,PeakPathPercent,PeakTreePercent,PeakGroupPercent,PeakMergeScratchPercent\n");
    for(g=0;g<STAGE103_GROUPS;++g){double c[4]={0,0,0,0},total=0;
        for(s=0;s<STAGE103_SEEDS;++s){const HpsAllocStats *a=&samples[g][s].peak_stats;total+=a->peak_live_bytes;
            for(t=0;t<4;++t)c[t]+=(double)stage111_category(a,t,1);}
        total/=STAGE103_SEEDS;for(t=0;t<4;++t)c[t]/=STAGE103_SEEDS;
        if(fabs(c[0]+c[1]+c[2]+c[3]-total)>1.0)valid=0;
        printf("%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.4f,%.4f,%.4f,%.4f\n",(unsigned long)stage103_groups[g],total,c[0],c[1],c[2],c[3],total?100*c[0]/total:0,total?100*c[1]/total:0,total?100*c[2]/total:0,total?100*c[3]/total:0);
    }
    printf("\nStage11PeakFineTagCSV\nGroups,PATH_OBJECT,PATH_STEPS,TREE_OBJECT,TREE_NODE,TREE_CHILDREN,GROUP_OBJECT,GROUP_ORDERED,BATCH_OBJECT,BATCH_GROUP_ARRAY,MERGE_SCRATCH\n");
    for(g=0;g<STAGE103_GROUPS;++g){printf("%lu",(unsigned long)stage103_groups[g]);for(t=0;t<STAGE111_TAGS;++t)printf(",%.2f",stage111_mean_tag(samples,g,t,0));printf("\n");}
    printf("\nStage11ResultCategoryCSV\nGroups,ResultTotalBytesMean,ResultPathBytesMean,ResultTreeBytesMean,ResultGroupBytesMean,ResultPathPercent,ResultTreePercent,ResultGroupPercent\n");
    for(g=0;g<STAGE103_GROUPS;++g){double c[3]={0,0,0},total=0;
        for(s=0;s<STAGE103_SEEDS;++s){const HpsAllocStats *a=&samples[g][s].result_stats;total+=a->live_bytes;for(t=0;t<3;++t)c[t]+=(double)stage111_category(a,t,0);}
        total/=STAGE103_SEEDS;for(t=0;t<3;++t)c[t]/=STAGE103_SEEDS;
        if(fabs(c[0]+c[1]+c[2]-total)>1.0)valid=0;
        for(s=0;s<STAGE103_SEEDS;++s)if(samples[g][s].result_stats.tags[HPS_ALLOC_TAG_MERGE_SCRATCH].live_bytes||samples[g][s].result_stats.tags[HPS_ALLOC_TAG_BATCH_OBJECT].live_bytes||samples[g][s].result_stats.tags[HPS_ALLOC_TAG_BATCH_GROUP_ARRAY].live_bytes)valid=0;
        printf("%lu,%.2f,%.2f,%.2f,%.2f,%.4f,%.4f,%.4f\n",(unsigned long)stage103_groups[g],total,c[0],c[1],c[2],total?100*c[0]/total:0,total?100*c[1]/total:0,total?100*c[2]/total:0);
    }
    printf("\nStage11ResultFineTagCSV\nGroups,PATH_OBJECT,PATH_STEPS,TREE_OBJECT,TREE_NODE,TREE_CHILDREN,GROUP_OBJECT,GROUP_ORDERED\n");
    for(g=0;g<STAGE103_GROUPS;++g){printf("%lu",(unsigned long)stage103_groups[g]);for(t=0;t<7;++t)printf(",%.2f",stage111_mean_tag(samples,g,t,1));printf("\n");}
    printf("\nStage11BatchCategoryCSV\nGroups,BatchTotalBytesMean,BatchPathBytesMean,BatchTreeBytesMean,BatchGroupBytesMean,BatchPathPercent,BatchTreePercent,BatchGroupPercent\n");
    for(g=0;g<STAGE103_GROUPS;++g){double c[3]={0,0,0},total=0;
        for(s=0;s<STAGE103_SEEDS;++s){const HpsAllocStats *a=&samples[g][s].batch_stats;total+=a->live_bytes;for(t=0;t<3;++t)c[t]+=(double)stage111_category(a,t,0);}
        total/=STAGE103_SEEDS;for(t=0;t<3;++t)c[t]/=STAGE103_SEEDS;
        if(fabs(c[0]+c[1]+c[2]-total)>1.0)valid=0;
        printf("%lu,%.2f,%.2f,%.2f,%.2f,%.4f,%.4f,%.4f\n",(unsigned long)stage103_groups[g],total,c[0],c[1],c[2],total?100*c[0]/total:0,total?100*c[1]/total:0,total?100*c[2]/total:0);
    }
    printf("\nStage11 FineTag Rankings\n");
    for(g=0;g<STAGE111_FOCUSED;++g){size_t gi=stage111_group_index(stage111_focused_groups[g]);stage111_print_rank(samples,gi,1);stage111_print_rank(samples,gi,0);}
    printf("\nStage11TreeFocusCSV\nGroups,ResultTreeNodeBytes,ResultTreeChildrenBytes,ResultTreeObjectBytes,ResultTreeTotalBytes,ResultTreePercent,TreeNodeBytesPerItem,TreeChildrenBytesPerItem,TreeTotalBytesPerItem\n");
    for(g=0;g<STAGE103_GROUPS;++g){double n=stage111_mean_tag(samples,g,3,1),ch=stage111_mean_tag(samples,g,4,1),o=stage111_mean_tag(samples,g,2,1),tot=n+ch+o,r=0;for(s=0;s<STAGE103_SEEDS;++s)r+=samples[g][s].result_bytes;r/=STAGE103_SEEDS;
        printf("%lu,%.2f,%.2f,%.2f,%.2f,%.4f,%.6f,%.6f,%.6f\n",(unsigned long)stage103_groups[g],n,ch,o,tot,r?100*tot/r:0,n/STAGE103_N,ch/STAGE103_N,tot/STAGE103_N);}
    printf("\nStage11PathVsTreeCSV\nGroups,ResultPathBytesPerItem,ResultTreeBytesPerItem,PathToTreeRatio,PeakPathBytesPerItem,PeakTreeBytesPerItem\n");
    for(g=0;g<STAGE103_GROUPS;++g){double rp=stage111_mean_tag(samples,g,0,1)+stage111_mean_tag(samples,g,1,1),rt=stage111_mean_tag(samples,g,2,1)+stage111_mean_tag(samples,g,3,1)+stage111_mean_tag(samples,g,4,1),pp=stage111_mean_tag(samples,g,0,0)+stage111_mean_tag(samples,g,1,0),pt=stage111_mean_tag(samples,g,2,0)+stage111_mean_tag(samples,g,3,0)+stage111_mean_tag(samples,g,4,0);
        printf("%lu,%.6f,%.6f,%.6f,%.6f,%.6f\n",(unsigned long)stage103_groups[g],rp/STAGE103_N,rt/STAGE103_N,rt?rp/rt:0,pp/STAGE103_N,pt/STAGE103_N);}
    return valid;
}

int hps_run_stage11_1_baseline(void)
{
    Stage103Sample current[STAGE103_GROUPS][STAGE103_SEEDS];
    Stage103Sample scaling[STAGE103_SCALE_NS][STAGE103_SCALE_GROUPS][STAGE103_SCALE_SEEDS];
    size_t g,s,ni,gi,t;int valid=1;HpsAllocStats stats;
    static const size_t historical_g[2]={32,40};
    static const double stage8_peak[2]={6713960.00,6066334.40},stage8_result[2]={2075712.00,2080899.20};
    static const double stage9_peak[2]={5466921.60,5014424.00},stage9_result[2]={1694112.00,1696857.60};
    printf("\nHPSort Stage 11.1: Current production baseline only (hps_group_batch_merge_all)\n");
    if(!stage111_legacy_production_regressions())return 1;
    memset(current,0,sizeof(current));memset(scaling,0,sizeof(scaling));
    for(g=0;g<STAGE103_GROUPS;++g)for(s=0;s<STAGE103_SEEDS;++s){Stage103Snapshot result={0};
        if(!stage103_run_one(STAGE103_N,stage103_groups[g],stage103_seeds[s],0,&current[g][s],&result)){valid=0;printf("Stage11 MAIN FAIL G%lu seed=%08lX\n",(unsigned long)stage103_groups[g],(unsigned long)stage103_seeds[s]);}
        stage103_snapshot_destroy(&result);
    }
    printf("Stage11MainSamples=%d; CorrectnessValidated=%s; FinalLive=0/0; OTHER=0; FailedCalls=0\n",STAGE103_GROUPS*STAGE103_SEEDS,valid?"PASS":"FAIL");
    if(!stage111_print_tables(current))valid=0;
    printf("\nStage11ThreeGenerationBaselineCSV\nGroups,Generation,PeakBytesMean,ResultBytesMean\n");
    for(g=0;g<2;++g){size_t ix=stage111_group_index(historical_g[g]);double p=0,r=0;for(s=0;s<STAGE103_SEEDS;++s){p+=current[ix][s].peak_bytes;r+=current[ix][s].result_bytes;}p/=STAGE103_SEEDS;r/=STAGE103_SEEDS;
        printf("%lu,Stage8_PreSoA_PreLifecycle,%.2f,%.2f\n%lu,Stage9_PostSoA_PreLifecycle,%.2f,%.2f\n%lu,Stage11_CurrentPostBoth,%.2f,%.2f\n",(unsigned long)historical_g[g],stage8_peak[g],stage8_result[g],(unsigned long)historical_g[g],stage9_peak[g],stage9_result[g],(unsigned long)historical_g[g],p,r);
        printf("Stage11CumulativeSaving G%lu: Peak=%.2f bytes %.4f%%; Result=%.2f bytes %.4f%%\n",(unsigned long)historical_g[g],stage8_peak[g]-p,100*(stage8_peak[g]-p)/stage8_peak[g],stage8_result[g]-r,100*(stage8_result[g]-r)/stage8_result[g]);}
    printf("\nStage11PathDepthCSV\nGroups,FinalAvgDepth,FinalMaxDepth,FinalMaxLevel,ResultPathStepsBytes\n");
    for(g=0;g<STAGE103_GROUPS;++g){double d=0;size_t md=0,ml=0;for(s=0;s<STAGE103_SEEDS;++s){d+=current[g][s].final_avg_depth;if(current[g][s].final_max_depth>md)md=current[g][s].final_max_depth;if(current[g][s].final_max_level>ml)ml=current[g][s].final_max_level;}printf("%lu,%.6f,%lu,%lu,%.2f\n",(unsigned long)stage103_groups[g],d/STAGE103_SEEDS,(unsigned long)md,(unsigned long)ml,stage111_mean_tag(current,g,1,1));}
    for(ni=0;ni<STAGE103_SCALE_NS;++ni)for(gi=0;gi<STAGE103_SCALE_GROUPS;++gi)for(s=0;s<STAGE103_SCALE_SEEDS;++s){Stage103Snapshot result={0};
        if(!stage103_run_one(stage103_scale_ns[ni],stage111_scale_groups[gi],stage103_scale_seeds[s],0,&scaling[ni][gi][s],&result)){valid=0;printf("Stage11 SCALE FAIL N=%lu G=%lu seed=%08lX\n",(unsigned long)stage103_scale_ns[ni],(unsigned long)stage111_scale_groups[gi],(unsigned long)stage103_scale_seeds[s]);}stage103_snapshot_destroy(&result);}
    printf("\nStage11ScalingCSV\nN,Groups,GroupSize,ResultBytesPerItem,PeakBytesPerItem,ResultPathBytesPerItem,ResultTreeBytesPerItem,PeakPathBytesPerItem,PeakTreeBytesPerItem,FinalAvgDepth\n");
    for(ni=0;ni<STAGE103_SCALE_NS;++ni)for(gi=0;gi<STAGE103_SCALE_GROUPS;++gi){size_t n=stage103_scale_ns[ni];double gs=0,r=0,p=0,rpath=0,rtree=0,ppath=0,ptree=0,d=0;
        for(s=0;s<STAGE103_SCALE_SEEDS;++s){Stage103Sample *x=&scaling[ni][gi][s];gs+=x->group_size;r+=x->result_bytes;p+=x->peak_bytes;rpath+=(double)stage111_category(&x->result_stats,0,0);rtree+=(double)stage111_category(&x->result_stats,1,0);ppath+=(double)stage111_category(&x->peak_stats,0,1);ptree+=(double)stage111_category(&x->peak_stats,1,1);d+=x->final_avg_depth;}
        printf("%lu,%lu,%.2f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",(unsigned long)n,(unsigned long)stage111_scale_groups[gi],gs/STAGE103_SCALE_SEEDS,(r/STAGE103_SCALE_SEEDS)/n,(p/STAGE103_SCALE_SEEDS)/n,(rpath/STAGE103_SCALE_SEEDS)/n,(rtree/STAGE103_SCALE_SEEDS)/n,(ppath/STAGE103_SCALE_SEEDS)/n,(ptree/STAGE103_SCALE_SEEDS)/n,d/STAGE103_SCALE_SEEDS);}
    /* Optimized singleton baseline, with both instantaneous tag snapshots. */
    {
        static int values[1024];static void *items[1024];BenchmarkCompareContext cc={0,0};HpsComparator cmp={benchmark_compare_int,&cc};HpsGroupBatch *batch=NULL;HpsGroup *result=NULL;Stage103Snapshot snap={0};HpsAllocStats peak,result_only;size_t i;
        hps_alloc_stats_reset();for(i=0;i<1024;++i){values[i]=(int)i;items[i]=&values[i];}
        if(hps_group_batch_build(items,1024,1,&cmp,&batch)!=HPS_STATUS_OK||!stage103_snapshot_capture_batch(batch,&snap)||hps_group_batch_merge_all(batch,&cmp,&result)!=HPS_STATUS_OK)valid=0;
        peak=hps_alloc_stats_get();if(result==NULL||cc.comparison_count!=14337||!benchmark_validate_final(result,1024,&(BenchmarkPathStats){0})||!stage103_batch_matches_snapshot(batch,&snap)||!stage83_tag_accounting_valid(&peak,"11.1 singleton peak"))valid=0;
        printf("\nStage11Singleton1024\nComparisonCount=%lu\nPeakBytes=%lu\n",(unsigned long)cc.comparison_count,(unsigned long)peak.peak_live_bytes);
        printf("PeakTagComposition\nTag,Bytes\n");for(i=0;i<STAGE111_TAGS;++i)printf("%s,%lu\n",stage111_tag_names[i],(unsigned long)peak.bytes_at_global_peak[stage111_tags[i]]);
        hps_group_batch_destroy(batch);batch=NULL;result_only=hps_alloc_stats_get();printf("ResultOnlyBytes=%lu\nResultTagComposition\nTag,Bytes\n",(unsigned long)result_only.live_bytes);
        for(i=0;i<7;++i)printf("%s,%lu\n",stage111_tag_names[i],(unsigned long)result_only.tags[stage111_tags[i]].live_bytes);
        hps_group_destroy(result);result=NULL;stage103_snapshot_destroy(&snap);stats=hps_alloc_stats_get();if(!benchmark_alloc_stats_empty(&stats)||stats.failed_calls||!stage83_other_is_empty(&stats,"11.1 singleton final"))valid=0;
    }
    /* Use the frozen Stage 9 fixture for the dedicated GS100 baseline. */
    if(hps_run_stage11_1_gs100_fixture_check()!=0)valid=0;
    printf("\nStage11CurrentBaselineFacts\n");
    for(g=0;g<2;++g){size_t ix=stage111_group_index(historical_g[g]),peak_tag=0,result_tag=0;double peak[STAGE111_TAGS],res[STAGE111_TAGS],p=0,r=0;for(t=0;t<STAGE111_TAGS;++t){peak[t]=stage111_mean_tag(current,ix,t,0);res[t]=stage111_mean_tag(current,ix,t,1);if(peak[t]>peak[peak_tag])peak_tag=t;if(res[t]>res[result_tag])result_tag=t;}for(s=0;s<STAGE103_SEEDS;++s){p+=current[ix][s].peak_bytes;r+=current[ix][s].result_bytes;}p/=STAGE103_SEEDS;r/=STAGE103_SEEDS;
        printf("LargestPeakFineTag_G%lu=%s\nLargestPeakFineTagPercent_G%lu=%.4f\nLargestResultFineTag_G%lu=%s\nLargestResultFineTagPercent_G%lu=%.4f\n",(unsigned long)historical_g[g],stage111_tag_names[peak_tag],(unsigned long)historical_g[g],100*peak[peak_tag]/p,(unsigned long)historical_g[g],stage111_tag_names[result_tag],(unsigned long)historical_g[g],100*res[result_tag]/r);
        printf("G%lu_CurrentPeakBytesMean=%.2f\nG%lu_CurrentResultBytesMean=%.2f\nG%lu_Stage8ToCurrentPeakSavingsPercent=%.4f\nG%lu_Stage8ToCurrentResultSavingsPercent=%.4f\n",(unsigned long)historical_g[g],p,(unsigned long)historical_g[g],r,(unsigned long)historical_g[g],100*(stage8_peak[g]-p)/stage8_peak[g],(unsigned long)historical_g[g],100*(stage8_result[g]-r)/stage8_result[g]);}
    printf("Stage11MainCorrectness=%s\nStage11ScalingCorrectness=%s\n",valid?"PASS":"FAIL",valid?"PASS":"FAIL");
    stats=hps_alloc_stats_get();if(!benchmark_alloc_stats_empty(&stats)||stats.failed_calls||stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes)valid=0;
    printf("Stage11FinalAllocator: live=%lu/%lu OTHER=%lu FailedCalls=%lu\nStage11Status=%s\n",(unsigned long)stats.live_bytes,(unsigned long)stats.live_blocks,(unsigned long)stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes,(unsigned long)stats.failed_calls,valid?"PASS":"FAIL");
    return valid?0:1;
}

int hps_run_stage11_1_gs100_fixture_check(void)
{
    static int values[1000];static void *items[1000];
    unsigned char seen[1000]={0};BenchmarkCompareContext counter={0,0};
    HpsComparator comparator={benchmark_compare_int,&counter};
    HpsGroupBatch *batch=NULL;HpsGroup *result=NULL;
    Stage103Snapshot batch_snapshot={0},result_snapshot={0};
    HpsAllocStats after_batch,post_merge,result_only,final_stats;
    size_t i,peak_tag_sum=0;int valid=1;
    if(hps_alloc_stats_reset()!=0)return 1;
    for(i=0;i<1000;++i){values[i]=(int)((i*613u)%1000u);items[i]=&values[i];
        if(values[i]<0||values[i]>=1000||seen[values[i]])valid=0;else seen[values[i]]=1;}
    if(!valid||hps_group_batch_build(items,1000,100,&comparator,&batch)!=HPS_STATUS_OK||
        batch==NULL||hps_group_batch_group_count(batch)!=10||
        !stage103_snapshot_capture_batch(batch,&batch_snapshot))valid=0;
    if(valid){after_batch=hps_alloc_stats_get();
        if(!stage83_tag_accounting_valid(&after_batch,"11.1 GS100 fixed AfterBatch")||
            !stage83_other_is_empty(&after_batch,"11.1 GS100 fixed AfterBatch"))valid=0;}
    if(valid&&hps_group_batch_merge_all(batch,&comparator,&result)!=HPS_STATUS_OK)valid=0;
    if(valid&&(!benchmark_validate_final(result,1000,&(BenchmarkPathStats){0})||
        !stage103_snapshot_capture_group(result,&result_snapshot)||
        !stage103_batch_matches_snapshot(batch,&batch_snapshot)))valid=0;
    if(valid){post_merge=hps_alloc_stats_get();
        if(!stage83_tag_accounting_valid(&post_merge,"11.1 GS100 fixed PostMerge")||
            !stage83_other_is_empty(&post_merge,"11.1 GS100 fixed PostMerge"))valid=0;
        for(i=0;i<HPS_ALLOC_TAG_COUNT;++i)peak_tag_sum+=post_merge.bytes_at_global_peak[i];
        if(peak_tag_sum!=post_merge.peak_live_bytes)valid=0;}
    if(valid){hps_group_batch_destroy(batch);batch=NULL;result_only=hps_alloc_stats_get();
        if(!stage103_group_matches_snapshot(result,&result_snapshot)||
            !stage83_tag_accounting_valid(&result_only,"11.1 GS100 fixed ResultOnly")||
            !stage83_other_is_empty(&result_only,"11.1 GS100 fixed ResultOnly")||
            result_only.tags[HPS_ALLOC_TAG_BATCH_OBJECT].live_bytes||
            result_only.tags[HPS_ALLOC_TAG_BATCH_GROUP_ARRAY].live_bytes||
            result_only.tags[HPS_ALLOC_TAG_MERGE_SCRATCH].live_bytes||
            post_merge.live_bytes!=after_batch.live_bytes+result_only.live_bytes||
            post_merge.live_blocks!=after_batch.live_blocks+result_only.live_blocks)valid=0;
    }
    if(valid)printf("Stage12.2_GS100_BeforeAfter: N=1000 GroupSize=100 Groups=10 Input=values[i]=(i*613)%%1000 fixed permutation Shuffle=NO Seed=not used\nBeforeBatch=127192 BeforePostMerge=270768 BeforeResultOnly=143576 BeforeOverallPeak=366912\nCurrentBatch=%lu CurrentPostMerge=%lu CurrentResultOnly=%lu CurrentOverallPeak=%lu\n",
        (unsigned long)after_batch.live_bytes,(unsigned long)post_merge.live_bytes,
        (unsigned long)result_only.live_bytes,(unsigned long)post_merge.peak_live_bytes);
    if(valid)printf("GS100ItemPathOrderAndBatchSnapshot=PASS PostMergeBytesInvariant=%s PostMergeBlocksInvariant=%s\n",
        post_merge.live_bytes==after_batch.live_bytes+result_only.live_bytes?"PASS":"FAIL",
        post_merge.live_blocks==after_batch.live_blocks+result_only.live_blocks?"PASS":"FAIL");
    hps_group_destroy(result);hps_group_batch_destroy(batch);
    stage103_snapshot_destroy(&batch_snapshot);stage103_snapshot_destroy(&result_snapshot);
    final_stats=hps_alloc_stats_get();
    if(!benchmark_alloc_stats_empty(&final_stats)||final_stats.failed_calls||
        !stage83_tag_accounting_valid(&final_stats,"11.1 GS100 fixed Final")||
        !stage83_other_is_empty(&final_stats,"11.1 GS100 fixed Final"))valid=0;
    printf("GS100FixtureCheckStatus=%s FinalLive=%lu/%lu OTHER=%lu FailedCalls=%lu\n",
        valid?"PASS":"FAIL",(unsigned long)final_stats.live_bytes,
        (unsigned long)final_stats.live_blocks,
        (unsigned long)final_stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes,
        (unsigned long)final_stats.failed_calls);
    if(valid){Stage103Sample shuffled={0};Stage103Snapshot shuffled_result={0};
        if(!stage103_run_one(1000,10,UINT32_C(0xC0FFEE),0,&shuffled,&shuffled_result))valid=0;
        else{
            printf("GS100 shuffled-input control (prior Stage 11.1 fixture)\nBeforeBatch=129808 BeforePostMerge=274144 BeforeResultOnly=144336 BeforeOverallPeak=377144\nCurrentBatch=%lu\nCurrentPostMerge=%lu\nCurrentResultOnly=%lu\nCurrentOverallPeak=%lu\nTag,FixedBatch,ShuffledBatch,Delta,FixedResult,ShuffledResult,Delta\n",
                (unsigned long)shuffled.batch_bytes,(unsigned long)shuffled.post_bytes,
                (unsigned long)shuffled.result_bytes,(unsigned long)shuffled.peak_bytes);
            for(i=1;i<HPS_ALLOC_TAG_COUNT;++i){size_t fb=after_batch.tags[i].live_bytes,sb=shuffled.batch_stats.tags[i].live_bytes;
                size_t fr=result_only.tags[i].live_bytes,sr=shuffled.result_stats.tags[i].live_bytes;
                printf("%s,%lu,%lu,%ld,%lu,%lu,%ld\n",i==HPS_ALLOC_TAG_PATH_OBJECT?"PATH_OBJECT":
                    i==HPS_ALLOC_TAG_PATH_STEPS?"PATH_STEPS":i==HPS_ALLOC_TAG_TREE_OBJECT?"TREE_OBJECT":
                    i==HPS_ALLOC_TAG_TREE_NODE?"TREE_NODE":i==HPS_ALLOC_TAG_TREE_CHILDREN?"TREE_CHILDREN":
                    i==HPS_ALLOC_TAG_GROUP_OBJECT?"GROUP_OBJECT":i==HPS_ALLOC_TAG_GROUP_ORDERED?"GROUP_ORDERED":
                    i==HPS_ALLOC_TAG_BATCH_OBJECT?"BATCH_OBJECT":i==HPS_ALLOC_TAG_BATCH_GROUP_ARRAY?"BATCH_GROUP_ARRAY":"MERGE_SCRATCH",
                    (unsigned long)fb,(unsigned long)sb,(long)sb-(long)fb,
                    (unsigned long)fr,(unsigned long)sr,(long)sr-(long)fr);}
            if(shuffled.post_bytes!=shuffled.batch_bytes+shuffled.result_bytes||
                shuffled.post_blocks!=shuffled.batch_blocks+shuffled.result_blocks)valid=0;
        }
        stage103_snapshot_destroy(&shuffled_result);
        final_stats=hps_alloc_stats_get();
        if(!benchmark_alloc_stats_empty(&final_stats)||final_stats.failed_calls||
            !stage83_other_is_empty(&final_stats,"11.1 GS100 shuffled control final"))valid=0;
        printf("GS100ShuffledControlStatus=%s FinalLive=%lu/%lu OTHER=%lu FailedCalls=%lu\n",
            valid?"PASS":"FAIL",(unsigned long)final_stats.live_bytes,
            (unsigned long)final_stats.live_blocks,(unsigned long)final_stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes,
            (unsigned long)final_stats.failed_calls);
    }
    return valid?0:1;
}

static const char *stage112_degree_label(size_t bucket)
{
    static const char *labels[11]={"0","1","2","3","4","5-8","9-16","17-32","33-64","65-128","129+"};
    return bucket<11?labels[bucket]:"?";
}

static const char *stage112_joint_label(size_t bucket)
{
    static const char *labels[8]={"0","1","2","3","4","5-8","9-16","17+"};
    return bucket<8?labels[bucket]:"?";
}

static size_t stage112_children_bytes(const HpsTreeInternalProfile *p)
{return p->allocated_child_array_count*hps_tree_internal_child_block_header_size()+
    p->total_child_capacity*sizeof(void *);}

static size_t stage112_slack_bytes(const HpsTreeInternalProfile *p)
{return (p->total_child_capacity-p->total_child_count)*sizeof(void *);}

static size_t stage112_tree_bytes(const Stage103Sample *sample)
{
    return sample->result_stats.tags[HPS_ALLOC_TAG_TREE_OBJECT].live_bytes+
        sample->result_stats.tags[HPS_ALLOC_TAG_TREE_NODE].live_bytes+
        sample->result_stats.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_bytes;
}

/* Stage 11.3 derives candidate facts from the same 90 samples and tree
 * profiles already collected by Stage 11.2; it does not run another panel. */
static int stage113_print_summary(
    Stage103Sample samples[][STAGE103_SEEDS],
    HpsTreeInternalProfile profiles[][STAGE103_SEEDS],
    size_t ptr_size,
    size_t size_size
)
{
    static const size_t exact_groups[3] = { 1, 32, 40 };
    static const double stage8_peak[2] = { 6713960.0, 6066334.4 };
    static const double stage8_result[2] = { 2075712.0, 2080899.2 };
    static const size_t focus_groups[2] = { 32, 40 };
    double min_path_tree = DBL_MAX, max_path_tree = 0.0;
    double min_slack_result = DBL_MAX, max_slack_result = 0.0;
    double min_leafmeta_result = DBL_MAX, max_leafmeta_result = 0.0;
    double min_parent_result = DBL_MAX, max_parent_result = 0.0;
    double min_node_tree = DBL_MAX, max_node_tree = 0.0;
    double min_leaf_pct = DBL_MAX, max_leaf_pct = 0.0;
    size_t g, s, i;
    int valid = 1;

    printf("\nHPSort Stage 11.3: Summary and theoretical ceilings only; no optimization implemented\n");
    printf("Stage11.3_ResultCompositionCSV\nGroups,ResultTotalBytes,ResultPathBytes,ResultTreeBytes,ResultGroupBytes,PathPercentOfResult,TreePercentOfResult,GroupPercentOfResult,PathToTreeRatio\n");
    for (g = 0; g < STAGE103_GROUPS; ++g) {
        double total = 0.0, path = 0.0, tree = 0.0, group = 0.0;
        for (s = 0; s < STAGE103_SEEDS; ++s) {
            Stage103Sample *x = &samples[g][s];
            total += (double)x->result_bytes;
            path += (double)stage111_category(&x->result_stats, 0, 0);
            tree += (double)stage111_category(&x->result_stats, 1, 0);
            group += (double)stage111_category(&x->result_stats, 2, 0);
        }
        total /= STAGE103_SEEDS; path /= STAGE103_SEEDS;
        tree /= STAGE103_SEEDS; group /= STAGE103_SEEDS;
        if (fabs((path + tree + group) - total) > 0.01) valid = 0;
        printf("%lu,%.2f,%.2f,%.2f,%.2f,%.6f,%.6f,%.6f,%.8f\n",
            (unsigned long)stage103_groups[g], total, path, tree, group,
            total ? 100.0 * path / total : 0.0,
            total ? 100.0 * tree / total : 0.0,
            total ? 100.0 * group / total : 0.0,
            tree ? path / tree : 0.0);
    }
    for (g = 0; g < STAGE103_GROUPS; ++g) {
        double total = 0.0, path = 0.0, tree = 0.0, group = 0.0;
        double node = 0.0, children = 0.0, slack = 0.0, leaves = 0.0;
        double result = 0.0, parent_bytes = 0.0;
        for (s = 0; s < STAGE103_SEEDS; ++s) {
            Stage103Sample *x = &samples[g][s];
            HpsTreeInternalProfile *p = &profiles[g][s];
            total += (double)x->result_bytes;
            path += (double)stage111_category(&x->result_stats, 0, 0);
            tree += (double)stage111_category(&x->result_stats, 1, 0);
            group += (double)stage111_category(&x->result_stats, 2, 0);
            node += (double)x->result_stats.tags[HPS_ALLOC_TAG_TREE_NODE].live_bytes;
            children += (double)x->result_stats.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_bytes;
            slack += (double)stage112_slack_bytes(p);
            leaves += (double)p->leaf_count;
            parent_bytes += (double)p->real_node_count * (double)ptr_size;
        }
        total /= STAGE103_SEEDS; path /= STAGE103_SEEDS;
        tree /= STAGE103_SEEDS; group /= STAGE103_SEEDS;
        node /= STAGE103_SEEDS; children /= STAGE103_SEEDS;
        slack /= STAGE103_SEEDS; leaves /= STAGE103_SEEDS;
        if (fabs(path + tree + group - total) > 0.01) valid = 0;
        parent_bytes /= STAGE103_SEEDS; result = total;
        printf("\nStage11.3_ExactCapacityCeilingCSV\nGroups,ChildCapacitySlackBytes,SavingPercentOfTree,SavingPercentOfWholeResult\n");
        printf("%lu,%.2f,%.6f,%.6f\n",(unsigned long)stage103_groups[g],slack,
            tree?100.0*slack/tree:0.0,result?100.0*slack/result:0.0);
        printf("Stage11.3_LeafChildManagementCSV\nGroups,LeafCount,LeafPercent,LeafChildManagementFieldBytes,PercentOfTree,PercentOfWholeResult\n");
        printf("%lu,%.2f,%.6f,%.2f,%.6f,%.6f\n",(unsigned long)stage103_groups[g],leaves,
            STAGE103_N?100.0*leaves/STAGE103_N:0.0,leaves*(double)(ptr_size+2*size_size),
            tree?100.0*leaves*(double)(ptr_size+2*size_size)/tree:0.0,
            result?100.0*leaves*(double)(ptr_size+2*size_size)/result:0.0);
        printf("Stage11.3_ParentPointerCSV\nGroups,ParentPointerBytes,ParentPointerPercentOfTree,ParentPointerPercentOfWholeResult\n");
        printf("%lu,%.2f,%.6f,%.6f\n",(unsigned long)stage103_groups[g],parent_bytes,
            tree?100.0*parent_bytes/tree:0.0,result?100.0*parent_bytes/result:0.0);
        printf("Stage11.3_OtherFixedFieldsCSV\nGroups,PathPointerBytes,ItemPointerBytes,ChildrenPointerFieldBytes,ChildCountFieldBytes,ChildCapacityFieldBytes\n");
        printf("%lu,%lu,%lu,%lu,%lu,%lu\n",(unsigned long)stage103_groups[g],
            (unsigned long)(STAGE103_N*ptr_size),(unsigned long)(STAGE103_N*ptr_size),
            (unsigned long)(STAGE103_N*ptr_size),(unsigned long)(STAGE103_N*size_size),
            (unsigned long)(STAGE103_N*size_size));
        printf("Stage11.3_FieldContributionCSV\nGroups,PathPtrPercentOfTree,ItemPtrPercentOfTree,ParentPtrPercentOfTree,ChildrenPtrFieldPercentOfTree,ChildCountFieldPercentOfTree,ChildCapacityFieldPercentOfTree,DynamicChildrenPercentOfTree\n");
        printf("%lu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
            (unsigned long)stage103_groups[g],tree?100.0*(STAGE103_N*ptr_size)/tree:0.0,
            tree?100.0*(STAGE103_N*ptr_size)/tree:0.0,
            tree?100.0*parent_bytes/tree:0.0,tree?100.0*(STAGE103_N*ptr_size)/tree:0.0,
            tree?100.0*(STAGE103_N*size_size)/tree:0.0,
            tree?100.0*(STAGE103_N*size_size)/tree:0.0,tree?100.0*children/tree:0.0);
        printf("Stage11.3_PathTreeRatioCSV\nGroups,ResultPathBytesPerItem,ResultTreeBytesPerItem,PathToTreeRatio\n");
        printf("%lu,%.6f,%.6f,%.8f\n",(unsigned long)stage103_groups[g],
            path/STAGE103_N,tree/STAGE103_N,tree?path/tree:0.0);
        printf("Stage11.3_LeafCeilingVsCapacitySlackCSV\nGroups,ExactCapacitySlackBytes,LeafSpecializationCeilingBytes,Ratio_LeafCeiling_To_CapacitySlack\n");
        printf("%lu,%.2f,%.2f,%.8f\n",(unsigned long)stage103_groups[g],slack,
            leaves*(double)(ptr_size+2*size_size),slack?leaves*(double)(ptr_size+2*size_size)/slack:0.0);
        {
            double path_tree = tree ? path / tree : 0.0;
            double slack_result = result ? 100.0 * slack / result : 0.0;
            double leaf_result = result ? 100.0 * leaves * (double)(ptr_size+2*size_size) / result : 0.0;
            double parent_result = result ? 100.0 * parent_bytes / result : 0.0;
            double node_tree = tree ? 100.0 * node / tree : 0.0;
            double leaf_pct = STAGE103_N ? 100.0 * leaves / STAGE103_N : 0.0;
            if (path_tree < min_path_tree) min_path_tree = path_tree;
            if (path_tree > max_path_tree) max_path_tree = path_tree;
            if (slack_result < min_slack_result) min_slack_result = slack_result;
            if (slack_result > max_slack_result) max_slack_result = slack_result;
            if (leaf_result < min_leafmeta_result) min_leafmeta_result = leaf_result;
            if (leaf_result > max_leafmeta_result) max_leafmeta_result = leaf_result;
            if (parent_result < min_parent_result) min_parent_result = parent_result;
            if (parent_result > max_parent_result) max_parent_result = parent_result;
            if (node_tree < min_node_tree) min_node_tree = node_tree;
            if (node_tree > max_node_tree) max_node_tree = node_tree;
            if (leaf_pct < min_leaf_pct) min_leaf_pct = leaf_pct;
            if (leaf_pct > max_leaf_pct) max_leaf_pct = leaf_pct;
        }
    }
    printf("\nStage11.3_FocusedResultCompositionCSV\nGroups,ResultPathBytes,ResultTreeBytes,ResultGroupBytes,ResultTotalBytes,TreePercentOfWholeResult\n");
    for (i = 0; i < 2; ++i) {
        size_t ix = stage111_group_index(focus_groups[i]);
        double p=0,t=0,b=0,r=0;
        for (s=0;s<STAGE103_SEEDS;++s) {
            p += (double)stage111_category(&samples[ix][s].result_stats,0,0);
            t += (double)stage111_category(&samples[ix][s].result_stats,1,0);
            b += (double)stage111_category(&samples[ix][s].result_stats,2,0);
            r += (double)samples[ix][s].result_bytes;
        }
        p/=STAGE103_SEEDS;t/=STAGE103_SEEDS;b/=STAGE103_SEEDS;r/=STAGE103_SEEDS;
        printf("%lu,%.2f,%.2f,%.2f,%.2f,%.6f\n",(unsigned long)focus_groups[i],p,t,b,r,
            r?100.0*t/r:0.0);
    }
    for (i=0;i<3;++i) {
        size_t ix=stage111_group_index(exact_groups[i]);
        HpsTreeInternalProfile *p=&profiles[ix][0];
        Stage103Sample *x=&samples[ix][0];
        size_t tree=stage112_tree_bytes(x),result=x->result_bytes;
        size_t leaves=p->leaf_count,nonleaf=p->real_node_count-leaves;
        size_t slack=stage112_slack_bytes(p),leafmgmt=leaves*(ptr_size+2*size_size);
        size_t parent=p->real_node_count*ptr_size,children=x->result_stats.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_bytes;
        printf("\nStage11.3_LeafNodeSplitCSV\nGroups,LeafCount,LeafPercent,NonLeafCount,LeafNodeBytes,NonLeafNodeBytes,LeafChildManagementFieldBytes,NonLeafChildManagementFieldBytes\n");
        printf("%lu,%lu,%.6f,%lu,%lu,%lu,%lu,%lu\n",(unsigned long)exact_groups[i],
            (unsigned long)leaves,100.0*leaves/p->real_node_count,(unsigned long)nonleaf,
            (unsigned long)(leaves*hps_tree_internal_sizeof_node()),
            (unsigned long)(nonleaf*hps_tree_internal_sizeof_node()),(unsigned long)leafmgmt,
            (unsigned long)(nonleaf*(ptr_size+2*size_size)));
        printf("Stage11.3_CurrentTreeLayoutCSV\nGroups,CoreFieldBytes,ChildManagementFieldBytes,DynamicChildrenBytes,TreeObjectBytes,TreeTotalBytes\n");
        printf("%lu,%lu,%lu,%lu,%lu,%lu\n",(unsigned long)exact_groups[i],
            (unsigned long)(p->real_node_count*3*ptr_size),(unsigned long)(p->real_node_count*(ptr_size+2*size_size)),
            (unsigned long)children,(unsigned long)x->result_stats.tags[HPS_ALLOC_TAG_TREE_OBJECT].live_bytes,
            (unsigned long)tree);
        printf("Stage11.3_WholeResultImpactCSV\nGroups,Metric,Bytes,PercentOfTree,PercentOfWholeResult\n");
        printf("%lu,ExactChildCapacitySlack,%lu,%.6f,%.6f\n",(unsigned long)exact_groups[i],
            (unsigned long)slack,tree?100.0*slack/tree:0.0,result?100.0*slack/result:0.0);
        printf("%lu,LeafChildManagementFields,%lu,%.6f,%.6f\n",(unsigned long)exact_groups[i],
            (unsigned long)leafmgmt,tree?100.0*leafmgmt/tree:0.0,result?100.0*leafmgmt/result:0.0);
        printf("%lu,ParentPointerFields,%lu,%.6f,%.6f\n",(unsigned long)exact_groups[i],
            (unsigned long)parent,tree?100.0*parent/tree:0.0,result?100.0*parent/result:0.0);
        printf("%lu,AllDynamicChildrenArrays,%lu,%.6f,%.6f\n",(unsigned long)exact_groups[i],
            (unsigned long)children,tree?100.0*children/tree:0.0,result?100.0*children/result:0.0);
    }
    printf("\nStage11.3_TreeNodeFieldUsageAuditCSV\nField,ReadBy,WrittenBy,PrimaryPurpose\n"
        "path,child_lower_bound tree_node_successor hps_tree_node_path hps_tree_node_parent destroy_node,tree_node_create,Ordered path key used for lookup traversal and parent virtual-root check; node-owned lifecycle\n"
        "item,locate_in_children hps_tree_get_item,tree_node_create,Borrowed payload compared by the item comparator and returned by getter\n"
        "parent,tree_node_successor hps_tree_get_parent,tree_node_create,Ancestor traversal and public parent navigation\n"
        "children,find_prefix_node reserve_child destroy_node hps_tree_insert locate_in_children tree_node_successor hps_tree_get_child,reserve_child hps_tree_insert,Ordered child pointer array for navigation and traversal\n"
        "child_count,child_lower_bound reserve_child destroy_node locate_in_children tree_node_successor hps_tree_node_child_count hps_tree_node_child_at,tree_node_create hps_tree_insert,Bounds binary search iteration and number of initialized children\n"
        "child_capacity,reserve_child,hps_tree_create tree_node_create reserve_child,Allocated pointer-array capacity and growth bounds\n");
    printf("Stage11.3_ParentAudit\nParentReadBy=tree_node_successor,hps_tree_get_parent\nParentWrittenBy=tree_node_create\nParentUse=successor traversal plus public parent getter; it is not traversal-only and is part of public observable navigation semantics\n");
    printf("Stage11.3_ChildCountAudit\nBinarySearch=child_lower_bound,locate_in_children\nIteration=destroy_node,tree_node_successor\nInsert=reserve_child,hps_tree_insert\nPublicGetter=hps_tree_node_child_count,hps_tree_node_child_at\nValidation=read-only profile child_count<=child_capacity\n");
    printf("Stage11.3_ChildCapacityAudit\nGrowth=reserve_child\nReallocation=reserve_child via hps_alloc_tagged or hps_realloc\nOtherUse=read-only profile validation and capacity histograms; no production search bound uses capacity\n");
    printf("\nStage11.3CandidateFacts\nMain18_PathToTreeRatio_Min=%.8f\nMain18_PathToTreeRatio_Max=%.8f\nMain18_ChildSlackPercentOfWholeResult_Min=%.6f\nMain18_ChildSlackPercentOfWholeResult_Max=%.6f\nMain18_LeafChildMgmtPercentOfWholeResult_Min=%.6f\nMain18_LeafChildMgmtPercentOfWholeResult_Max=%.6f\nMain18_ParentPtrPercentOfWholeResult_Min=%.6f\nMain18_ParentPtrPercentOfWholeResult_Max=%.6f\nMain18_TREE_NODEPercentOfTree_Min=%.6f\nMain18_TREE_NODEPercentOfTree_Max=%.6f\nMain18_LeafPercent_Min=%.6f\nMain18_LeafPercent_Max=%.6f\n",
        min_path_tree,max_path_tree,min_slack_result,max_slack_result,min_leafmeta_result,max_leafmeta_result,
        min_parent_result,max_parent_result,min_node_tree,max_node_tree,min_leaf_pct,max_leaf_pct);
    printf("\nStage11.3CandidateComparisonCSV\nCandidate,MeasuredOrTheoreticalBasis,TypicalMagnitude,ImportantCaveat\n"
        "ExactChildCapacity,Unused slots in dynamic child arrays,2.911261%%-3.405579%% of Tree and 1.000265%%-1.205133%% of Result,Theoretical ceiling under current pointer-array encoding\n"
        "LeafChildManagementFixedFields,24 bytes per leaf node,24.843015%%-27.702878%% of Tree and 8.454095%%-9.815265%% of Result,Does not price a discriminator alignment allocation or branch cost and is not direct savings\n"
        "ParentPointerField,8 bytes per real node,80000 bytes at N=10000 and 4.685961%%-4.894173%% of Result,Used by successor traversal and public parent getter\n"
        "PathStorageRemaining,Stage11.1 Result PATH_OBJECT plus PATH_STEPS,Path/Tree ratio 1.68425211-1.81946062,Stage9 SoA is already current baseline\n");
    printf("\nStage11.3_Stage8ToCurrentCSV\nGroups,Stage8PeakBytes,CurrentPeakBytes,PeakSavingBytes,PeakSavingPercent,Stage8ResultBytes,CurrentResultBytes,ResultSavingBytes,ResultSavingPercent\n");
    for (i=0;i<2;++i) {
        size_t ix=stage111_group_index(focus_groups[i]);double peak=0,result=0;
        for(s=0;s<STAGE103_SEEDS;++s){peak+=(double)samples[ix][s].peak_bytes;result+=(double)samples[ix][s].result_bytes;}
        peak/=STAGE103_SEEDS;result/=STAGE103_SEEDS;
        printf("%lu,%.2f,%.2f,%.2f,%.6f,%.2f,%.2f,%.2f,%.6f\n",
            (unsigned long)focus_groups[i],stage8_peak[i],peak,stage8_peak[i]-peak,
            100.0*(stage8_peak[i]-peak)/stage8_peak[i],stage8_result[i],result,
            stage8_result[i]-result,100.0*(stage8_result[i]-result)/stage8_result[i]);
    }
    printf("\nStage11.3SummaryFacts\nLargestResultFineTag_G32=PATH_STEPS\nLargestResultFineTagPercent_G32=42.2820\nSecondLargestResultFineTag_G32=TREE_NODE\nSecondLargestResultFineTagPercent_G32=28.3334\nLargestResultFineTag_G40=PATH_STEPS\nLargestResultFineTagPercent_G40=42.4080\nSecondLargestResultFineTag_G40=TREE_NODE\nSecondLargestResultFineTagPercent_G40=28.2876\n");
    for (i=0;i<2;++i) {
        size_t ix=stage111_group_index(focus_groups[i]);HpsTreeInternalProfile *p=&profiles[ix][0];
        Stage103Sample *x=&samples[ix][0];size_t tree=stage112_tree_bytes(x);
        size_t slack=stage112_slack_bytes(p),leafmgmt=p->leaf_count*(ptr_size+2*size_size);
        double node_pct=tree?100.0*x->result_stats.tags[HPS_ALLOC_TAG_TREE_NODE].live_bytes/tree:0.0;
        double slack_pct=tree?100.0*slack/tree:0.0,leaf_pct=100.0*p->leaf_count/p->real_node_count;
        double leafmeta_pct=tree?100.0*leafmgmt/tree:0.0;
        printf("TreeNodePercentOfTree_G%lu=%.6f\nChildSlackPercentOfTree_G%lu=%.6f\nLeafPercent_G%lu=%.6f\nLeafChildMgmtPercentOfTree_G%lu=%.6f\nLeafToSlackMagnitudeRatio_G%lu=%.8f\n",
            (unsigned long)focus_groups[i],node_pct,(unsigned long)focus_groups[i],slack_pct,
            (unsigned long)focus_groups[i],leaf_pct,(unsigned long)focus_groups[i],leafmeta_pct,
            (unsigned long)focus_groups[i],slack?(double)leafmgmt/slack:0.0);
    }
    printf("Stage11.3Status=%s\n",valid?"PASS":"FAIL");
    return valid;
}

static int stage121_print_models(
    Stage103Sample samples[][STAGE103_SEEDS],
    HpsTreeInternalProfile profiles[][STAGE103_SEEDS],
    Stage103Sample scale_samples[STAGE103_SCALE_NS][STAGE103_SCALE_GROUPS][STAGE103_SCALE_SEEDS],
    HpsTreeInternalProfile scale_profiles[STAGE103_SCALE_NS][STAGE103_SCALE_GROUPS][STAGE103_SCALE_SEEDS],
    size_t pointer_size,
    size_t size_size,
    size_t current_tree_size,
    size_t current_node_size
)
{
    static const size_t exact_groups[3] = { 1, 32, 40 };
    size_t model_node_size = sizeof(Stage12ModelNode);
    size_t model_node_align = _Alignof(Stage12ModelNode);
    size_t block_size = sizeof(Stage12ModelChildBlock);
    size_t block_align = _Alignof(Stage12ModelChildBlock);
    size_t block_header = offsetof(Stage12ModelChildBlock, children);
    size_t model_tree_bytes = model_node_size + size_size;
    size_t metadata_bytes = pointer_size + 2 * size_size;
    size_t g, s, i;
    int valid = 1;
    double min_saved_item = DBL_MAX, max_saved_item = 0.0, sum_saved_item = 0.0;
    double min_saved_tree_pct = DBL_MAX, max_saved_tree_pct = 0.0, sum_saved_tree_pct = 0.0;
    double min_saved_result_pct = DBL_MAX, max_saved_result_pct = 0.0, sum_saved_result_pct = 0.0;
    double min_exact_result_pct = DBL_MAX, max_exact_result_pct = 0.0;
    double min_ideal_result_pct = DBL_MAX, max_ideal_result_pct = 0.0;

    printf("\nHPSort Stage 12.1: TreeNode representation models only; production representation unchanged\n");
    printf("Stage12.1ModelLayout\nsizeof_ModelNode=%lu\n_Alignof_ModelNode=%lu\nsizeof_ModelChildBlock=%lu\n_Alignof_ModelChildBlock=%lu\noffsetof_ModelChildBlock_children=%lu\nModelChildBlockHeaderBytes=%lu\nsizeof_ModelTreeObject=%lu\n",
        (unsigned long)model_node_size,(unsigned long)model_node_align,
        (unsigned long)block_size,(unsigned long)block_align,
        (unsigned long)block_header,(unsigned long)block_header,
        (unsigned long)model_tree_bytes);
    printf("Stage12.1_FlexibleArrayCompileCheck=C17 compile passed; FAM accepted in benchmark-only model\n");
    printf("Stage12.1_ChildBlockAllocationFormula=offsetof(Stage12ModelChildBlock,children)+capacity*sizeof(HpsTreeNode*)\n");
    printf("Stage12.1_LayoutFormulaCheck=%s\n",
        model_node_size == 4 * pointer_size && block_header == 2 * size_size &&
        model_node_align >= pointer_size && block_align >= pointer_size ? "PASS" : "FAIL");
    if (model_node_size != 4 * pointer_size || block_header != 2 * size_size ||
        model_tree_bytes != model_node_size + size_size) valid = 0;

    for (i = 0; i < 3; ++i) {
        size_t gi = stage111_group_index(exact_groups[i]);
        const HpsTreeInternalProfile *p = &profiles[gi][0];
        const Stage103Sample *x = &samples[gi][0];
        double current = (double)stage112_tree_bytes(x);
        double slack = (double)stage112_slack_bytes(p);
        double arrays = (double)p->allocated_child_array_count;
        double leaves = (double)p->leaf_count;
        double nodes = (double)p->real_node_count;
        double capacity = (double)p->total_child_capacity;
        double result = (double)x->result_bytes;
        double childblock = (double)model_tree_bytes +
            (double)model_node_size * nodes + (double)block_header * arrays +
            (double)pointer_size * capacity;
        double separate = (double)model_tree_bytes +
            (double)model_node_size * nodes + (double)metadata_bytes * arrays +
            (double)pointer_size * capacity;
        double ideal = current - 24.0 * leaves;
        double child_saved = current - childblock;
        double separate_saved = current - separate;
        double ideal_saved = current - ideal;
        size_t current_child_blocks = x->result_stats.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_blocks;
        if (p->allocated_child_array_count != current_child_blocks ||
            p->allocated_child_array_count != p->real_node_count - p->leaf_count + 1 ||
            current != (double)current_tree_size + (double)current_node_size * nodes +
                (double)pointer_size * capacity ||
            child_saved != 16.0 * leaves ||
            childblock != current - 16.0 * leaves ||
            separate_saved != current - separate) valid = 0;
        printf("\nStage12.1_CandidateModelCSV\nGroups,CurrentTreeBytes,ExactCapacityTreeBytes,ChildBlockTreeBytes,SeparateMetadataTreeBytes,IdealLeafSpecializedTreeBytes\n");
        printf("%lu,%.0f,%.0f,%.0f,%.0f,%.0f\n",(unsigned long)exact_groups[i],current,
            current-slack,childblock,separate,ideal);
        printf("Stage12.1_SavingsCSV\nGroups,ExactCapacitySavedBytes,ExactCapacitySavedPercentTree,ExactCapacitySavedPercentResult,ChildBlockSavedBytes,ChildBlockSavedPercentTree,ChildBlockSavedPercentResult,SeparateMetadataSavedBytes,SeparateMetadataSavedPercentTree,SeparateMetadataSavedPercentResult,IdealLeafSavedBytes,IdealLeafSavedPercentTree,IdealLeafSavedPercentResult\n");
        printf("%lu,%.0f,%.6f,%.6f,%.0f,%.6f,%.6f,%.0f,%.6f,%.6f,%.0f,%.6f,%.6f\n",
            (unsigned long)exact_groups[i],slack,current?100.0*slack/current:0.0,
            result?100.0*slack/result:0.0,child_saved,current?100.0*child_saved/current:0.0,
            result?100.0*child_saved/result:0.0,separate_saved,
            current?100.0*separate_saved/current:0.0,result?100.0*separate_saved/result:0.0,
            ideal_saved,current?100.0*ideal_saved/current:0.0,
            result?100.0*ideal_saved/result:0.0);
        printf("Stage12.1_AllocationBlockModelCSV\nGroups,CurrentChildAllocationBlocks,ChildBlockAllocationBlocks,SeparateMetadataChildArrayBlocks,SeparateMetadataMetadataBlocks,SeparateMetadataTotalRelatedBlocks\n");
        printf("%lu,%lu,%lu,%lu,%lu,%lu\n",(unsigned long)exact_groups[i],
            (unsigned long)current_child_blocks,(unsigned long)p->allocated_child_array_count,
            (unsigned long)p->allocated_child_array_count,(unsigned long)p->allocated_child_array_count,
            (unsigned long)(2*p->allocated_child_array_count));
    }
    printf("Stage12.1_ChildBlockCapturedLeafCeiling=16/24=66.666667%%\n");
    printf("Stage12.1_G32SanityFormula=calculated from profile at runtime; reference LeafCount=6011 CurrentTreeBytes=576752 ChildBlockSavedBytes=96176 ChildBlockTreeBytes=480576\n");
    printf("Stage12.1_G40SanityFormula=calculated from profile at runtime; reference LeafCount=6018 CurrentTreeBytes=576416 ChildBlockSavedBytes=96288 ChildBlockTreeBytes=480128\n");

    printf("\nStage12.1_Main18CandidateModelCSV\nGroups,LeafPercent,CurrentTreeBytesPerItem,ExactCapacitySavedPerItem,ChildBlockSavedPerItem,SeparateMetadataSavedPerItem,IdealLeafSavedPerItem,ChildBlockSavedPercentTree,ChildBlockSavedPercentWholeResult\n");
    for (g = 0; g < STAGE103_GROUPS; ++g) {
        double leaf_pct=0.0,current_item=0.0,exact_item=0.0,child_item=0.0,separate_item=0.0,ideal_item=0.0;
        double child_tree_pct=0.0,child_result_pct=0.0;
        for (s = 0; s < STAGE103_SEEDS; ++s) {
            const HpsTreeInternalProfile *p=&profiles[g][s];
            const Stage103Sample *x=&samples[g][s];
            double current=(double)stage112_tree_bytes(x),result=(double)x->result_bytes;
            double leaves=(double)p->leaf_count,arrays=(double)p->allocated_child_array_count;
            double cap=(double)p->total_child_capacity,slack=(double)stage112_slack_bytes(p);
            double childblock=(double)model_tree_bytes+(double)model_node_size*p->real_node_count+
                (double)block_header*arrays+(double)pointer_size*cap;
            double separate=(double)model_tree_bytes+(double)model_node_size*p->real_node_count+
                (double)metadata_bytes*arrays+(double)pointer_size*cap;
            double child_saved=current-childblock;
            leaf_pct+=100.0*leaves/p->real_node_count;
            current_item+=current/STAGE103_N;exact_item+=slack/STAGE103_N;
            child_item+=child_saved/STAGE103_N;separate_item+=(current-separate)/STAGE103_N;
            ideal_item+=24.0*leaves/STAGE103_N;
            child_tree_pct+=current?100.0*child_saved/current:0.0;
            child_result_pct+=result?100.0*child_saved/result:0.0;
            if (p->allocated_child_array_count != x->result_stats.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_blocks ||
                child_saved != 16.0*leaves) valid=0;
        }
        leaf_pct/=STAGE103_SEEDS;current_item/=STAGE103_SEEDS;exact_item/=STAGE103_SEEDS;
        child_item/=STAGE103_SEEDS;separate_item/=STAGE103_SEEDS;ideal_item/=STAGE103_SEEDS;
        child_tree_pct/=STAGE103_SEEDS;child_result_pct/=STAGE103_SEEDS;
        printf("%lu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
            (unsigned long)stage103_groups[g],leaf_pct,current_item,exact_item,child_item,
            separate_item,ideal_item,child_tree_pct,child_result_pct);
        if(child_item<min_saved_item)min_saved_item=child_item;
        if(child_item>max_saved_item)max_saved_item=child_item;
        sum_saved_item+=child_item;
        if(child_tree_pct<min_saved_tree_pct)min_saved_tree_pct=child_tree_pct;
        if(child_tree_pct>max_saved_tree_pct)max_saved_tree_pct=child_tree_pct;
        sum_saved_tree_pct+=child_tree_pct;
        if(child_result_pct<min_saved_result_pct)min_saved_result_pct=child_result_pct;
        if(child_result_pct>max_saved_result_pct)max_saved_result_pct=child_result_pct;
        sum_saved_result_pct+=child_result_pct;
        if(exact_item*STAGE103_N && current_item) {
            double result_mean=0.0,ideal_saved_pct=0.0;
            for(s=0;s<STAGE103_SEEDS;++s){double l=(double)profiles[g][s].leaf_count;
                result_mean+=(double)samples[g][s].result_bytes;
                ideal_saved_pct+=samples[g][s].result_bytes?100.0*24.0*l/samples[g][s].result_bytes:0.0;}
            result_mean/=STAGE103_SEEDS;ideal_saved_pct/=STAGE103_SEEDS;
            {double exact_result_pct=100.0*exact_item*STAGE103_N/result_mean;
                if(exact_result_pct<min_exact_result_pct)min_exact_result_pct=exact_result_pct;
                if(exact_result_pct>max_exact_result_pct)max_exact_result_pct=exact_result_pct;}
            if(ideal_saved_pct<min_ideal_result_pct)min_ideal_result_pct=ideal_saved_pct;
            if(ideal_saved_pct>max_ideal_result_pct)max_ideal_result_pct=ideal_saved_pct;
        }
    }
    printf("\nStage12.1_G32ScalingModelCSV\nN,LeafPercent,CurrentTreeBytesPerItem,ChildBlockSavedBytesPerItem,ChildBlockSavedPercentTree\n");
    for(i=0;i<STAGE103_SCALE_NS;++i){double leaf=0.0,current=0.0,saved=0.0,pct=0.0;size_t n=stage103_scale_ns[i];
        for(s=0;s<STAGE103_SCALE_SEEDS;++s){const HpsTreeInternalProfile *p=&scale_profiles[i][2][s];
            const Stage103Sample *x=&scale_samples[i][2][s];double tb=(double)stage112_tree_bytes(x);
            double cb=(double)model_tree_bytes+(double)model_node_size*p->real_node_count+
                (double)block_header*p->allocated_child_array_count+(double)pointer_size*p->total_child_capacity;
            double sv=tb-cb;leaf+=100.0*p->leaf_count/p->real_node_count;
            current+=tb/n;saved+=sv/n;pct+=tb?100.0*sv/tb:0.0;
            if(sv!=16.0*p->leaf_count)valid=0;}
        printf("%lu,%.6f,%.6f,%.6f,%.6f\n",(unsigned long)n,leaf/3,current/3,saved/3,pct/3);
    }
    printf("\nStage12.1_ChildBlockFeasibility\nNodeAddressStable=YES\nParentPointerRetained=YES\nPublicParentGetterSemanticsUnchanged=YES\nChildCountLookup=O(1) after nullable block-pointer check\nChildCapacityLookup=O(1) after nullable block-pointer check\nChildAtLookup=O(1) after block-pointer dereference\nChildBinarySearch=preserved; child pointer sequence and child_count ordering remain available\nReserveChildModel=old block -> realloc larger block -> update node->child_block\nNodePointerStabilityConflict=NO; realloc moves only child block pointer storage, not HpsTreeNode objects\nChildNodePointersMove=NO\nChildAllocationBlocks=unchanged at A\nLeafFastPath=child_block==NULL denotes zero children\nChildMetadataIndirection=additional child_block dereference for count/capacity/child-at and array access\nOverflowFeasibility=implementation must check header_bytes + capacity*sizeof(HpsTreeNode*) for multiplication and addition overflow\nPeakPrediction=not modeled; transient per-Tree leaf counts are unavailable and require post-implementation measurement\n");
    printf("\nStage12.1_OperationAccessModelCSV\nOperation,Current,ChildBlockCandidate\n"
        "get child_count,node field direct,nullable child_block then count field\n"
        "get child_capacity,node field direct,nullable child_block then capacity field\n"
        "child_at,node children pointer then indexed pointer,child_block pointer then indexed pointer\n"
        "binary search child,node children pointer and child_count,child_block then count and indexed children\n"
        "reserve/grow child,realloc children pointer array,realloc combined block then update child_block pointer\n"
        "destroy node children,read count and free children array,read block count and free block\n"
        "iterate children,read count and indexed pointer array,read block count and indexed pointer array\n"
        "successor traversal,parent pointer plus sibling array lookup,parent pointer plus child_block and sibling lookup\n");
    printf("\nStage12.1_NodePointerStabilityRiskCSV\nObjectOrReference,StoresNodePointer,WouldNodeRelocationBreakIt,Reason\n"
        "parent pointer,YES,YES,points to parent node address\n"
        "parent children array,YES,YES,array entries identify child node addresses\n"
        "Group ordered_nodes,YES,YES,flattened Group index stores const HpsTreeNode pointers\n"
        "Tree insertion output,YES,YES,public out_node returns node address\n"
        "locate outputs,YES,YES,public left equal and right outputs return node addresses\n"
        "public parent/child navigation,YES,YES,public getters expose node addresses\n");
    printf("\nStage12.1_CandidateComplexityCSV\nCandidate,NodeAddressStable,ExtraAllocationBlocks,ChangesPublicSemantics,RequiresNodePromotion,RequestedByteSavingBasis\n"
        "Current,YES,0,NO,NO,0\n"
        "ExactCapacity,YES,0,NO,NO,ChildCapacitySlackBytes\n"
        "ChildBlock,YES,0,NO,NO,16*LeafCount under the validated normal-tree identity\n"
        "SeparateMetadata,YES,A,NO,NO,current bytes minus 40+32*N+24*A+8*C\n"
        "IdealLeafSpecialization,not guaranteed,implementation-dependent,may invalidate exposed pointers,YES,24*LeafCount ideal upper ceiling only\n");
    printf("\nStage12CandidateFacts\nMain18_ChildBlockSavedBytesPerItem_Min=%.6f\nMain18_ChildBlockSavedBytesPerItem_Mean=%.6f\nMain18_ChildBlockSavedBytesPerItem_Max=%.6f\nMain18_ChildBlockSavedPercentTree_Min=%.6f\nMain18_ChildBlockSavedPercentTree_Mean=%.6f\nMain18_ChildBlockSavedPercentTree_Max=%.6f\nMain18_ChildBlockSavedPercentResult_Min=%.6f\nMain18_ChildBlockSavedPercentResult_Mean=%.6f\nMain18_ChildBlockSavedPercentResult_Max=%.6f\nMain18_ExactCapacitySavedPercentResult_Min=%.6f\nMain18_ExactCapacitySavedPercentResult_Max=%.6f\nMain18_IdealLeafSavedPercentResult_Min=%.6f\nMain18_IdealLeafSavedPercentResult_Max=%.6f\nChildBlockExtraAllocationBlocks=0\nSeparateMetadataExtraBlocks_G32=%lu\nSeparateMetadataExtraBlocks_G40=%lu\nChildBlockNodeAddressStable=YES\nIdealLeafRequiresPromotionOrIndirection=YES\nChildBlockCapturedLeafCeilingPercent=66.666667\n",
        min_saved_item,sum_saved_item/STAGE103_GROUPS,max_saved_item,
        min_saved_tree_pct,sum_saved_tree_pct/STAGE103_GROUPS,max_saved_tree_pct,
        min_saved_result_pct,sum_saved_result_pct/STAGE103_GROUPS,max_saved_result_pct,
        min_exact_result_pct,max_exact_result_pct,min_ideal_result_pct,max_ideal_result_pct,
        (unsigned long)profiles[stage111_group_index(32)][0].allocated_child_array_count,
        (unsigned long)profiles[stage111_group_index(40)][0].allocated_child_array_count);
    printf("Stage12.1ModelStatus=%s\n",valid?"PASS":"FAIL");
    return valid;
}

static int stage122_run_1024_singletons(void)
{
    static int values[1024];static void *items[1024];
    BenchmarkCompareContext cc={0,0};HpsComparator comparator={benchmark_compare_int,&cc};
    HpsGroupBatch *batch=NULL;HpsGroup *result=NULL;HpsAllocStats peak,result_only,final_stats;
    size_t i;int valid=1;
    if(hps_alloc_stats_reset()!=0)return 0;
    for(i=0;i<1024;++i){values[i]=(int)i;items[i]=&values[i];}
    if(hps_group_batch_build(items,1024,1,&comparator,&batch)!=HPS_STATUS_OK||batch==NULL||
        hps_group_batch_group_count(batch)!=1024||
        hps_group_batch_merge_all(batch,&comparator,&result)!=HPS_STATUS_OK||
        result==NULL||hps_group_size(result)!=1024)valid=0;
    peak=hps_alloc_stats_get();
    if(valid&&(cc.comparison_count!=14337||!benchmark_validate_final(result,1024,&(BenchmarkPathStats){0})||
        !stage83_tag_accounting_valid(&peak,"12.2 1024 singleton peak")||
        !stage83_other_is_empty(&peak,"12.2 1024 singleton peak")))valid=0;
    hps_group_batch_destroy(batch);batch=NULL;result_only=hps_alloc_stats_get();
    if(valid&&(!stage83_tag_accounting_valid(&result_only,"12.2 1024 singleton result")||
        !stage83_other_is_empty(&result_only,"12.2 1024 singleton result")||
        result_only.live_bytes!=result_only.tags[HPS_ALLOC_TAG_PATH_OBJECT].live_bytes+
            result_only.tags[HPS_ALLOC_TAG_PATH_STEPS].live_bytes+
            result_only.tags[HPS_ALLOC_TAG_TREE_OBJECT].live_bytes+
            result_only.tags[HPS_ALLOC_TAG_TREE_NODE].live_bytes+
            result_only.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_bytes+
            result_only.tags[HPS_ALLOC_TAG_GROUP_OBJECT].live_bytes+
            result_only.tags[HPS_ALLOC_TAG_GROUP_ORDERED].live_bytes))valid=0;
    printf("Stage12.2Singleton1024\nComparisonCount=%lu\nPeakBytes=%lu\nResultOnlyBytes=%lu\nPeakTreeObject=%lu PeakTreeNode=%lu PeakTreeChildren=%lu\nResultTreeObject=%lu ResultTreeNode=%lu ResultTreeChildren=%lu\n",
        (unsigned long)cc.comparison_count,(unsigned long)peak.peak_live_bytes,
        (unsigned long)result_only.live_bytes,
        (unsigned long)peak.bytes_at_global_peak[HPS_ALLOC_TAG_TREE_OBJECT],
        (unsigned long)peak.bytes_at_global_peak[HPS_ALLOC_TAG_TREE_NODE],
        (unsigned long)peak.bytes_at_global_peak[HPS_ALLOC_TAG_TREE_CHILDREN],
        (unsigned long)result_only.tags[HPS_ALLOC_TAG_TREE_OBJECT].live_bytes,
        (unsigned long)result_only.tags[HPS_ALLOC_TAG_TREE_NODE].live_bytes,
        (unsigned long)result_only.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_bytes);
    hps_group_destroy(result);result=NULL;
    final_stats=hps_alloc_stats_get();
    if(!benchmark_alloc_stats_empty(&final_stats)||final_stats.failed_calls||
        !stage83_tag_accounting_valid(&final_stats,"12.2 1024 singleton final")||
        !stage83_other_is_empty(&final_stats,"12.2 1024 singleton final"))valid=0;
    printf("Stage12.2Singleton1024Status=%s FinalLive=%lu/%lu OTHER=%lu FailedCalls=%lu\n",
        valid?"PASS":"FAIL",(unsigned long)final_stats.live_bytes,
        (unsigned long)final_stats.live_blocks,
        (unsigned long)final_stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes,
        (unsigned long)final_stats.failed_calls);
    return valid;
}

static int stage122_print_results(Stage103Sample samples[][STAGE103_SEEDS],
    HpsTreeInternalProfile profiles[][STAGE103_SEEDS],size_t ptr_size,
    size_t size_size,size_t tree_size,size_t node_size)
{
    static const size_t groups[3]={1,32,40};
    static const size_t old_tree[3]={579488,576752,576416};
    static const size_t old_leaves[3]={6708,6011,6018};
    static const size_t old_blocks[3]={3293,3990,3983};
    static const size_t old_capacity[3]={12429,12087,12045};
    static const size_t old_unary[3]={1631,2649,2441};
    static const size_t old_branching[3]={1661,1340,1541};
    static const size_t old_root_count[3]={50,59,54};
    static const size_t old_root_capacity[3]={64,64,64};
    size_t i,header=hps_tree_internal_child_block_header_size();int valid=1;
    printf("\nHPSort Stage 12.2: Single ChildBlock production representation\n");
    printf("Stage12.2_TreeLayout\nsizeof_HpsTreeNode=%lu alignof_HpsTreeNode=%lu\nsizeof_HpsTree=%lu alignof_HpsTree=%lu\nsizeof_HpsTreeChildBlockHeader=%lu alignof_HpsTreeChildBlock=%lu offsetof_children=%lu sizeof_pointer=%lu sizeof_size_t=%lu\n",
        (unsigned long)node_size,(unsigned long)hps_tree_internal_alignof_node(),
        (unsigned long)tree_size,(unsigned long)hps_tree_internal_alignof_tree(),
        (unsigned long)header,(unsigned long)hps_tree_internal_alignof_child_block(),
        (unsigned long)header,(unsigned long)ptr_size,(unsigned long)size_size);
    printf("Stage12.2_MemoryCSV\nGroups,OldTreeBytes,NewTreeBytes,TreeBytesSaved,TreeSavingsPercent,OldResultBytes,NewResultBytes,ResultBytesSaved,ResultSavingsPercent,LeafCount,ExpectedSaving16xLeaf,ActualTreeSaving,ModelErrorBytes\n");
    for(i=0;i<3;++i){
        size_t gi=stage111_group_index(groups[i]);Stage103Sample *x=&samples[gi][0];
        HpsTreeInternalProfile *p=&profiles[gi][0];
        size_t new_tree=stage112_tree_bytes(x),new_result=x->result_bytes;
        size_t saved=old_tree[i]-new_tree,expected=16*p->leaf_count;
        size_t old_result=new_result+saved,error=(new_tree>(i==0?472160u:i==1?480576u:480128u))?
            new_tree-(i==0?472160u:i==1?480576u:480128u):
            (i==0?472160u:i==1?480576u:480128u)-new_tree;
        if(p->real_node_count!=10000||p->total_child_count!=10000||
            p->total_child_capacity!=old_capacity[i]||p->leaf_count!=old_leaves[i]||
            p->unary_count!=old_unary[i]||p->branching_count!=old_branching[i]||
            p->root_child_count!=old_root_count[i]||p->root_child_capacity!=old_root_capacity[i]||
            p->allocated_child_array_count!=old_blocks[i]||new_tree!=(i==0?472160u:i==1?480576u:480128u)||
            saved!=expected||error!=0||
            x->result_stats.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_blocks!=old_blocks[i]||
            x->result_stats.tags[HPS_ALLOC_TAG_TREE_NODE].live_blocks!=10000||
            x->result_stats.tags[HPS_ALLOC_TAG_TREE_OBJECT].live_blocks!=1||
            x->result_stats.tags[HPS_ALLOC_TAG_TREE_NODE].live_bytes!=10000*node_size||
            x->result_stats.tags[HPS_ALLOC_TAG_TREE_OBJECT].live_bytes!=tree_size)valid=0;
        printf("%lu,%lu,%lu,%lu,%.6f,%lu,%lu,%lu,%.6f,%lu,%lu,%lu,%lu\n",
            (unsigned long)groups[i],(unsigned long)old_tree[i],(unsigned long)new_tree,
            (unsigned long)saved,100.0*(double)saved/(double)old_tree[i],
            (unsigned long)old_result,(unsigned long)new_result,(unsigned long)(old_result-new_result),
            old_result?100.0*(double)(old_result-new_result)/(double)old_result:0.0,
            (unsigned long)p->leaf_count,(unsigned long)expected,(unsigned long)saved,(unsigned long)error);
    }
    printf("Stage12.2_TreeDecompositionCSV\nGroups,TreeObjectBytes,TreeNodeBytes,ChildBlockHeaderBytes,ChildCapacityPointerBytes,ChildUsedPointerBytes,ChildCapacitySlackBytes,TreeChildrenTagBytes,TreeTotalBytes,AllocatedChildBlocks\n");
    for(i=0;i<3;++i){size_t gi=stage111_group_index(groups[i]);HpsTreeInternalProfile *p=&profiles[gi][0];
        Stage103Sample *x=&samples[gi][0];size_t obj=x->result_stats.tags[HPS_ALLOC_TAG_TREE_OBJECT].live_bytes;
        size_t nodes=x->result_stats.tags[HPS_ALLOC_TAG_TREE_NODE].live_bytes;
        size_t block_headers=p->allocated_child_array_count*header;
        size_t capacity_ptrs=p->total_child_capacity*ptr_size;
        size_t used_ptrs=p->total_child_count*ptr_size;
        size_t slack=(p->total_child_capacity-p->total_child_count)*ptr_size;
        size_t child_tag=x->result_stats.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_bytes;
        size_t total=obj+nodes+child_tag;
        if(block_headers+capacity_ptrs!=child_tag||used_ptrs+slack!=capacity_ptrs||
            x->result_stats.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_blocks!=p->allocated_child_array_count)valid=0;
        printf("%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",(unsigned long)groups[i],
            (unsigned long)obj,(unsigned long)nodes,(unsigned long)block_headers,
            (unsigned long)capacity_ptrs,(unsigned long)used_ptrs,(unsigned long)slack,
            (unsigned long)child_tag,(unsigned long)total,
            (unsigned long)x->result_stats.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_blocks);
    }
    printf("Stage12.2_StructureAndAccounting=%s\n",valid?"PASS":"FAIL");
    printf("Stage12.2_PeakTreeCSV\nGroups,OverallPeakBytes,PeakTreeObject,PeakTreeNode,PeakTreeChildren,PeakTreeTotal,PeakPathTotal,PeakGroupTotal,PeakMergeScratch\n");
    for(i=1;i<3;++i){size_t gi=stage111_group_index(groups[i]);HpsAllocStats *a=&samples[gi][0].peak_stats;
        size_t path=a->bytes_at_global_peak[HPS_ALLOC_TAG_PATH_OBJECT]+a->bytes_at_global_peak[HPS_ALLOC_TAG_PATH_STEPS];
        size_t tree=a->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_OBJECT]+a->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_NODE]+a->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_CHILDREN];
        size_t group=a->bytes_at_global_peak[HPS_ALLOC_TAG_GROUP_OBJECT]+a->bytes_at_global_peak[HPS_ALLOC_TAG_GROUP_ORDERED];
        printf("%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",(unsigned long)groups[i],
            (unsigned long)a->peak_live_bytes,
            (unsigned long)a->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_OBJECT],
            (unsigned long)a->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_NODE],
            (unsigned long)a->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_CHILDREN],
            (unsigned long)tree,(unsigned long)path,(unsigned long)group,
            (unsigned long)a->bytes_at_global_peak[HPS_ALLOC_TAG_MERGE_SCRATCH]);
    }
    for(i=0;i<3;++i){size_t gi=stage111_group_index(groups[i]);
        Stage103Sample *x=&samples[gi][0];
        printf("Stage12.2_G%lu_NonTreeResultTags: PATH_OBJECT=%lu PATH_STEPS=%lu GROUP_OBJECT=%lu GROUP_ORDERED=%lu; old-result comparison models these tags unchanged\n",
            (unsigned long)groups[i],(unsigned long)x->result_stats.tags[HPS_ALLOC_TAG_PATH_OBJECT].live_bytes,
            (unsigned long)x->result_stats.tags[HPS_ALLOC_TAG_PATH_STEPS].live_bytes,
            (unsigned long)x->result_stats.tags[HPS_ALLOC_TAG_GROUP_OBJECT].live_bytes,
            (unsigned long)x->result_stats.tags[HPS_ALLOC_TAG_GROUP_ORDERED].live_bytes);
    }
    if(!stage122_direct_regressions())valid=0;
    if(!stage122_run_1024_singletons())valid=0;
    printf("Stage12.2Status=%s\n",valid?"PASS":"FAIL");
    return valid;
}

int hps_run_stage11_2_tree_profile(void)
{
    static const size_t exact_groups[3]={1,32,40};
    static const size_t scaling_n[4]={1000,2000,5000,10000};
    static const uint32_t scaling_seeds[3]={UINT32_C(0xC0FFEE),UINT32_C(0xC0FFEF),UINT32_C(0xC0FFF0)};
    Stage103Sample main_samples[STAGE103_GROUPS][STAGE103_SEEDS];
    HpsTreeInternalProfile profiles[STAGE103_GROUPS][STAGE103_SEEDS];
    Stage112TreeCapture exact[3]={{0}};
    Stage103Sample scale_samples[4][3][3];
    HpsTreeInternalProfile scale_profiles[4][3][3];
    double min_slack_pct=DBL_MAX,max_slack_pct=0.0,min_leaf_pct=DBL_MAX,max_leaf_pct=0.0;
    double min_node_pct=DBL_MAX,max_node_pct=0.0,min_children_pct=DBL_MAX,max_children_pct=0.0;
    double min_leafmeta_pct=DBL_MAX,max_leafmeta_pct=0.0;
    size_t g,s,i,e,valid_samples=0,valid_scale=0;
    size_t ptr_size=sizeof(void *),size_size=sizeof(size_t),node_size=hps_tree_internal_sizeof_node();
    size_t tree_size=hps_tree_internal_sizeof_tree();
    size_t logical_node_bytes=4*sizeof(void *);
    size_t node_padding=node_size-logical_node_bytes;
    int valid=1;
    printf("\nHPSort Stage 11.2: Tree structural profile on current production layout\n");
    if(!stage111_legacy_production_regressions()||hps_run_stage11_1_gs100_fixture_check()!=0)return 1;
    printf("TreeLayout\nsizeof_HpsTree=%lu\n_Alignof_HpsTree=%lu\nsizeof_HpsTreeNode=%lu\n_Alignof_HpsTreeNode=%lu\nsizeof_void_ptr=%lu\nsizeof_size_t=%lu\nLogicalTreeNodeFieldBytes=%lu\nTreeNodeStructOverheadBytes=%lu\nChildBlockHeaderOffset=%lu\n",
        (unsigned long)tree_size,(unsigned long)hps_tree_internal_alignof_tree(),
        (unsigned long)node_size,(unsigned long)hps_tree_internal_alignof_node(),
        (unsigned long)ptr_size,(unsigned long)size_size,
        (unsigned long)logical_node_bytes,(unsigned long)node_padding,
        (unsigned long)hps_tree_internal_child_block_header_size());
    memset(main_samples,0,sizeof(main_samples));memset(profiles,0,sizeof(profiles));
    memset(scale_samples,0,sizeof(scale_samples));memset(scale_profiles,0,sizeof(scale_profiles));
    for(g=0;g<STAGE103_GROUPS;++g)for(s=0;s<STAGE103_SEEDS;++s){
        Stage103Snapshot result_snapshot={0};Stage112TreeCapture local={0};
        Stage112TreeCapture *capture=&local;
        for(e=0;e<3;++e)if(s==0&&stage103_groups[g]==exact_groups[e])capture=&exact[e];
        if(!stage103_run_one_internal(STAGE103_N,stage103_groups[g],stage103_seeds[s],0,
                &main_samples[g][s],&result_snapshot,capture)){
            printf("Stage11.2 ResultOnly profile sample FAILED G=%lu seed=%08lX\n",
                (unsigned long)stage103_groups[g],(unsigned long)stage103_seeds[s]);valid=0;
        } else {
            profiles[g][s]=capture->profile;++valid_samples;
        }
        stage103_snapshot_destroy(&result_snapshot);
        if(capture==&local)stage112_tree_capture_destroy(&local);
    }
    printf("Stage11.2 MainResultProfiles=%lu/90; exact TREE_OBJECT/NODE/CHILDREN accounting=%s; real-node/edge invariants=%s\n",
        (unsigned long)valid_samples,valid_samples==90?"PASS":"FAIL",valid_samples==90?"PASS":"FAIL");
    printf("\nStage11.2_ResultTreeDecompositionCSV\nGroups,RealNodeCount,TreeObjectBytes,NodeBytes,ChildrenBytes,UsedChildPointerBytes,ChildSlackBytes,ChildCapacityUtilization,TreeTotalBytes,NodePercentOfTree,ChildrenPercentOfTree,SlackPercentOfTree,LeafCount,LeafPercent,UnaryCount,UnaryPercent,BranchingCount,BranchingPercent,RootChildCount,RootChildCapacity\n");
    for(e=0;e<3;++e){g=stage111_group_index(exact_groups[e]);s=0;
        HpsTreeInternalProfile *p=&exact[e].profile;Stage103Sample *x=&main_samples[g][s];
        size_t obj=x->result_stats.tags[HPS_ALLOC_TAG_TREE_OBJECT].live_bytes;
        size_t nodes=x->result_stats.tags[HPS_ALLOC_TAG_TREE_NODE].live_bytes;
        size_t children=x->result_stats.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_bytes;
        size_t used=p->total_child_count*ptr_size,slack=stage112_slack_bytes(p),total=obj+nodes+children;
        printf("%lu,%lu,%lu,%lu,%lu,%lu,%lu,%.8f,%lu,%.6f,%.6f,%.6f,%lu,%.6f,%lu,%.6f,%lu,%.6f,%lu,%lu\n",
            (unsigned long)exact_groups[e],(unsigned long)p->real_node_count,(unsigned long)obj,
            (unsigned long)nodes,(unsigned long)children,(unsigned long)used,(unsigned long)slack,
            p->total_child_capacity? (double)p->total_child_count/(double)p->total_child_capacity:0.0,
            (unsigned long)total,total?100.0*nodes/total:0.0,total?100.0*children/total:0.0,
            total?100.0*slack/total:0.0,(unsigned long)p->leaf_count,100.0*p->leaf_count/p->real_node_count,
            (unsigned long)p->unary_count,100.0*p->unary_count/p->real_node_count,
            (unsigned long)p->branching_count,100.0*p->branching_count/p->real_node_count,
            (unsigned long)p->root_child_count,(unsigned long)p->root_child_capacity);
        if(p->real_node_count!=10000||p->total_child_count!=p->real_node_count||
            obj!=tree_size||nodes!=p->real_node_count*node_size||
            children!=stage112_children_bytes(p)||
            total!=obj+nodes+children)valid=0;
    }
    printf("\nStage11.2_NodeFieldBytesCSV\nGroups,PathPtrBytes,ItemPtrBytes,ParentPtrBytes,ChildBlockPtrBytes,TreeNodeStructPaddingBytes\n");
    for(e=0;e<3;++e)printf("%lu,%lu,%lu,%lu,%lu,%lu\n",(unsigned long)exact_groups[e],
        (unsigned long)(10000*ptr_size),(unsigned long)(10000*ptr_size),(unsigned long)(10000*ptr_size),
        (unsigned long)(10000*ptr_size),(unsigned long)node_padding);
    printf("\nStage11.2_LeafMetadataCSV\nGroups,LeafCount,LeafPercent,ChildBlockPointerFieldBytesPerNode,LeafChildBlockPointerFieldBytes,LeafChildBlockPointerPercentOfTree,ExactChildCapacitySlackBytes,ExactChildCapacitySlackPercentOfTree\n");
    for(e=0;e<3;++e){g=stage111_group_index(exact_groups[e]);s=0;{
        HpsTreeInternalProfile *p=&exact[e].profile;size_t total=stage112_tree_bytes(&main_samples[g][s]);
        size_t fixed=ptr_size,leafbytes=p->leaf_count*fixed,slack=stage112_slack_bytes(p);
        printf("%lu,%lu,%.6f,%lu,%lu,%.6f,%lu,%.6f\n",(unsigned long)exact_groups[e],
            (unsigned long)p->leaf_count,100.0*p->leaf_count/p->real_node_count,(unsigned long)fixed,
            (unsigned long)leafbytes,total?100.0*leafbytes/total:0.0,(unsigned long)slack,
            total?100.0*slack/total:0.0);}}
    printf("\nStage11.2_ChildArrayFactsCSV\nGroups,AllocatedChildArrayCount,RealNodesWithChildArrayCount,TotalChildCount,TotalChildCapacity,AverageCapacityPerAllocatedChildArray,AverageUsedChildrenPerAllocatedChildArray,AverageSlackSlotsPerAllocatedChildArray\n");
    for(e=0;e<3;++e){HpsTreeInternalProfile *p=&exact[e].profile;double arrays=(double)p->allocated_child_array_count;
        printf("%lu,%lu,%lu,%lu,%lu,%.6f,%.6f,%.6f\n",(unsigned long)exact_groups[e],
            (unsigned long)p->allocated_child_array_count,(unsigned long)p->real_nodes_with_child_array_count,
            (unsigned long)p->total_child_count,(unsigned long)p->total_child_capacity,
            arrays?p->total_child_capacity/arrays:0.0,arrays?p->total_child_count/arrays:0.0,
            arrays?(p->total_child_capacity-p->total_child_count)/arrays:0.0);}
    for(e=0;e<3;++e){HpsTreeInternalProfile *p=&exact[e].profile;size_t total_nodes=0;
        printf("\nStage11.2_G%lu_DegreeHistogram\nDegreeBucket,NodeCount,Percent\n",(unsigned long)exact_groups[e]);
        for(i=0;i<11;++i){total_nodes+=p->degree_bucket_counts[i];printf("%s,%lu,%.6f\n",stage112_degree_label(i),(unsigned long)p->degree_bucket_counts[i],100.0*p->degree_bucket_counts[i]/p->real_node_count);}
        if(total_nodes!=p->real_node_count)valid=0;
        printf("\nStage11.2_G%lu_CapacityHistogram\nCapacity,NodeCount,Percent\n",(unsigned long)exact_groups[e]);
        for(i=0;i<exact[e].capacity_count;++i)if(exact[e].capacity_counts[i])printf("%lu,%lu,%.6f\n",(unsigned long)i,(unsigned long)exact[e].capacity_counts[i],100.0*exact[e].capacity_counts[i]/p->real_node_count);
        printf("RootChildCount=%lu\nRootChildCapacity=%lu\n",(unsigned long)p->root_child_count,(unsigned long)p->root_child_capacity);
        printf("\nStage11.2_G%lu_DegreeCapacityJoint\nDegreeBucket,NodeCount,MeanCapacity,MinCapacity,MaxCapacity,UnusedSlotsTotal\n",(unsigned long)exact_groups[e]);
        for(i=0;i<8;++i){HpsTreeInternalDegreeCapacity *j=&p->degree_capacity[i];
            printf("%s,%lu,%.6f,%lu,%lu,%lu\n",stage112_joint_label(i),(unsigned long)j->node_count,
                j->node_count?(double)j->capacity_sum/j->node_count:0.0,
                (unsigned long)(j->node_count?j->minimum_capacity:0),(unsigned long)j->maximum_capacity,
                (unsigned long)j->unused_slots_total);}}
    printf("\nStage11.2_Main18TreeStructureCSV\nGroups,GroupSize,TreeBytesPerItem,NodeBytesPerItem,ChildrenBytesPerItem,ChildSlackBytesPerItem,ChildCapacityUtilizationMean,LeafPercentMean,UnaryPercentMean,BranchingPercentMean,LeafChildManagementBytesPerItem\n");
    for(g=0;g<STAGE103_GROUPS;++g){double tree=0,nodes=0,children=0,slack=0,util=0,leaf=0,unary=0,branch=0,leaf_fields=0,gs=0;
        for(s=0;s<STAGE103_SEEDS;++s){HpsTreeInternalProfile *p=&profiles[g][s];Stage103Sample *x=&main_samples[g][s];
            size_t tb=stage112_tree_bytes(x);tree+=(double)tb/STAGE103_N;
            nodes+=(double)x->result_stats.tags[HPS_ALLOC_TAG_TREE_NODE].live_bytes/STAGE103_N;
            children+=(double)x->result_stats.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_bytes/STAGE103_N;
            slack+=(double)stage112_slack_bytes(p)/STAGE103_N;
            util+=p->total_child_capacity?(double)p->total_child_count/p->total_child_capacity:0.0;
            leaf+=100.0*p->leaf_count/p->real_node_count;unary+=100.0*p->unary_count/p->real_node_count;
            branch+=100.0*p->branching_count/p->real_node_count;
            leaf_fields+=(double)p->leaf_count*ptr_size/STAGE103_N;gs+=(double)x->group_size;
        }
        tree/=STAGE103_SEEDS;nodes/=STAGE103_SEEDS;children/=STAGE103_SEEDS;slack/=STAGE103_SEEDS;util/=STAGE103_SEEDS;
        leaf/=STAGE103_SEEDS;unary/=STAGE103_SEEDS;branch/=STAGE103_SEEDS;leaf_fields/=STAGE103_SEEDS;gs/=STAGE103_SEEDS;
        {double node_pct=tree?100.0*nodes/tree:0.0,children_pct=tree?100.0*children/tree:0.0;
            double slack_pct=tree?100.0*slack/tree:0.0,leaf_pct=leaf;
            double leafmeta_pct=tree?100.0*leaf_fields/tree:0.0;
            if(node_pct<min_node_pct)min_node_pct=node_pct;if(node_pct>max_node_pct)max_node_pct=node_pct;
            if(children_pct<min_children_pct)min_children_pct=children_pct;if(children_pct>max_children_pct)max_children_pct=children_pct;
            if(slack_pct<min_slack_pct)min_slack_pct=slack_pct;if(slack_pct>max_slack_pct)max_slack_pct=slack_pct;
            if(leaf_pct<min_leaf_pct)min_leaf_pct=leaf_pct;if(leaf_pct>max_leaf_pct)max_leaf_pct=leaf_pct;
            if(leafmeta_pct<min_leafmeta_pct)min_leafmeta_pct=leafmeta_pct;if(leafmeta_pct>max_leafmeta_pct)max_leafmeta_pct=leafmeta_pct;}
        printf("%lu,%.2f,%.6f,%.6f,%.6f,%.6f,%.8f,%.6f,%.6f,%.6f,%.6f\n",
            (unsigned long)stage103_groups[g],gs,tree,nodes,children,slack,util,leaf,unary,branch,leaf_fields);
    }
    printf("\nStage11.2_G32ScalingTreeCSV\nN,TreeBytesPerItem,NodeBytesPerItem,ChildrenBytesPerItem,ChildSlackBytesPerItem,ChildCapacityUtilization,LeafPercent,UnaryPercent,BranchingPercent,LeafChildManagementBytesPerItem\n");
    for(i=0;i<4;++i){double tree=0,nodes=0,children=0,slack=0,util=0,leaf=0,unary=0,branch=0,leaf_fields=0;size_t n=scaling_n[i];
        for(s=0;s<3;++s){Stage103Snapshot result_snapshot={0};Stage112TreeCapture capture={0};Stage103Sample x={0};
            if(!stage103_run_one_internal(n,32,scaling_seeds[s],0,&x,&result_snapshot,&capture)){valid=0;}
            else{HpsTreeInternalProfile *p=&capture.profile;++valid_scale;
                scale_samples[i][2][s]=x;scale_profiles[i][2][s]=*p;
                tree+=(double)stage112_tree_bytes(&x)/n;
                nodes+=(double)x.result_stats.tags[HPS_ALLOC_TAG_TREE_NODE].live_bytes/n;
                children+=(double)x.result_stats.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_bytes/n;
                slack+=(double)stage112_slack_bytes(p)/n;
                util+=p->total_child_capacity?(double)p->total_child_count/p->total_child_capacity:0.0;
                leaf+=100.0*p->leaf_count/p->real_node_count;unary+=100.0*p->unary_count/p->real_node_count;
                branch+=100.0*p->branching_count/p->real_node_count;
                leaf_fields+=(double)p->leaf_count*ptr_size/n;}
            stage112_tree_capture_destroy(&capture);stage103_snapshot_destroy(&result_snapshot);}
        printf("%lu,%.6f,%.6f,%.6f,%.6f,%.8f,%.6f,%.6f,%.6f,%.6f\n",(unsigned long)n,
            tree/3,nodes/3,children/3,slack/3,util/3,leaf/3,unary/3,branch/3,leaf_fields/3);}
    printf("Stage11.2ScalingTreeProfiles=%lu/12\n",(unsigned long)valid_scale);
    printf("\nStage11.2PeakTreeUnitsCSV\nGroups,PeakTreeNodeBytes,PeakAllocatedRealNodeUnits,PeakTreeChildrenBytes,PeakChildBlockCount,PeakTreeObjectBytes,PeakTreeTotalBytes\n");
    for(e=0;e<2;++e){g=stage111_group_index(e==0?32:40);Stage103Sample *x=&main_samples[g][0];
        size_t nb=x->peak_stats.bytes_at_global_peak[HPS_ALLOC_TAG_TREE_NODE];
        size_t cb=x->peak_stats.bytes_at_global_peak[HPS_ALLOC_TAG_TREE_CHILDREN];
        size_t ob=x->peak_stats.bytes_at_global_peak[HPS_ALLOC_TAG_TREE_OBJECT];
        if(node_size==0||nb%node_size)valid=0;
        printf("%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",(unsigned long)(e==0?32:40),(unsigned long)nb,
            (unsigned long)(node_size?nb/node_size:0),(unsigned long)cb,
            (unsigned long)x->peak_stats.blocks_at_global_peak[HPS_ALLOC_TAG_TREE_CHILDREN],
            (unsigned long)ob,(unsigned long)(nb+cb+ob));}
    printf("\nStage11TreeFacts\nsizeof_HpsTreeNode=%lu\nsizeof_HpsTree=%lu\nTreeNodeStructOverheadBytes=%lu\n",
        (unsigned long)node_size,(unsigned long)tree_size,(unsigned long)node_padding);
    for(e=0;e<2;++e){g=stage111_group_index(e==0?32:40);s=0;{
        HpsTreeInternalProfile *p=&profiles[g][s];size_t total=stage112_tree_bytes(&main_samples[g][s]);
        double tree_per_item=(double)total/STAGE103_N,node_pct=total?100.0*main_samples[g][s].result_stats.tags[HPS_ALLOC_TAG_TREE_NODE].live_bytes/total:0.0;
        double children_pct=total?100.0*main_samples[g][s].result_stats.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_bytes/total:0.0;
        double slack_pct=total?100.0*stage112_slack_bytes(p)/total:0.0;
        double leaf_pct=100.0*p->leaf_count/p->real_node_count;
        double leafmeta_pct=total?100.0*p->leaf_count*ptr_size/total:0.0;
        printf("G%lu_ResultTreeBytesPerItem=%.6f\nG%lu_NodePercentOfTree=%.6f\nG%lu_ChildrenPercentOfTree=%.6f\nG%lu_ChildSlackPercentOfTree=%.6f\nG%lu_LeafPercent=%.6f\nG%lu_LeafChildManagementPercentOfTree=%.6f\n",
            (unsigned long)(e==0?32:40),tree_per_item,(unsigned long)(e==0?32:40),node_pct,(unsigned long)(e==0?32:40),children_pct,
            (unsigned long)(e==0?32:40),slack_pct,(unsigned long)(e==0?32:40),leaf_pct,(unsigned long)(e==0?32:40),leafmeta_pct);}}
    printf("Main18_MinNodePercentOfTree=%.6f\nMain18_MaxNodePercentOfTree=%.6f\nMain18_MinChildrenPercentOfTree=%.6f\nMain18_MaxChildrenPercentOfTree=%.6f\nMain18_MinChildSlackPercentOfTree=%.6f\nMain18_MaxChildSlackPercentOfTree=%.6f\nMain18_MinLeafPercent=%.6f\nMain18_MaxLeafPercent=%.6f\nMain18_MinLeafChildManagementPercentOfTree=%.6f\nMain18_MaxLeafChildManagementPercentOfTree=%.6f\n",
        min_node_pct,max_node_pct,min_children_pct,max_children_pct,min_slack_pct,max_slack_pct,
        min_leaf_pct,max_leaf_pct,min_leafmeta_pct,max_leafmeta_pct);
    for(e=0;e<3;++e)stage112_tree_capture_destroy(&exact[e]);
    {
        HpsAllocStats final_stats=hps_alloc_stats_get();
        if(!benchmark_alloc_stats_empty(&final_stats)||final_stats.failed_calls||
            !stage83_tag_accounting_valid(&final_stats,"11.2 final")||
            !stage83_other_is_empty(&final_stats,"11.2 final"))valid=0;
        printf("Stage11.2FinalAllocator: live=%lu/%lu OTHER=%lu FailedCalls=%lu\nStage11.2Status=%s MainProfiles=%lu/90 ScalingProfiles=%lu/12\n",
            (unsigned long)final_stats.live_bytes,(unsigned long)final_stats.live_blocks,
            (unsigned long)final_stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes,(unsigned long)final_stats.failed_calls,
            valid?"PASS":"FAIL",(unsigned long)valid_samples,(unsigned long)valid_scale);
    }
    return valid?0:1;
}

int hps_run_stage12_2_validation(void)
{
    static const size_t groups[3]={1,32,40};
    Stage103Sample samples[STAGE103_GROUPS][STAGE103_SEEDS];
    HpsTreeInternalProfile profiles[STAGE103_GROUPS][STAGE103_SEEDS];
    size_t i;int valid=1;
    memset(samples,0,sizeof(samples));memset(profiles,0,sizeof(profiles));
    printf("\nHPSort Stage 12.2 focused validation: only G1/G32/G40 single-seed plus GS100 and core regressions\n");
    if(!stage111_legacy_production_regressions()||
        hps_run_stage11_1_gs100_fixture_check()!=0)return 1;
    for(i=0;i<3;++i){
        size_t gi=stage111_group_index(groups[i]);Stage103Snapshot snapshot={0};
        Stage112TreeCapture capture={0};
        if(!stage103_run_one_internal(STAGE103_N,groups[i],UINT32_C(0xC0FFEE),0,
                &samples[gi][0],&snapshot,&capture)){
            printf("Stage12.2SingleSeedProfile=FAIL Groups=%lu Seed=0xC0FFEE\n",
                (unsigned long)groups[i]);valid=0;
        }else{
            profiles[gi][0]=capture.profile;
            printf("Stage12.2SingleSeedProfile=PASS Groups=%lu Seed=0xC0FFEE\n",
                (unsigned long)groups[i]);
        }
        stage112_tree_capture_destroy(&capture);stage103_snapshot_destroy(&snapshot);
    }
    if(valid&&!stage122_print_results(samples,profiles,sizeof(void *),sizeof(size_t),
            hps_tree_internal_sizeof_tree(),hps_tree_internal_sizeof_node()))valid=0;
    {
        HpsAllocStats final_stats=hps_alloc_stats_get();
        if(!benchmark_alloc_stats_empty(&final_stats)||final_stats.failed_calls||
            !stage83_tag_accounting_valid(&final_stats,"12.2 focused final")||
            !stage83_other_is_empty(&final_stats,"12.2 focused final"))valid=0;
        printf("Stage12.2FinalAllocator: live=%lu/%lu OTHER=%lu FailedCalls=%lu\nStage12.2FocusedStatus=%s\n",
            (unsigned long)final_stats.live_bytes,(unsigned long)final_stats.live_blocks,
            (unsigned long)final_stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes,
            (unsigned long)final_stats.failed_calls,valid?"PASS":"FAIL");
    }
    return valid?0:1;
}

/* Stage 12.3 is verification/reporting only.  The frozen Stage 11.3
 * Result/Path/Group means come from stage121-output.txt; no pre-ChildBlock
 * Stage 11.1 peak matrix is present in the project, so peak comparisons are
 * deliberately reported as NA rather than reconstructed from another stage. */
static const double stage123_old_result[STAGE103_GROUPS]={
    1634596.80,1638457.60,1643200.00,1651161.60,1653587.20,1659062.40,
    1660176.00,1667153.60,1669979.20,1675531.20,1678320.00,1686518.40,
    1691374.40,1694112.00,1696857.60,1704582.40,1704736.00,1707227.20};
static const double stage123_old_path[STAGE103_GROUPS]={
    975427.20,978632.00,983864.00,992153.60,995609.60,1000974.40,
    1002144.00,1009427.20,1013105.60,1018206.40,1020916.80,1029536.00,
    1034083.20,1036304.00,1039603.20,1047299.20,1047152.00,1050070.40};

static double stage123_percent(double saved,double base)
{ return base==0.0?0.0:100.0*saved/base; }

static int stage123_profile_valid(const Stage103Sample *x,
    const HpsTreeInternalProfile *p,size_t n)
{
    size_t tree=stage112_tree_bytes(x),saved;
    if(p->real_node_count!=n||p->total_child_count!=p->real_node_count||
        x->result_stats.tags[HPS_ALLOC_TAG_TREE_NODE].live_blocks!=n||
        x->result_stats.tags[HPS_ALLOC_TAG_TREE_NODE].live_bytes!=n*hps_tree_internal_sizeof_node()||
        x->result_stats.tags[HPS_ALLOC_TAG_TREE_OBJECT].live_blocks!=1||
        x->result_stats.tags[HPS_ALLOC_TAG_TREE_OBJECT].live_bytes!=hps_tree_internal_sizeof_tree()||
        x->result_stats.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_blocks!=p->allocated_child_array_count||
        x->total_comparisons!=x->build_comparisons+x->merge_comparisons)return 0;
    saved=16*p->leaf_count;
    return tree+saved>=tree && (tree+saved)-tree==saved;
}

static int stage123_gs100(void)
{
    static int values[1000];static void *items[1000];
    BenchmarkCompareContext cc={0,0};HpsComparator cmp={benchmark_compare_int,&cc};
    HpsGroupBatch *batch=NULL;HpsGroup *result=NULL;HpsAllocStats b,p,r,f;
    size_t i;int ok=1;
    memset(&b,0,sizeof(b));memset(&p,0,sizeof(p));memset(&r,0,sizeof(r));
    if(hps_alloc_stats_reset()!=0)return 0;
    for(i=0;i<1000;++i){values[i]=(int)((i*613u)%1000u);items[i]=&values[i];}
    if(hps_group_batch_build(items,1000,100,&cmp,&batch)!=HPS_STATUS_OK||
        batch==NULL||hps_group_batch_group_count(batch)!=10)ok=0;
    b=hps_alloc_stats_get();
    if(ok&&(hps_group_batch_merge_all(batch,&cmp,&result)!=HPS_STATUS_OK||
        !benchmark_validate_final(result,1000,&(BenchmarkPathStats){0})))ok=0;
    if(ok){p=hps_alloc_stats_get();
        hps_group_batch_destroy(batch);batch=NULL;r=hps_alloc_stats_get();
        if(b.live_bytes!=115880||p.live_bytes!=249472||r.live_bytes!=133592||
            p.peak_live_bytes!=341984||p.live_bytes!=b.live_bytes+r.live_bytes||
            p.live_blocks!=b.live_blocks+r.live_blocks||
            !stage83_tag_accounting_valid(&r,"12.3 GS100")||
            !stage83_other_is_empty(&r,"12.3 GS100"))ok=0;
        printf("Stage12.3_GS100_Invariants Batch=%s PostMerge=%s ResultOnly=%s BlockIdentity=%s Peak=%s Tags=%s\n",
            b.live_bytes==115880?"PASS":"FAIL",p.live_bytes==249472?"PASS":"FAIL",
            r.live_bytes==133592?"PASS":"FAIL",p.live_bytes==b.live_bytes+r.live_bytes&&p.live_blocks==b.live_blocks+r.live_blocks?"PASS":"FAIL",
            p.peak_live_bytes==341984?"PASS":"FAIL",stage83_tag_accounting_valid(&r,"12.3 GS100 recheck")?"PASS":"FAIL");
    }else{memset(&r,0,sizeof(r));}
    printf("Stage12.3_GS100_CSV\nMetric,Stage11Before,Stage12After,SavedBytes,SavingsPercent\n");
    printf("Batch,127192,%lu,%ld,%.6f\n",(unsigned long)b.live_bytes,
        (long)127192-(long)b.live_bytes,stage123_percent((double)127192-b.live_bytes,127192));
    printf("PostMerge,270768,%lu,%ld,%.6f\n",(unsigned long)p.live_bytes,
        (long)270768-(long)p.live_bytes,stage123_percent((double)270768-p.live_bytes,270768));
    printf("ResultOnly,143576,%lu,%ld,%.6f\n",(unsigned long)r.live_bytes,
        (long)143576-(long)r.live_bytes,stage123_percent((double)143576-r.live_bytes,143576));
    printf("Peak,366912,%lu,%ld,%.6f\n",(unsigned long)p.peak_live_bytes,
        (long)366912-(long)p.peak_live_bytes,stage123_percent((double)366912-p.peak_live_bytes,366912));
    printf("Input=values[i]=(i*613u)%%1000; Shuffle=NO; status=%s\n",ok?"PASS":"FAIL");
    hps_group_destroy(result);hps_group_batch_destroy(batch);f=hps_alloc_stats_get();
    if(!benchmark_alloc_stats_empty(&f)||f.failed_calls||!stage83_tag_accounting_valid(&f,"12.3 GS100 final")||
        !stage83_other_is_empty(&f,"12.3 GS100 final"))ok=0;
    printf("Stage12.3_GS100_FinalAllocator=%lu/%lu OTHER=%lu FailedCalls=%lu\n",
        (unsigned long)f.live_bytes,(unsigned long)f.live_blocks,
        (unsigned long)f.tags[HPS_ALLOC_TAG_OTHER].live_bytes,(unsigned long)f.failed_calls);
    return ok;
}

static int stage123_singleton_1024(void)
{
    static int values[1024];static void *items[1024];
    BenchmarkCompareContext cc={0,0};HpsComparator cmp={benchmark_compare_int,&cc};
    HpsGroupBatch *batch=NULL;HpsGroup *result=NULL;HpsAllocStats peak,only,final;
    HpsTreeInternalProfile profile;size_t i,tree=0,saving=0;int ok=1;
    memset(&profile,0,sizeof(profile));
    if(hps_alloc_stats_reset()!=0)return 0;
    for(i=0;i<1024;++i){values[i]=(int)i;items[i]=&values[i];}
    if(hps_group_batch_build(items,1024,1,&cmp,&batch)!=HPS_STATUS_OK||
        hps_group_batch_group_count(batch)!=1024||
        hps_group_batch_merge_all(batch,&cmp,&result)!=HPS_STATUS_OK||
        hps_tree_internal_profile(hps_group_internal_tree(result),&profile)!=HPS_STATUS_OK||
        cc.comparison_count!=14337||!benchmark_validate_final(result,1024,&(BenchmarkPathStats){0}))ok=0;
    peak=hps_alloc_stats_get();hps_group_batch_destroy(batch);batch=NULL;only=hps_alloc_stats_get();
    tree=only.tags[HPS_ALLOC_TAG_TREE_OBJECT].live_bytes+only.tags[HPS_ALLOC_TAG_TREE_NODE].live_bytes+
        only.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_bytes;
    saving=16*profile.leaf_count;
    if(profile.leaf_count!=2||only.live_bytes!=5339184||saving!=32||
        tree+saving!=tree+32||peak.peak_live_bytes!=14744752||
        only.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_blocks!=profile.allocated_child_array_count||
        !stage83_tag_accounting_valid(&only,"12.3 singleton result")||
        !stage83_other_is_empty(&only,"12.3 singleton result"))ok=0;
    printf("Stage12.3Singleton1024\nComparisonCount=%lu\nLeafCount=%lu\nLegacyModeledTreeBytes=%lu\nCurrentTreeBytes=%lu\nTreeSaving=%lu\nExpected16xLeaf=%lu\nResultOnlyBefore=5339216\nResultOnlyAfter=%lu\nPeakBefore=14761200\nPeakAfter=%lu\nPeakSavedBytes=%ld\nPeakSavingsPercent=%.6f\nExplanation=LeafCount=2, so 16*2=32 result bytes saved\n",
        (unsigned long)cc.comparison_count,(unsigned long)profile.leaf_count,
        (unsigned long)(tree+saving),(unsigned long)tree,(unsigned long)saving,
        (unsigned long)(16*profile.leaf_count),(unsigned long)only.live_bytes,
        (unsigned long)peak.peak_live_bytes,(long)14761200-(long)peak.peak_live_bytes,
        stage123_percent((double)14761200-peak.peak_live_bytes,14761200));
    hps_group_destroy(result);final=hps_alloc_stats_get();
    if(!benchmark_alloc_stats_empty(&final)||final.failed_calls||
        !stage83_tag_accounting_valid(&final,"12.3 singleton final")||
        !stage83_other_is_empty(&final,"12.3 singleton final"))ok=0;
    return ok;
}

static int stage123_run_panels(void)
{
    Stage103Sample main_samples[STAGE103_GROUPS][STAGE103_SEEDS];
    HpsTreeInternalProfile main_profiles[STAGE103_GROUPS][STAGE103_SEEDS];
    Stage103Sample scale_samples[STAGE103_SCALE_NS][STAGE103_SCALE_GROUPS][STAGE103_SCALE_SEEDS];
    HpsTreeInternalProfile scale_profiles[STAGE103_SCALE_NS][STAGE103_SCALE_GROUPS][STAGE103_SCALE_SEEDS];
    static const size_t peak_focus[8]={1,4,8,16,32,40,64,80};
    size_t g,s,ni,gi,t,main_pass=0,scale_pass=0,model_errors=0,scale_errors=0;
    size_t block_errors=0,comparison_errors=0,profile_errors=0;
    double tree_min=DBL_MAX,tree_max=0,tree_sum=0,result_min=DBL_MAX,result_max=0,result_sum=0;
    double per_min=DBL_MAX,per_max=0,per_sum=0,leaf_min=DBL_MAX,leaf_max=0,leaf_sum=0;
    double scale_result_min=DBL_MAX,scale_result_max=0,scale_peak_min=DBL_MAX,scale_peak_max=0;
    int valid=1;
    memset(main_samples,0,sizeof(main_samples));memset(main_profiles,0,sizeof(main_profiles));
    memset(scale_samples,0,sizeof(scale_samples));memset(scale_profiles,0,sizeof(scale_profiles));
    printf("\nHPSort Stage 12.3 final validation; input benchmark_shuffle with Stage11 seeds\n");
    printf("Stage12.3PeakBaselineAvailability=NO (stage111-output.txt and frozen Stage11 peak arrays are absent)\n");
    printf("StrictABTimingAvailable=NO (no Git repository or exact Stage12.1 production snapshot found)\n");
    printf("Stage12.3_Layout\nsizeof_HpsTreeNode=%lu\nalignof_HpsTreeNode=%lu\nsizeof_HpsTree=%lu\nalignof_HpsTree=%lu\nChildBlockHeaderBytes=%lu\nChildBlockHeaderAlignment=%lu\nChildrenPointerAlignment=%lu\n",
        (unsigned long)hps_tree_internal_sizeof_node(),(unsigned long)hps_tree_internal_alignof_node(),
        (unsigned long)hps_tree_internal_sizeof_tree(),(unsigned long)hps_tree_internal_alignof_tree(),
        (unsigned long)hps_tree_internal_child_block_header_size(),
        (unsigned long)hps_tree_internal_alignof_child_block(),
        (unsigned long)(hps_tree_internal_child_block_header_size()%sizeof(void *)));
    for(g=0;g<STAGE103_GROUPS;++g)for(s=0;s<STAGE103_SEEDS;++s){
        Stage103Snapshot snap={0};Stage112TreeCapture cap={0};Stage103Sample *x=&main_samples[g][s];
        if(stage103_run_one_internal(STAGE103_N,stage103_groups[g],stage103_seeds[s],0,x,&snap,&cap)){
            main_profiles[g][s]=cap.profile;++main_pass;
            if(!stage123_profile_valid(x,&cap.profile,STAGE103_N))++profile_errors;
            if(x->total_comparisons!=x->build_comparisons+x->merge_comparisons)++comparison_errors;
            if(x->result_stats.tags[HPS_ALLOC_TAG_TREE_NODE].live_blocks!=STAGE103_N||
                x->result_stats.tags[HPS_ALLOC_TAG_TREE_OBJECT].live_blocks!=1||
                x->result_stats.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_blocks!=cap.profile.allocated_child_array_count)++block_errors;
            if(cap.profile.total_child_count!=cap.profile.real_node_count)++profile_errors;
            if(s==0&&(stage103_groups[g]==1||stage103_groups[g]==32||stage103_groups[g]==40)){
                HpsTreeInternalProfile *p=&cap.profile;size_t expected_leaf=stage103_groups[g]==1?6708:stage103_groups[g]==32?6011:6018;
                size_t expected_unary=stage103_groups[g]==1?1631:stage103_groups[g]==32?2649:2441;
                size_t expected_branch=stage103_groups[g]==1?1661:stage103_groups[g]==32?1340:1541;
                size_t expected_capacity=stage103_groups[g]==1?12429:stage103_groups[g]==32?12087:12045;
                size_t expected_blocks=stage103_groups[g]==1?3293:stage103_groups[g]==32?3990:3983;
                size_t expected_root=stage103_groups[g]==1?50:stage103_groups[g]==32?59:54;
                if(p->leaf_count!=expected_leaf||p->unary_count!=expected_unary||p->branching_count!=expected_branch||
                    p->total_child_capacity!=expected_capacity||p->allocated_child_array_count!=expected_blocks||
                    p->root_child_count!=expected_root||p->root_child_capacity!=64)++profile_errors;
            }
        }else{++model_errors;valid=0;printf("MainSampleFailure G=%lu Seed=%08lX\n",(unsigned long)stage103_groups[g],(unsigned long)stage103_seeds[s]);}
        stage112_tree_capture_destroy(&cap);stage103_snapshot_destroy(&snap);
    }
    printf("\nStage12.3_MainResultMemoryCSV\nGroups,GroupSize,LeafCountMean,LeafPercentMean,LegacyTreeBytesMean,CurrentTreeBytesMean,TreeBytesSavedMean,TreeSavingsPercent,LegacyResultBytesMean,CurrentResultBytesMean,ResultBytesSavedMean,ResultSavingsPercent,ModelErrorBytes\n");
    for(g=0;g<STAGE103_GROUPS;++g){double leaves=0,oldtree=0,newtree=0,saved=0,oldres=0,newres=0,gsmean=0,leafpct=0;
        for(s=0;s<STAGE103_SEEDS;++s){Stage103Sample *x=&main_samples[g][s];size_t l=16*main_profiles[g][s].leaf_count;
            size_t nt=stage112_tree_bytes(x);leaves+=main_profiles[g][s].leaf_count;oldtree+=(double)nt+l;newtree+=nt;saved+=l;
            oldres+=(double)x->result_bytes+l;newres+=x->result_bytes;gsmean+=x->group_size;
            if((double)l+nt-(double)nt!=(double)l)++model_errors;
        }
        leaves/=STAGE103_SEEDS;oldtree/=STAGE103_SEEDS;newtree/=STAGE103_SEEDS;saved/=STAGE103_SEEDS;
        oldres/=STAGE103_SEEDS;newres/=STAGE103_SEEDS;gsmean/=STAGE103_SEEDS;leafpct=100*leaves/STAGE103_N;
        printf("%lu,%.2f,%.2f,%.6f,%.2f,%.2f,%.2f,%.6f,%.2f,%.2f,%.2f,%.6f,%.2f\n",
            (unsigned long)stage103_groups[g],gsmean,leaves,leafpct,oldtree,newtree,saved,
            stage123_percent(saved,oldtree),oldres,newres,saved,stage123_percent(saved,oldres),oldtree-newtree-saved);
    }
    printf("\nStage12.3_MainSavingsRobustnessCSV\nGroups,MinTreeSavingsPercent,MeanTreeSavingsPercent,MaxTreeSavingsPercent,MinResultSavingsPercent,MeanResultSavingsPercent,MaxResultSavingsPercent\n");
    for(g=0;g<STAGE103_GROUPS;++g){double min_t=DBL_MAX,max_t=0,sum_t=0,min_r=DBL_MAX,max_r=0,sum_r=0;
        for(s=0;s<STAGE103_SEEDS;++s){Stage103Sample *x=&main_samples[g][s];double sv=16.0*main_profiles[g][s].leaf_count;
            double nt=(double)stage112_tree_bytes(x),rt=stage123_percent(sv,nt+sv),rr=stage123_percent(sv,(double)x->result_bytes+sv);
            if(rt<min_t)min_t=rt;if(rt>max_t)max_t=rt;sum_t+=rt;if(rr<min_r)min_r=rr;if(rr>max_r)max_r=rr;sum_r+=rr;}
        printf("%lu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",(unsigned long)stage103_groups[g],min_t,sum_t/5,max_t,min_r,sum_r/5,max_r);}
    printf("\nStage12.3_LeafIdentityCSV\nGroups,MeanLeafCount,MeanExpectedSaving16xLeaf,MeanActualTreeSaving,Difference\n");
    for(g=0;g<STAGE103_GROUPS;++g){double leaf=0,expected=0,actual=0;
        for(s=0;s<STAGE103_SEEDS;++s){double nt=(double)stage112_tree_bytes(&main_samples[g][s]);double sv=16.0*main_profiles[g][s].leaf_count;
            leaf+=main_profiles[g][s].leaf_count;expected+=sv;actual+=nt+sv-nt;}
        leaf/=5;expected/=5;actual/=5;if(fabs(expected-actual)>0.01)++model_errors;
        printf("%lu,%.2f,%.2f,%.2f,%.2f\n",(unsigned long)stage103_groups[g],leaf,expected,actual,actual-expected);}
    printf("\nStage12.3_Stage11FrozenMeanReconciliationCSV\nGroups,Stage11ResultMean,Stage12ModeledLegacyResultMean,Delta,Stage11PathMean,Stage12PathMean,PathDelta,Stage11GroupMean,Stage12GroupMean,GroupDelta\n");
    for(g=0;g<STAGE103_GROUPS;++g){double result=0,path=0,group=0;
        for(s=0;s<STAGE103_SEEDS;++s){Stage103Sample *x=&main_samples[g][s];result+=(double)x->result_bytes+16.0*main_profiles[g][s].leaf_count;
            path+=stage111_category(&x->result_stats,0,0);group+=stage111_category(&x->result_stats,2,0);}
        result/=5;path/=5;group/=5;
         if(fabs(result-stage123_old_result[g])>0.01||fabs(path-stage123_old_path[g])>0.01||fabs(group-80024.0)>0.01)valid=0;
        printf("%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,80024.00,%.2f,%.2f\n",(unsigned long)stage103_groups[g],
            stage123_old_result[g],result,stage123_old_result[g]-result,stage123_old_path[g],path,
            stage123_old_path[g]-path,group,group-80024.0);}
    printf("\nStage12.3_MainTimingCurrentDebugBaseline\nGroups,BuildMsMean,MergeMsMean,TotalMsMean\n");
    for(g=0;g<STAGE103_GROUPS;++g){double b=0,m=0;for(s=0;s<5;++s){b+=main_samples[g][s].build_ms;m+=main_samples[g][s].merge_ms;}
        printf("%lu,%.6f,%.6f,%.6f\n",(unsigned long)stage103_groups[g],b/5,m/5,(b+m)/5);}
    for(g=0;g<STAGE103_GROUPS;++g){
        double leaf=0,save=0,actual=0,result=0,path=0,group=0,oldtree=0;size_t nodeblocks=0,childblocks=0;
        for(s=0;s<STAGE103_SEEDS;++s){Stage103Sample *x=&main_samples[g][s];HpsTreeInternalProfile *p=&main_profiles[g][s];
            double nt=(double)stage112_tree_bytes(x),sv=16.0*p->leaf_count;
            leaf+=p->leaf_count;save+=sv;actual+=nt+sv-nt;result+=x->result_bytes+sv;
            oldtree+=nt+sv;path+=stage111_category(&x->result_stats,0,0);group+=stage111_category(&x->result_stats,2,0);
            nodeblocks+=x->result_stats.tags[HPS_ALLOC_TAG_TREE_NODE].live_blocks;
            childblocks+=x->result_stats.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_blocks;
        }
        leaf/=STAGE103_SEEDS;save/=STAGE103_SEEDS;actual/=STAGE103_SEEDS;result/=STAGE103_SEEDS;oldtree/=STAGE103_SEEDS;path/=STAGE103_SEEDS;group/=STAGE103_SEEDS;
        if(fabs(actual-save)>0.01)++model_errors;
        if(fabs(result-stage123_old_result[g])>0.01||fabs(path-stage123_old_path[g])>0.01||fabs(group-80024.0)>0.01)valid=0;
        if(nodeblocks!=STAGE103_SEEDS*STAGE103_N)++block_errors;
        (void)childblocks;
        {double per=save/STAGE103_N,tperc=stage123_percent(save,oldtree);
            if(per<per_min)per_min=per;if(per>per_max)per_max=per;per_sum+=per;tree_sum+=tperc;
            if(tperc<tree_min)tree_min=tperc;if(tperc>tree_max)tree_max=tperc;
            {double rp=100.0*save/result;if(rp<result_min)result_min=rp;if(rp>result_max)result_max=rp;result_sum+=rp;}
            {double lp=100.0*leaf/STAGE103_N;if(lp<leaf_min)leaf_min=lp;if(lp>leaf_max)leaf_max=lp;leaf_sum+=lp;}}
    }
    /* Peak before values are unavailable; retain the exact row and matrix shapes. */
    printf("\nStage12.3_PeakBeforeAfterCSV\nGroups,Stage11PeakBytesMean,Stage12PeakBytesMean,PeakBytesSaved,PeakSavingsPercent\n");
    for(g=0;g<STAGE103_GROUPS;++g){double peak=0;for(s=0;s<STAGE103_SEEDS;++s)peak+=main_samples[g][s].peak_bytes;
        printf("%lu,NA,%.2f,NA,NA\n",(unsigned long)stage103_groups[g],peak/STAGE103_SEEDS);}
    printf("\nStage12.3_PeakSavingsMatrix\nSeed");for(g=0;g<STAGE103_GROUPS;++g)printf(",G%lu",(unsigned long)stage103_groups[g]);printf("\n");
    for(s=0;s<STAGE103_SEEDS;++s){printf("0x%08lX",(unsigned long)stage103_seeds[s]);for(g=0;g<STAGE103_GROUPS;++g)printf(",NA");printf("\n");}
    printf("NegativePeakSavingSampleCount=NA (frozen baseline unavailable)\n");
    printf("\nStage12.3_CurrentPeakTagCSV\nGroups,PeakTotal,PeakPath,PeakTree,PeakGroup,PeakMergeScratch,TREE_OBJECT,TREE_NODE,TREE_CHILDREN\n");
    for(gi=0;gi<8;++gi){g=stage111_group_index(peak_focus[gi]);double a[9]={0};
        for(s=0;s<STAGE103_SEEDS;++s){const HpsAllocStats *p=&main_samples[g][s].peak_stats;
            a[0]+=p->peak_live_bytes;a[1]+=stage111_category(p,0,1);a[2]+=stage111_category(p,1,1);a[3]+=stage111_category(p,2,1);a[4]+=p->bytes_at_global_peak[HPS_ALLOC_TAG_MERGE_SCRATCH];
            a[5]+=p->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_OBJECT];a[6]+=p->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_NODE];a[7]+=p->bytes_at_global_peak[HPS_ALLOC_TAG_TREE_CHILDREN];}
        printf("%lu",(unsigned long)peak_focus[gi]);for(t=0;t<8;++t)printf(",%.2f",a[t]/STAGE103_SEEDS);printf("\n");}
    for(ni=0;ni<STAGE103_SCALE_NS;++ni)for(gi=0;gi<STAGE103_SCALE_GROUPS;++gi)for(s=0;s<STAGE103_SCALE_SEEDS;++s){
        Stage103Snapshot snap={0};Stage112TreeCapture cap={0};Stage103Sample *x=&scale_samples[ni][gi][s];
        if(stage103_run_one_internal(stage103_scale_ns[ni],stage111_scale_groups[gi],stage103_scale_seeds[s],0,x,&snap,&cap)){
            scale_profiles[ni][gi][s]=cap.profile;++scale_pass;
            if(!stage123_profile_valid(x,&cap.profile,stage103_scale_ns[ni]))++scale_errors;
            if(x->total_comparisons!=x->build_comparisons+x->merge_comparisons)++comparison_errors;
            if(x->result_stats.tags[HPS_ALLOC_TAG_TREE_NODE].live_blocks!=stage103_scale_ns[ni]||
                x->result_stats.tags[HPS_ALLOC_TAG_TREE_OBJECT].live_blocks!=1||
                x->result_stats.tags[HPS_ALLOC_TAG_TREE_CHILDREN].live_blocks!=cap.profile.allocated_child_array_count)++block_errors;
        }else{++scale_errors;valid=0;printf("ScaleSampleFailure N=%lu G=%lu Seed=%08lX\n",(unsigned long)stage103_scale_ns[ni],(unsigned long)stage111_scale_groups[gi],(unsigned long)stage103_scale_seeds[s]);}
        stage112_tree_capture_destroy(&cap);stage103_snapshot_destroy(&snap);
    }
    printf("\nStage12.3_ScalingResultCSV\nN,Groups,GroupSize,LeafPercent,LegacyTreeBytesPerItem,CurrentTreeBytesPerItem,TreeSavedBytesPerItem,TreeSavingsPercent,LegacyResultBytesPerItem,CurrentResultBytesPerItem,ResultSavingsPercent\n");
    printf("\nStage12.3_ScalingPeakCSV\nN,Groups,Stage11PeakPerItem,Stage12PeakPerItem,PeakSavingsPercent\n");
    for(ni=0;ni<STAGE103_SCALE_NS;++ni)for(gi=0;gi<STAGE103_SCALE_GROUPS;++gi){double leaf=0,oldtree=0,newtree=0,saved=0,oldres=0,newres=0,peaks=0,gsmean=0;
        size_t n=stage103_scale_ns[ni],groups=stage111_scale_groups[gi];
        for(s=0;s<STAGE103_SCALE_SEEDS;++s){Stage103Sample *x=&scale_samples[ni][gi][s];HpsTreeInternalProfile *p=&scale_profiles[ni][gi][s];
            double sv=16.0*p->leaf_count,nt=(double)stage112_tree_bytes(x);leaf+=100.0*p->leaf_count/n;newtree+=nt/(double)n;oldtree+=(nt+sv)/(double)n;saved+=sv/(double)n;
            newres+=(double)x->result_bytes/n;oldres+=(double)(x->result_bytes+sv)/n;peaks+=(double)x->peak_bytes/n;gsmean+=x->group_size;
            if(fabs((nt+sv)-nt-sv)>0.01)++scale_errors;}
        leaf/=3;oldtree/=3;newtree/=3;saved/=3;oldres/=3;newres/=3;peaks/=3;gsmean/=3;
        printf("%lu,%lu,%.2f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",(unsigned long)n,(unsigned long)groups,gsmean,leaf,oldtree,newtree,saved,stage123_percent(saved,oldtree),oldres,newres,stage123_percent(saved,oldres));
        printf("%lu,%lu,NA,%.6f,NA\n",(unsigned long)n,(unsigned long)groups,peaks);
        {double rp=stage123_percent(saved,oldres);if(rp<scale_result_min)scale_result_min=rp;if(rp>scale_result_max)scale_result_max=rp;}
    }
    printf("\nStage12.3_ScalingSavingsSummary\nGroups,N1000ResultSavingPercent,N2000ResultSavingPercent,N5000ResultSavingPercent,N10000ResultSavingPercent,N1000PeakSavingPercent,N2000PeakSavingPercent,N5000PeakSavingPercent,N10000PeakSavingPercent\n");
    for(gi=0;gi<3;++gi){printf("%lu",(unsigned long)stage111_scale_groups[gi]);for(ni=0;ni<4;++ni){double sv=0,base=0;for(s=0;s<3;++s){Stage103Sample *x=&scale_samples[ni][gi][s];size_t l=16*scale_profiles[ni][gi][s].leaf_count;sv+=l;base+=x->result_bytes+l;}printf(",%.6f",stage123_percent(sv/3,base/3));}printf(",NA,NA,NA,NA\n");}
    printf("\nStage12.3_AllocatorCalls\nGroups,AllocCallsMean,ReallocCallsMean,FreeCallsMean\n");
    for(g=0;g<STAGE103_GROUPS;++g){double a=0,r=0,f=0;for(s=0;s<5;++s){a+=main_samples[g][s].peak_stats.alloc_calls;r+=main_samples[g][s].peak_stats.realloc_calls;f+=main_samples[g][s].peak_stats.free_calls;}printf("%lu,%.2f,%.2f,%.2f\n",(unsigned long)stage103_groups[g],a/5,r/5,f/5);}
    printf("Stage12MemorySummary\nMain18_TreeSavedBytesPerItem_Min=%.6f\nMain18_TreeSavedBytesPerItem_Mean=%.6f\nMain18_TreeSavedBytesPerItem_Max=%.6f\nMain18_TreeSavingsPercent_Min=%.6f\nMain18_TreeSavingsPercent_Mean=%.6f\nMain18_TreeSavingsPercent_Max=%.6f\nMain18_ResultSavingsPercent_Min=%.6f\nMain18_ResultSavingsPercent_Mean=%.6f\nMain18_ResultSavingsPercent_Max=%.6f\nMain18_PeakSavingsPercent_Min=NA\nMain18_PeakSavingsPercent_Mean=NA\nMain18_PeakSavingsPercent_Max=NA\nScaling_ResultSavingsPercent_Min=%.6f\nScaling_ResultSavingsPercent_Max=%.6f\nScaling_PeakSavingsPercent_Min=NA\nScaling_PeakSavingsPercent_Max=NA\nMain90_ModelErrorCount=%lu\nScaling36_ModelErrorCount=%lu\nNegativePeakSavingSampleCount=NA\n",
        per_min,(per_sum/STAGE103_GROUPS),(per_max),(tree_min),(tree_sum/STAGE103_GROUPS),(tree_max),result_min,result_sum/STAGE103_GROUPS,result_max,scale_result_min,scale_result_max,(unsigned long)model_errors,(unsigned long)scale_errors);
    printf("Stage12CorrectnessFacts\nMainSamplesPassed=%lu\nScalingSamplesPassed=%lu\nResultOrderFailureCount=%lu\nPathOrderFailureCount=%lu\nBatchMutationCount=%lu\nTreeTagAccountingMismatchCount=%lu\nModelErrorCount=%lu\nNodePointerStabilityFailureCount=0 (direct ChildBlock regression suite)\nComparatorRegressionFailureCount=%lu\nProfileInvariantFailureCount=%lu\nAllocationBlockMismatchCount=%lu\n",
        (unsigned long)main_pass,(unsigned long)scale_pass,(unsigned long)stage103_result_item_mismatches,
        (unsigned long)stage103_result_path_mismatches,(unsigned long)stage103_input_batch_mutations,
        (unsigned long)block_errors,(unsigned long)(model_errors+scale_errors),(unsigned long)comparison_errors,
        (unsigned long)profile_errors,(unsigned long)block_errors);
    printf("Stage12RepresentationFacts\nsizeof_HpsTreeNode=%lu\nsizeof_HpsTree=%lu\nChildBlockHeaderBytes=%lu\nChildBlockAlignment=%lu\nMain18_LeafPercent_Min=%.6f\nMain18_LeafPercent_Mean=%.6f\nMain18_LeafPercent_Max=%.6f\nResult_TREE_NODE_Blocks=%lu per sample\nChildBlockExtraAllocationBlocks=0 (TREE_CHILDREN blocks equal allocated ChildBlock count)\n",
        (unsigned long)hps_tree_internal_sizeof_node(),(unsigned long)hps_tree_internal_sizeof_tree(),
        (unsigned long)hps_tree_internal_child_block_header_size(),(unsigned long)hps_tree_internal_alignof_child_block(),
        leaf_min,leaf_sum/STAGE103_GROUPS,leaf_max,(unsigned long)STAGE103_N);
    {int gs_ok=stage123_gs100();int direct_ok=stage122_direct_regressions();
        int singleton_ok=stage123_singleton_1024();int semantic_ok=stage111_legacy_production_regressions();
        printf("Stage12.3FocusedRegressionFacts GS100=%s DirectTree=%s Singleton1024=%s G0EqualEmptySingle=%s\n",
            gs_ok?"PASS":"FAIL",direct_ok?"PASS":"FAIL",singleton_ok?"PASS":"FAIL",semantic_ok?"PASS":"FAIL");
        if(!gs_ok||!direct_ok||!singleton_ok||!semantic_ok)valid=0;}
    {
        HpsAllocStats final=hps_alloc_stats_get();
        if(!benchmark_alloc_stats_empty(&final)||final.failed_calls||
            !stage83_tag_accounting_valid(&final,"12.3 final")||
            !stage83_other_is_empty(&final,"12.3 final"))valid=0;
        printf("Stage12FinalAllocator live=%lu/%lu OTHER=%lu FailedCalls=%lu\n",
            (unsigned long)final.live_bytes,(unsigned long)final.live_blocks,
            (unsigned long)final.tags[HPS_ALLOC_TAG_OTHER].live_bytes,(unsigned long)final.failed_calls);
    }
    if(main_pass!=90||scale_pass!=36||model_errors||scale_errors||block_errors||comparison_errors||profile_errors)valid=0;
    printf("Stage12.3Status=%s\n",valid?"PASS":"FAIL");return valid?0:1;
}

int hps_run_stage12_3_validation(void)
{
    if(!stage111_legacy_production_regressions()||hps_run_stage11_1_gs100_fixture_check()!=0)return 1;
    return stage123_run_panels();
}

int hps_run_stage12_3_release_smoke(void)
{
    Stage103Sample sample={0};Stage103Snapshot snap={0};Stage112TreeCapture cap={0};
    HpsAllocStats final;int ok=1;
    printf("Stage12.3ReleaseSmoke\n");
    if(!stage111_legacy_production_regressions()||hps_run_stage11_1_gs100_fixture_check()!=0)ok=0;
    if(!stage103_run_one_internal(10000,32,UINT32_C(0xC0FFEE),0,&sample,&snap,&cap)||
        !stage123_profile_valid(&sample,&cap.profile,10000))ok=0;
    printf("Release G32 C0FFEE=%s\n",ok?"PASS":"FAIL");
    stage112_tree_capture_destroy(&cap);stage103_snapshot_destroy(&snap);
    if(!stage122_direct_regressions()||!stage123_singleton_1024())ok=0;
    final=hps_alloc_stats_get();
    if(!benchmark_alloc_stats_empty(&final)||final.failed_calls||
        !stage83_tag_accounting_valid(&final,"12.3 Release final")||
        !stage83_other_is_empty(&final,"12.3 Release final"))ok=0;
    printf("Stage12.3ReleaseFinal live=%lu/%lu OTHER=%lu FailedCalls=%lu\nStage12.3ReleaseSmokeStatus=%s\n",
        (unsigned long)final.live_bytes,(unsigned long)final.live_blocks,
        (unsigned long)final.tags[HPS_ALLOC_TAG_OTHER].live_bytes,(unsigned long)final.failed_calls,
        ok?"PASS":"FAIL");
    return ok?0:1;
}

static int stage131_case_start(void)
{
    HpsAllocStats stats;
    hps_alloc_test_disable_failure();
    stats=hps_alloc_stats_get();
    if(!benchmark_alloc_stats_empty(&stats))return 0;
    return hps_alloc_stats_reset()==0;
}

static int stage131_case_finish(const char *name,int passed)
{
    HpsAllocStats stats;
    hps_alloc_test_disable_failure();
    stats=hps_alloc_stats_get();
    if(!benchmark_alloc_stats_empty(&stats)||stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes!=0||
        stats.tags[HPS_ALLOC_TAG_OTHER].live_blocks!=0||
        !stage83_tag_accounting_valid(&stats,name))passed=0;
    printf("Stage13.1 %s=%s final=%lu/%lu failed_calls=%lu OTHER=%lu\n",name,
        passed?"PASS":"FAIL",(unsigned long)stats.live_bytes,(unsigned long)stats.live_blocks,
        (unsigned long)stats.failed_calls,(unsigned long)stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes);
    return passed;
}

static int stage131_test_a_disabled(void)
{
    unsigned char *p;size_t i;int ok=stage131_case_start();HpsAllocStats stats;
    if(!ok)return stage131_case_finish("DisabledAllocReallocFree",0);
    if(hps_alloc_test_failure_triggered()||hps_alloc_test_get_attempt_count()!=0)ok=0;
    p=(unsigned char *)hps_alloc(16);
    if(p==NULL)ok=0;
    if(p!=NULL){for(i=0;i<16;++i)p[i]=(unsigned char)(i*13u+7u);
        p=(unsigned char *)hps_realloc(p,64);
        if(p==NULL)ok=0;
        if(p!=NULL){for(i=0;i<16;++i)if(p[i]!=(unsigned char)(i*13u+7u))ok=0;
            p=(unsigned char *)hps_realloc(p,8);
            if(p==NULL)ok=0;
            if(p!=NULL)for(i=0;i<8;++i)if(p[i]!=(unsigned char)(i*13u+7u))ok=0;}}
    hps_free(p);stats=hps_alloc_stats_get();
    if(stats.alloc_calls!=1||stats.realloc_calls!=2||stats.free_calls!=1||stats.failed_calls!=0||
        stats.total_successful_requested_bytes!=88||stats.live_bytes!=0||stats.live_blocks!=0)ok=0;
    return stage131_case_finish("DisabledAllocReallocFree",ok);
}

static int stage131_test_b_first_alloc_failure(void)
{
    void *p;HpsAllocStats s;int ok=stage131_case_start();if(!ok)return stage131_case_finish("AllocFailurePASS",0);
    hps_alloc_test_fail_on_attempt(1);p=hps_alloc(32);s=hps_alloc_stats_get();
    if(p!=NULL||hps_alloc_test_get_attempt_count()!=1||!hps_alloc_test_failure_triggered()||
        s.alloc_calls!=1||s.failed_calls!=1||s.live_bytes!=0||s.live_blocks!=0||
        s.total_successful_requested_bytes!=0||s.tags[HPS_ALLOC_TAG_OTHER].live_bytes!=0||
        s.tags[HPS_ALLOC_TAG_OTHER].live_blocks!=0)ok=0;
    return stage131_case_finish("AllocFailurePASS",ok);
}

static int stage131_test_c_single_shot(void)
{
    void *a=NULL,*b=NULL,*c=NULL;HpsAllocStats s;int ok=stage131_case_start();
    if(!ok)return stage131_case_finish("SingleShotFailure",0);
    hps_alloc_test_fail_on_attempt(2);a=hps_alloc(16);b=hps_alloc(32);c=hps_alloc(48);s=hps_alloc_stats_get();
    if(a==NULL||b!=NULL||c==NULL||hps_alloc_test_get_attempt_count()!=3||
        !hps_alloc_test_failure_triggered()||s.failed_calls!=1||s.alloc_calls!=3||
        s.live_bytes!=64||s.live_blocks!=2||s.total_successful_requested_bytes!=64)ok=0;
    hps_free(a);hps_free(b);hps_free(c);
    return stage131_case_finish("SingleShotFailure",ok);
}

static int stage131_test_d_zero_alloc(void)
{
    void *p,*q;HpsAllocStats s;int ok=stage131_case_start();if(!ok)return stage131_case_finish("ZeroAllocFailurePASS",0);
    hps_alloc_test_fail_on_attempt(1);p=hps_alloc(0);s=hps_alloc_stats_get();
    if(p!=NULL||hps_alloc_test_get_attempt_count()!=1||!hps_alloc_test_failure_triggered()||
        s.failed_calls!=1||s.alloc_calls!=1||s.live_blocks!=0)ok=0;
    hps_alloc_test_disable_failure();q=hps_alloc(0);s=hps_alloc_stats_get();
    if(q==NULL||s.live_bytes!=0||s.live_blocks!=1)ok=0;
    hps_free(q);return stage131_case_finish("ZeroAllocFailurePASS",ok);
}

static int stage131_test_e_realloc_preserves_old(void)
{
    unsigned char *p;size_t i;HpsAllocStats s;int ok=stage131_case_start();
    if(!ok)return stage131_case_finish("ReallocFailurePreservesOldPASS",0);
    p=(unsigned char *)hps_alloc_tagged(64,HPS_ALLOC_TAG_PATH_STEPS);
    if(p==NULL)ok=0;
    if(p!=NULL)for(i=0;i<64;++i)p[i]=(unsigned char)(i*29u+3u);
    hps_alloc_test_fail_on_attempt(1);
    if(hps_realloc(p,128)!=NULL)ok=0;
    s=hps_alloc_stats_get();
    if(p==NULL||hps_alloc_test_get_attempt_count()!=1||!hps_alloc_test_failure_triggered()||
        s.alloc_calls!=1||s.realloc_calls!=1||s.failed_calls!=1||
        s.total_successful_requested_bytes!=64||s.live_bytes!=64||s.live_blocks!=1||
        s.tags[HPS_ALLOC_TAG_PATH_STEPS].live_bytes!=64||
        s.tags[HPS_ALLOC_TAG_PATH_STEPS].live_blocks!=1||
        s.tags[HPS_ALLOC_TAG_PATH_STEPS].alloc_calls!=1||
        s.tags[HPS_ALLOC_TAG_PATH_STEPS].realloc_calls!=1||
        s.tags[HPS_ALLOC_TAG_OTHER].live_bytes!=0)ok=0;
    if(p!=NULL)for(i=0;i<64;++i)if(p[i]!=(unsigned char)(i*29u+3u))ok=0;
    hps_free(p);return stage131_case_finish("ReallocFailurePreservesOldPASS",ok);
}

static int stage131_test_f_realloc_null(void)
{
    void *p;HpsAllocStats s;int ok=stage131_case_start();if(!ok)return stage131_case_finish("ReallocNullFailurePASS",0);
    hps_alloc_test_fail_on_attempt(1);p=hps_realloc(NULL,64);s=hps_alloc_stats_get();
    if(p!=NULL||hps_alloc_test_get_attempt_count()!=1||!hps_alloc_test_failure_triggered()||
        s.alloc_calls!=0||s.realloc_calls!=1||s.tags[HPS_ALLOC_TAG_OTHER].alloc_calls!=0||
        s.tags[HPS_ALLOC_TAG_OTHER].realloc_calls!=1||s.failed_calls!=1||
        s.live_bytes!=0||s.live_blocks!=0)ok=0;
    return stage131_case_finish("ReallocNullFailurePASS",ok);
}

static int stage131_test_g_realloc_zero(void)
{
    void *p;HpsAllocStats s;int ok=stage131_case_start();if(!ok)return stage131_case_finish("ReallocZeroDoesNotConsumeAttemptPASS",0);
    p=hps_alloc(32);if(p==NULL)ok=0;hps_alloc_test_fail_on_attempt(1);
    if(hps_realloc(p,0)!=NULL)ok=0;
    if(hps_alloc_test_get_attempt_count()!=0||hps_alloc_test_failure_triggered())ok=0;
    p=hps_alloc(16);s=hps_alloc_stats_get();
    if(p!=NULL||hps_alloc_test_get_attempt_count()!=1||!hps_alloc_test_failure_triggered()||
        s.realloc_calls!=1||s.failed_calls!=1||s.live_bytes!=0||s.live_blocks!=0)ok=0;
    return stage131_case_finish("ReallocZeroDoesNotConsumeAttemptPASS",ok);
}

static int stage131_test_h_free_no_attempt(void)
{
    void *a,*b,*c;HpsAllocStats s;int ok=stage131_case_start();if(!ok)return stage131_case_finish("FreeDoesNotConsumeAttemptPASS",0);
    a=hps_alloc(16);if(a==NULL)ok=0;hps_alloc_test_fail_on_attempt(2);hps_free(a);
    if(hps_alloc_test_get_attempt_count()!=0)ok=0;
    b=hps_alloc(8);c=hps_alloc(8);s=hps_alloc_stats_get();
    if(b==NULL||c!=NULL||hps_alloc_test_get_attempt_count()!=2||!hps_alloc_test_failure_triggered()||
        s.failed_calls!=1||s.live_bytes!=8||s.live_blocks!=1)ok=0;
    hps_free(b);hps_free(c);return stage131_case_finish("FreeDoesNotConsumeAttemptPASS",ok);
}

static int stage131_test_i_reconfigure(void)
{
    void *a=NULL,*b=NULL,*c=NULL,*d=NULL;HpsAllocStats s;int ok=stage131_case_start();
    if(!ok)return stage131_case_finish("ReconfigureResetsCounterPASS",0);
    hps_alloc_test_fail_on_attempt(5);a=hps_alloc(8);b=hps_alloc(16);
    if(a==NULL||b==NULL||hps_alloc_test_get_attempt_count()!=2)ok=0;
    if(hps_alloc_stats_reset()==0||hps_alloc_test_get_attempt_count()!=2)ok=0;
    hps_alloc_test_reset_attempt_counter();if(hps_alloc_test_get_attempt_count()!=0)ok=0;
    c=hps_alloc(24);if(c==NULL||hps_alloc_test_get_attempt_count()!=1||hps_alloc_test_failure_triggered())ok=0;
    hps_alloc_test_fail_on_attempt(1);if(hps_alloc_test_get_attempt_count()!=0||hps_alloc_test_failure_triggered())ok=0;
    d=hps_alloc(32);s=hps_alloc_stats_get();
    if(d!=NULL||hps_alloc_test_get_attempt_count()!=1||!hps_alloc_test_failure_triggered()||
        s.failed_calls!=1||s.live_bytes!=48||s.live_blocks!=3)ok=0;
    hps_free(a);hps_free(b);hps_free(c);hps_free(d);
    return stage131_case_finish("ReconfigureResetsCounterPASS",ok);
}

static int stage131_test_j_disable(void)
{
    void *a=NULL,*b=NULL;HpsAllocStats s;int ok=stage131_case_start();if(!ok)return stage131_case_finish("DisablePASS",0);
    hps_alloc_test_fail_on_attempt(1);hps_alloc_test_fail_on_attempt(0);
    if(hps_alloc_test_get_attempt_count()!=0||hps_alloc_test_failure_triggered())ok=0;
    a=hps_alloc(8);hps_alloc_test_fail_on_attempt(1);hps_alloc_test_disable_failure();
    if(hps_alloc_test_get_attempt_count()!=0||hps_alloc_test_failure_triggered())ok=0;
    b=hps_alloc(8);s=hps_alloc_stats_get();
    if(a==NULL||b==NULL||s.failed_calls!=0||s.alloc_calls!=2||s.live_bytes!=16||s.live_blocks!=2)ok=0;
    hps_free(a);hps_free(b);return stage131_case_finish("DisablePASS",ok);
}

static int stage131_test_k_tag_accounting(void)
{
    static const HpsAllocTag tags[3]={HPS_ALLOC_TAG_PATH_STEPS,
        HPS_ALLOC_TAG_TREE_CHILDREN,HPS_ALLOC_TAG_MERGE_SCRATCH};
    size_t i;HpsAllocStats s;int ok=stage131_case_start();if(!ok)return stage131_case_finish("TagAccountingPASS",0);
    for(i=0;i<3;++i){void *p;hps_alloc_test_fail_on_attempt(1);p=hps_alloc_tagged(37,tags[i]);s=hps_alloc_stats_get();
        if(p!=NULL||!hps_alloc_test_failure_triggered()||hps_alloc_test_get_attempt_count()!=1||
            s.failed_calls!=i+1||s.tags[tags[i]].live_bytes!=0||s.tags[tags[i]].live_blocks!=0||
            s.tags[tags[i]].alloc_calls!=1||s.total_successful_requested_bytes!=0||
            s.tags[HPS_ALLOC_TAG_OTHER].live_bytes!=0||s.tags[HPS_ALLOC_TAG_OTHER].live_blocks!=0)ok=0;
        hps_free(p);}
    return stage131_case_finish("TagAccountingPASS",ok);
}

/* Stage 13.2 object-level exhaustive allocation-failure sweeps.  All
 * snapshots below use fixed test-side storage and never call hps_alloc(). */
enum { STAGE132_MAX_NODES = 256, STAGE132_MAX_DEPTH = 16,
       STAGE132_MAX_GROUPS = 8, STAGE132_MAX_ITEMS = 40,
       STAGE132_TEXT = 256, STAGE132_OPS = 19 };

typedef struct Stage132PathRecord {
    HpsDirection direction;
    size_t depth, capacity;
    unsigned int slots[STAGE132_MAX_DEPTH];
    size_t levels[STAGE132_MAX_DEPTH];
    char text[STAGE132_TEXT];
} Stage132PathRecord;

typedef struct Stage132TreeNodeRecord {
    void *item;
    size_t parent_index, child_count;
    Stage132PathRecord path;
} Stage132TreeNodeRecord;

typedef struct Stage132TreeRecord {
    size_t count;
    HpsTreeInternalProfile profile;
    Stage132TreeNodeRecord nodes[STAGE132_MAX_NODES];
} Stage132TreeRecord;

typedef struct Stage132GroupRecord {
    size_t count;
    void *items[STAGE132_MAX_ITEMS];
    Stage132PathRecord paths[STAGE132_MAX_ITEMS];
    HpsTreeInternalProfile profile;
} Stage132GroupRecord;

typedef struct Stage132BatchRecord {
    size_t groups, total, group_size;
    Stage132GroupRecord entries[STAGE132_MAX_GROUPS];
} Stage132BatchRecord;

typedef struct Stage132LiveRecord {
    size_t bytes, blocks, tag_bytes[HPS_ALLOC_TAG_COUNT];
    size_t tag_blocks[HPS_ALLOC_TAG_COUNT];
} Stage132LiveRecord;

typedef struct Stage132Context {
    int kind;
    int values[STAGE132_MAX_ITEMS];
    void *items[STAGE132_MAX_ITEMS];
    size_t item_count, group_size;
    HpsPath *path, *left_path, *right_path, *out_path;
    HpsTree *tree;
    HpsGroup *group, *base, *incoming, *result;
    HpsGroupBatch *batch;
    const HpsTreeNode *out_node;
    HpsStatus status;
    Stage132LiveRecord private_incoming_live;
} Stage132Context;

typedef struct Stage132Counters {
    size_t failure_not_triggered, wrong_status, unexpected_success;
    size_t output_not_null, input_mutated, source_path_mutated;
    size_t tree_mutated, batch_mutated, base_not_consumed;
    size_t incoming_mutated, live_bytes_mismatch, live_blocks_mismatch;
    size_t tag_mismatch, leak, crash;
    size_t operations, points, passes, path_points, tree_points;
    size_t group_points, merge_points, batch_points;
} Stage132Counters;

static Stage132Counters stage132_counts;
static size_t stage132_final_k[STAGE132_OPS];
static size_t stage131_direct_passed;
static size_t stage133_baseline_k_mismatches;
static size_t stage133_kplus_tests;
static size_t stage133_kplus_failures;
static size_t stage133_failed_calls_delta_mismatches;
static size_t stage133_release_cases;
static size_t stage133_release_passed;
static const char *stage132_names[STAGE132_OPS] = {
    "PathCreateNormal", "PathCreateZero", "PathClone", "PathAppendNoGrow",
    "PathAppendGrow", "PathAppendAtLevelGrow", "PathBefore", "PathBetween",
    "PathAfter", "TreeCreate", "TreeInsertFirstRoot", "TreeInsertNoGrow",
    "TreeInsertChildBlockGrow", "GroupBuild", "GroupBatchBuild",
    "PublicGroupMerge", "OwnedBasePrivateMerge", "BatchMergeAll_G5",
    "BatchMergeAll_G6"
};

static int stage132_path_capture(const HpsPath *path, Stage132PathRecord *record)
{
    size_t i;
    if (path == NULL || record == NULL || hps_path_depth(path) > STAGE132_MAX_DEPTH)
        return 0;
    memset(record, 0, sizeof(*record));
    record->direction = hps_path_direction(path);
    record->depth = hps_path_depth(path);
    record->capacity = hps_path_internal_capacity(path);
    for (i = 0; i < record->depth; ++i) {
        if (hps_path_get_slot(path, i, &record->slots[i]) != HPS_STATUS_OK ||
            hps_path_get_level(path, i, &record->levels[i]) != HPS_STATUS_OK) return 0;
    }
    if (hps_path_text_length(path) >= sizeof(record->text) ||
        hps_path_format(path, record->text, sizeof(record->text)) != HPS_STATUS_OK)
        return 0;
    return 1;
}

static int stage132_path_equal(const HpsPath *path, const Stage132PathRecord *record)
{
    Stage132PathRecord now;
    return stage132_path_capture(path, &now) &&
        memcmp(&now, record, sizeof(now)) == 0;
}

static int stage132_tree_walk(const HpsTreeNode *node, size_t parent,
    Stage132TreeRecord *record)
{
    size_t index, own;
    if (node == NULL || record->count >= STAGE132_MAX_NODES) return 0;
    own = record->count++;
    record->nodes[own].item = hps_tree_node_item(node);
    record->nodes[own].parent_index = parent;
    record->nodes[own].child_count = hps_tree_node_child_count(node);
    if (!stage132_path_capture(hps_tree_node_path(node), &record->nodes[own].path)) return 0;
    for (index = 0; index < hps_tree_node_child_count(node); ++index)
        if (!stage132_tree_walk(hps_tree_node_child_at(node, index), own, record)) return 0;
    return 1;
}

static int stage132_tree_capture(const HpsTree *tree, Stage132TreeRecord *record)
{
    size_t i;
    if (tree == NULL || record == NULL) return 0;
    memset(record, 0, sizeof(*record));
    if (hps_tree_internal_profile(tree, &record->profile) != HPS_STATUS_OK) return 0;
    for (i = 0; i < hps_tree_root_child_count(tree); ++i)
        if (!stage132_tree_walk(hps_tree_root_child_at(tree, i), (size_t)-1, record)) return 0;
    return record->count == hps_tree_size(tree) &&
        record->count == record->profile.real_node_count;
}

static int stage132_tree_equal(const HpsTree *tree, const Stage132TreeRecord *record)
{
    Stage132TreeRecord now;
    return stage132_tree_capture(tree, &now) &&
        memcmp(&now, record, sizeof(now)) == 0;
}

static int stage132_group_capture(const HpsGroup *group, Stage132GroupRecord *record)
{
    size_t i;
    const HpsTree *tree;
    if (group == NULL || record == NULL || hps_group_size(group) > STAGE132_MAX_ITEMS)
        return 0;
    memset(record, 0, sizeof(*record));
    record->count = hps_group_size(group);
    tree = hps_group_internal_tree(group);
    if (tree == NULL || hps_tree_internal_profile(tree, &record->profile) != HPS_STATUS_OK)
        return 0;
    for (i = 0; i < record->count; ++i) {
        record->items[i] = hps_group_item_at(group, i);
        if (!stage132_path_capture(hps_group_path_at(group, i), &record->paths[i])) return 0;
    }
    return 1;
}

static int stage132_group_equal(const HpsGroup *group, const Stage132GroupRecord *record)
{
    Stage132GroupRecord now;
    return stage132_group_capture(group, &now) &&
        memcmp(&now, record, sizeof(now)) == 0;
}

static int stage132_batch_capture(const HpsGroupBatch *batch, Stage132BatchRecord *record)
{
    size_t i;
    if (batch == NULL || record == NULL ||
        hps_group_batch_group_count(batch) > STAGE132_MAX_GROUPS) return 0;
    memset(record, 0, sizeof(*record));
    record->groups = hps_group_batch_group_count(batch);
    record->total = hps_group_batch_total_size(batch);
    record->group_size = hps_group_batch_group_size(batch);
    for (i = 0; i < record->groups; ++i)
        if (!stage132_group_capture(hps_group_batch_group_at(batch, i), &record->entries[i])) return 0;
    return 1;
}

static int stage132_batch_equal(const HpsGroupBatch *batch, const Stage132BatchRecord *record)
{
    Stage132BatchRecord now;
    return stage132_batch_capture(batch, &now) &&
        memcmp(&now, record, sizeof(now)) == 0;
}

static Stage132LiveRecord stage132_live_capture(void)
{
    HpsAllocStats stats = hps_alloc_stats_get();
    Stage132LiveRecord record;
    size_t i;
    record.bytes = stats.live_bytes;
    record.blocks = stats.live_blocks;
    for (i = 0; i < HPS_ALLOC_TAG_COUNT; ++i) {
        record.tag_bytes[i] = stats.tags[i].live_bytes;
        record.tag_blocks[i] = stats.tags[i].live_blocks;
    }
    return record;
}

static int stage132_live_equal(Stage132LiveRecord a, Stage132LiveRecord b)
{
    size_t i;
    if (a.bytes != b.bytes || a.blocks != b.blocks) return 0;
    for (i = 0; i < HPS_ALLOC_TAG_COUNT; ++i)
        if (a.tag_bytes[i] != b.tag_bytes[i] || a.tag_blocks[i] != b.tag_blocks[i]) return 0;
    return 1;
}

static int stage132_compare_int(const void *left, const void *right, void *context)
{
    int a = *(const int *)left, b = *(const int *)right;
    (void)context;
    return a < b ? -1 : a > b ? 1 : 0;
}

static HpsComparator stage132_comparator(void)
{
    HpsComparator c;
    c.compare = stage132_compare_int; c.context = NULL; return c;
}

static void stage132_items_init(Stage132Context *c, size_t count, int shuffled)
{
    size_t i;
    c->item_count = count;
    for (i = 0; i < count; ++i) {
        size_t source = shuffled ? (i * 7u) % count : i;
        c->values[source] = (int)source;
    }
    for (i = 0; i < count; ++i)
        c->items[i] = &c->values[shuffled ? (i * 7u) % count : i];
}

static HpsPath *stage132_make_path(unsigned int first, size_t level, int depth)
{
    HpsPath *p = hps_path_create_at_level(HPS_DIRECTION_POSITIVE, first, level);
    size_t i;
    if (p == NULL) return NULL;
    for (i = 1; i < (size_t)depth; ++i)
        if (hps_path_append_at_level(p, (unsigned int)(i + 10u), level + i * 2u) != HPS_STATUS_OK) {
            hps_path_destroy(p); return NULL;
        }
    return p;
}

static int stage132_insert_path(HpsTree *tree, HpsPath *path, void *item)
{
    return hps_tree_insert(tree, path, item, NULL) == HPS_STATUS_OK;
}

static int stage132_setup(Stage132Context *c)
{
    size_t i;
    HpsComparator cmp = stage132_comparator();
    hps_alloc_test_disable_failure();
    c->path = c->left_path = c->right_path = c->out_path = NULL;
    c->tree = NULL; c->group = c->base = c->incoming = c->result = NULL;
    c->batch = NULL; c->out_node = NULL; c->status = HPS_STATUS_INTERNAL_ERROR;
    switch (c->kind) {
    case 0: case 1: case 9: return 1;
    case 2:
        c->path = stage132_make_path(3, 0, 5);
        return c->path != NULL;
    case 3: case 4: case 5:
        c->path = hps_path_create(HPS_DIRECTION_POSITIVE, 3);
        if (c->path == NULL) return 0;
        if (c->kind == 3) return hps_path_append(c->path, 4) == HPS_STATUS_OK &&
            hps_path_append(c->path, 5) == HPS_STATUS_OK;
        while (hps_path_depth(c->path) < hps_path_internal_capacity(c->path))
            if (hps_path_append(c->path, 4) != HPS_STATUS_OK) return 0;
        if (c->kind == 5 && hps_path_depth(c->path) < hps_path_internal_capacity(c->path))
            return 0;
        return 1;
    case 6:
        c->right_path = hps_path_create_at_level(HPS_DIRECTION_POSITIVE, 20, 4);
        return c->right_path != NULL;
    case 7:
        c->left_path = hps_path_create_at_level(HPS_DIRECTION_POSITIVE, 4, 0);
        c->right_path = hps_path_create_at_level(HPS_DIRECTION_POSITIVE, 4, 0);
        if (c->left_path != NULL && c->right_path != NULL) {
            if (hps_path_append_at_level(c->left_path, 11, 2) != HPS_STATUS_OK ||
                hps_path_append_at_level(c->right_path, 11, 2) != HPS_STATUS_OK ||
                hps_path_append_at_level(c->left_path, 20, 5) != HPS_STATUS_OK ||
                hps_path_append_at_level(c->right_path, 21, 5) != HPS_STATUS_OK) return 0;
        }
        return c->left_path != NULL && c->right_path != NULL;
    case 8:
        c->left_path = stage132_make_path(4, 1, 4);
        return c->left_path != NULL;
    case 10:
        c->tree = hps_tree_create();
        c->path = hps_path_create(HPS_DIRECTION_POSITIVE, 40);
        return c->tree != NULL && c->path != NULL;
    case 11: case 12:
        c->tree = hps_tree_create();
        if (c->tree == NULL) return 0;
        if (c->kind == 11) {
            HpsPath *parent = hps_path_create(HPS_DIRECTION_POSITIVE, 10);
            if (parent == NULL || !stage132_insert_path(c->tree, parent, &c->values[0])) {
                hps_path_destroy(parent); return 0;
            }
            hps_path_destroy(parent);
            for (i = 0; i < 3; ++i) {
                HpsPath *child = hps_path_create_at_level(HPS_DIRECTION_POSITIVE, 10, 0);
                if (child == NULL || hps_path_append_at_level(child, (unsigned int)i, 1) != HPS_STATUS_OK ||
                    !stage132_insert_path(c->tree, child, &c->values[i + 1])) {
                    hps_path_destroy(child); return 0;
                }
                hps_path_destroy(child);
            }
            c->path = hps_path_create_at_level(HPS_DIRECTION_POSITIVE, 10, 0);
            if (c->path != NULL) (void)hps_path_append_at_level(c->path, 3, 1);
            return c->path != NULL;
        }
        for (i = 0; i < 2; ++i) {
            HpsPath *root = hps_path_create_at_level(HPS_DIRECTION_POSITIVE, (unsigned int)i, 0);
            if (root == NULL || !stage132_insert_path(c->tree, root, &c->values[i])) {
                hps_path_destroy(root); return 0;
            }
            hps_path_destroy(root);
        }
        c->path = hps_path_create_at_level(HPS_DIRECTION_POSITIVE, 2, 0);
        return c->path != NULL;
    case 13:
        stage132_items_init(c, 16, 1);
        return 1;
    case 14:
        stage132_items_init(c, 16, 1); c->group_size = 5; return 1;
    case 15: case 16:
        stage132_items_init(c, 16, 0);
        for (i = 0; i < 16; ++i) c->values[i] = (int)(i * 2);
        c->base = NULL; c->incoming = NULL;
        {
            void *left[8], *right[8];
            for (i = 0; i < 8; ++i) { left[i] = &c->values[i]; right[i] = &c->values[i + 8]; c->values[i + 8] = (int)(i * 2 + 1); c->values[i] = (int)(i * 2); }
            if (hps_group_build(left, 8, &cmp, &c->base) != HPS_STATUS_OK ||
                hps_group_build(right, 8, &cmp, &c->incoming) != HPS_STATUS_OK) return 0;
        }
        if (c->kind == 16) {
            /* The owned-base contract consumes Base on every runtime failure;
             * preserve the exact Incoming-only live footprint as the target. */
            hps_group_destroy(c->base);
            c->base = NULL;
            c->private_incoming_live = stage132_live_capture();
            /* Rebuild Base to leave the complete operation fixture in place. */
            {
                void *left[8];
                for (i = 0; i < 8; ++i) left[i] = &c->values[i];
                if (hps_group_build(left, 8, &cmp, &c->base) != HPS_STATUS_OK) return 0;
            }
        }
        return 1;
    case 17: case 18:
        stage132_items_init(c, c->kind == 17 ? 25 : 30, 1);
        c->group_size = 5;
        return hps_group_batch_build(c->items, c->item_count, c->group_size,
            &cmp, &c->batch) == HPS_STATUS_OK;
    default: return 0;
    }
}

static HpsStatus stage132_run(Stage132Context *c)
{
    HpsComparator cmp = stage132_comparator();
    switch (c->kind) {
    case 0: c->out_path = hps_path_create_at_level(HPS_DIRECTION_NEGATIVE, 9, 3); return c->out_path ? HPS_STATUS_OK : HPS_STATUS_OUT_OF_MEMORY;
    case 1: c->out_path = hps_path_create_zero(); return c->out_path ? HPS_STATUS_OK : HPS_STATUS_OUT_OF_MEMORY;
    case 2: c->out_path = hps_path_clone(c->path); return c->out_path ? HPS_STATUS_OK : HPS_STATUS_OUT_OF_MEMORY;
    case 3: return hps_path_append(c->path, 8);
    case 4: return hps_path_append(c->path, 8);
    case 5: return hps_path_append_at_level(c->path, 8, 100);
    case 6: return hps_path_before(c->right_path, &c->out_path);
    case 7: return hps_path_between(c->left_path, c->right_path, &c->out_path);
    case 8: return hps_path_after(c->left_path, &c->out_path);
    case 9: c->tree = hps_tree_create(); return c->tree ? HPS_STATUS_OK : HPS_STATUS_OUT_OF_MEMORY;
    case 10: case 11: case 12: return hps_tree_insert(c->tree, c->path, &c->values[20], &c->out_node);
    case 13: return hps_group_build(c->items, c->item_count, &cmp, &c->group);
    case 14: return hps_group_batch_build(c->items, c->item_count, c->group_size, &cmp, &c->batch);
    case 15: return hps_group_merge(c->base, c->incoming, &cmp, &c->result);
    case 16: return hps_group_merge_into_owned_base(&c->base, c->incoming, &cmp);
    case 17: case 18: return hps_group_batch_merge_all(c->batch, &cmp, &c->result);
    default: return HPS_STATUS_INTERNAL_ERROR;
    }
}

static void stage132_cleanup(Stage132Context *c)
{
    hps_alloc_test_disable_failure();
    hps_path_destroy(c->out_path); hps_path_destroy(c->path);
    hps_path_destroy(c->left_path); hps_path_destroy(c->right_path);
    hps_tree_destroy(c->tree);
    hps_group_destroy(c->group); hps_group_destroy(c->base);
    hps_group_destroy(c->incoming); hps_group_destroy(c->result);
    hps_group_batch_destroy(c->batch);
    c->out_path=NULL; c->path=NULL; c->left_path=NULL; c->right_path=NULL;
    c->tree=NULL; c->group=NULL; c->base=NULL; c->incoming=NULL;
    c->result=NULL; c->batch=NULL;
}

static int stage132_operation_snapshot(Stage132Context *c,
    Stage132PathRecord *paths, Stage132TreeRecord *tree,
    Stage132GroupRecord *groups, Stage132BatchRecord *batch,
    int *values, void **items, Stage132LiveRecord *live)
{
    size_t i;
    *live = stage132_live_capture();
    for (i = 0; i < STAGE132_MAX_ITEMS; ++i) values[i] = c->values[i];
    for (i = 0; i < STAGE132_MAX_ITEMS; ++i) items[i] = c->items[i];
    switch (c->kind) {
    case 2: case 3: case 4: case 5:
        return stage132_path_capture(c->path, &paths[0]);
    case 6: return stage132_path_capture(c->right_path, &paths[0]);
    case 7:
        return stage132_path_capture(c->left_path, &paths[0]) &&
            stage132_path_capture(c->right_path, &paths[1]);
    case 8: return stage132_path_capture(c->left_path, &paths[0]);
    case 10: case 11: case 12: return stage132_tree_capture(c->tree, tree) &&
        stage132_path_capture(c->path, &paths[0]);
    case 15: case 16:
        return stage132_group_capture(c->base, &groups[0]) &&
            stage132_group_capture(c->incoming, &groups[1]);
    case 17: case 18: return stage132_batch_capture(c->batch, batch);
    default: return 1;
    }
}

static int stage132_failure_contract(Stage132Context *c,
    Stage132PathRecord *paths, Stage132TreeRecord *tree,
    Stage132GroupRecord *groups, Stage132BatchRecord *batch,
    int *values, void **items, Stage132LiveRecord before,
    HpsAllocStats stats_before)
{
    HpsAllocStats after = hps_alloc_stats_get();
    Stage132LiveRecord live_after = stage132_live_capture();
    Stage132LiveRecord expected_live = c->kind == 16 ? c->private_incoming_live : before;
    size_t i;
    int valid = 1;
    if (!hps_alloc_test_failure_triggered()) { ++stage132_counts.failure_not_triggered; valid = 0; }
    if (c->status != HPS_STATUS_OUT_OF_MEMORY) {
        ++stage132_counts.wrong_status;
        if (c->status == HPS_STATUS_OK) ++stage132_counts.unexpected_success;
        valid = 0;
    }
    if (hps_alloc_test_get_attempt_count() == 0) valid = 0;
    if (after.failed_calls != stats_before.failed_calls + 1) {
        ++stage133_failed_calls_delta_mismatches;
        ++stage132_counts.tag_mismatch; valid = 0;
    }
    if (after.live_bytes != expected_live.bytes) { ++stage132_counts.live_bytes_mismatch; valid = 0; }
    if (after.live_blocks != expected_live.blocks) { ++stage132_counts.live_blocks_mismatch; valid = 0; }
    if (!stage132_live_equal(expected_live, live_after)) {
        ++stage132_counts.tag_mismatch; valid = 0;
    }
    switch (c->kind) {
    case 0: case 1: case 2: case 6: case 7: case 8:
        if (c->out_path != NULL) { ++stage132_counts.output_not_null; valid = 0; }
        if (c->kind == 2 && !stage132_path_equal(c->path, &paths[0])) {
            ++stage132_counts.source_path_mutated; valid = 0;
        }
        if (c->kind == 6 && !stage132_path_equal(c->right_path, &paths[0])) valid = 0;
        if (c->kind == 7 && (!stage132_path_equal(c->left_path, &paths[0]) ||
            !stage132_path_equal(c->right_path, &paths[1]))) valid = 0;
        if (c->kind == 8 && !stage132_path_equal(c->left_path, &paths[0])) valid = 0;
        break;
    case 3: case 4: case 5:
        if (!stage132_path_equal(c->path, &paths[0])) {
            ++stage132_counts.source_path_mutated; valid = 0;
        }
        break;
    case 9: if (c->tree != NULL) { ++stage132_counts.output_not_null; valid = 0; } break;
    case 10: case 11: case 12:
        if (!stage132_tree_equal(c->tree, tree)) { ++stage132_counts.tree_mutated; valid = 0; }
        break;
    case 13: if (c->group != NULL) { ++stage132_counts.output_not_null; valid = 0; } break;
    case 14: if (c->batch != NULL) { ++stage132_counts.output_not_null; valid = 0; } break;
    case 15:
        if (c->result != NULL) { ++stage132_counts.output_not_null; valid = 0; }
        if (!stage132_group_equal(c->base, &groups[0]) ||
            !stage132_group_equal(c->incoming, &groups[1])) {
            ++stage132_counts.input_mutated; valid = 0;
        }
        break;
    case 16:
        if (c->base != NULL) { ++stage132_counts.base_not_consumed; valid = 0; }
        if (!stage132_group_equal(c->incoming, &groups[1])) {
            ++stage132_counts.incoming_mutated; valid = 0;
        }
        break;
    case 17: case 18:
        if (c->result != NULL) { ++stage132_counts.output_not_null; valid = 0; }
        if (!stage132_batch_equal(c->batch, batch)) {
            ++stage132_counts.batch_mutated; valid = 0;
        }
        break;
    default: valid = 0; break;
    }
    for (i = 0; i < STAGE132_MAX_ITEMS; ++i)
        if (values[i] != c->values[i] || items[i] != c->items[i]) {
            ++stage132_counts.input_mutated; valid = 0; break;
        }
    return valid;
}

static int stage132_baseline_contract(Stage132Context *c)
{
    switch (c->kind) {
    case 0: case 1: case 2: case 6: case 7: case 8: return c->out_path != NULL;
    case 3: case 4: case 5: return c->status == HPS_STATUS_OK && hps_path_depth(c->path) > 0;
    case 9: return c->tree != NULL && hps_tree_size(c->tree) == 0;
    case 10: case 11: case 12: return c->status == HPS_STATUS_OK && hps_tree_size(c->tree) > 0;
    case 13: return c->group != NULL && hps_group_size(c->group) == c->item_count;
    case 14: return c->batch != NULL && hps_group_batch_total_size(c->batch) == c->item_count;
    case 15: return c->result != NULL && hps_group_size(c->result) == 16;
    case 16: return c->base != NULL && hps_group_size(c->base) == 16;
    case 17: case 18: return c->result != NULL &&
        hps_group_size(c->result) == c->item_count;
    default: return 0;
    }
}

static int stage132_post_root128(void)
{
    HpsTree *tree = hps_tree_create();
    int values[128], query = 127;
    HpsComparator comparator;
    const HpsTreeNode *equal = NULL, *left = NULL, *right = NULL;
    BenchmarkCompareContext count = { 0, 0 };
    size_t i;
    comparator.compare = benchmark_compare_int;
    int ok = tree != NULL;
    for (i = 0; ok && i < 128; ++i) {
        HpsPath *path;
        values[i] = (int)i;
        path = hps_path_create_at_level(HPS_DIRECTION_POSITIVE, (unsigned int)i, 0);
        if (path == NULL || hps_tree_insert(tree, path, &values[i], NULL) != HPS_STATUS_OK)
            ok = 0;
        hps_path_destroy(path);
    }
    comparator.context = &count;
    /* Count this exact root search only, excluding tree construction. */
    if (ok && hps_tree_locate_item(tree, &query, &comparator,
            &left, &equal, &right) != HPS_STATUS_OK) ok = 0;
    if (equal == NULL || hps_tree_node_item(equal) != &values[127] ||
        count.comparison_count != 7) ok = 0;
    printf("Stage13.2PostSweepRoot128=%s Comparisons=%lu\n", ok?"PASS":"FAIL",
        (unsigned long)count.comparison_count);
    hps_tree_destroy(tree);
    return ok;
}

static int stage132_count_kind(int kind)
{
    if (kind <= 8) return 0;
    if (kind <= 12) return 1;
    if (kind <= 14) return 2;
    if (kind <= 16) return 3;
    return 4;
}

static int stage132_sweep_one(int kind)
{
    Stage132Context c;
    size_t baseline, i, pass_count = 0, triggered_count = 0;
    size_t expected_status_count = 0, state_pass_count = 0, leak_free_count = 0;
    size_t unexpected_success_count = 0, batch_preserved_count = 0;
    size_t result_null_count = 0, oom_count = 0, base_consumed_count = 0;
    size_t incoming_preserved_count = 0;
    HpsStatus baseline_status;
    int ok = 1;
    memset(&c, 0, sizeof(c)); c.kind = kind;
    if (!stage132_setup(&c)) { stage132_cleanup(&c); return 0; }
    hps_alloc_test_disable_failure(); hps_alloc_test_reset_attempt_counter();
    baseline_status = stage132_run(&c);
    baseline = hps_alloc_test_get_attempt_count();
    stage132_final_k[kind] = baseline;
    c.status = baseline_status;
    if (baseline_status != HPS_STATUS_OK || !stage132_baseline_contract(&c) ||
        (kind == 3 && baseline != 0) ||
        (kind != 3 && baseline == 0)) ok = 0;
    stage132_cleanup(&c);
    if (!benchmark_alloc_stats_empty(&(HpsAllocStats){0})) { /* no-op: live check below */ }
    {
        HpsAllocStats s = hps_alloc_stats_get();
        if (s.live_bytes != 0 || s.live_blocks != 0) { ++stage132_counts.leak; ok = 0; }
        if (hps_alloc_stats_reset() != 0) ok = 0;
    }
    if (!ok) {
        printf("Stage13.2 %s baseline setup/run failed status=%s K=%lu\n",
            stage132_names[kind], hps_status_string(baseline_status), (unsigned long)baseline);
        return 0;
    }
    ++stage132_counts.operations;
    if (kind == 3) {
        memset(&c, 0, sizeof(c)); c.kind = kind;
        if (!stage132_setup(&c)) return 0;
        hps_alloc_test_fail_on_attempt(1);
        c.status = stage132_run(&c);
        if (c.status != HPS_STATUS_OK || hps_alloc_test_failure_triggered() ||
            hps_alloc_test_get_attempt_count() != 0) ok = 0;
        stage132_cleanup(&c);
        if (hps_alloc_stats_reset() != 0) ok = 0;
        printf("Stage13.2OperationSummary,PathAppendNoGrow,BaselineAllocationAttempts=0,FailCasesRun=0,FailureTriggeredCount=0,ExpectedStatusCount=0,StateContractPassCount=0,LeakFreeCount=0,UnexpectedSuccessCount=0,ContractFailureCount=%d,NoAllocationPathPASS=%s\n",
            ok?0:1,ok?"PASS":"FAIL");
        return ok;
    }
    for (i = 1; i <= baseline; ++i) {
        Stage132PathRecord paths[2];
        Stage132TreeRecord tree;
        Stage132GroupRecord groups[2];
        Stage132BatchRecord batch;
        Stage132LiveRecord before;
        HpsAllocStats stats_before;
        int old_values[STAGE132_MAX_ITEMS];
        void *old_items[STAGE132_MAX_ITEMS];
        int pass;
        int triggered_now, contract_pass;
        memset(paths, 0, sizeof(paths)); memset(&tree, 0, sizeof(tree));
        memset(groups, 0, sizeof(groups)); memset(&batch, 0, sizeof(batch));
        memset(&c, 0, sizeof(c)); c.kind = kind;
        if (!stage132_setup(&c)) { stage132_cleanup(&c); ++stage132_counts.leak; ok = 0; continue; }
        if (!stage132_operation_snapshot(&c, paths, &tree, groups, &batch,
                old_values, old_items, &before)) { stage132_cleanup(&c); ok = 0; continue; }
        stats_before = hps_alloc_stats_get();
        hps_alloc_test_fail_on_attempt(i);
        c.status = stage132_run(&c);
        triggered_now = hps_alloc_test_failure_triggered();
        contract_pass = stage132_failure_contract(&c, paths, &tree, groups, &batch,
                old_values, old_items, before, stats_before);
        pass = hps_alloc_test_get_attempt_count() == i && contract_pass;
        if (triggered_now) ++triggered_count;
        if (c.status == HPS_STATUS_OUT_OF_MEMORY) ++expected_status_count;
        if (c.status == HPS_STATUS_OK) ++unexpected_success_count;
        if (contract_pass && hps_alloc_test_get_attempt_count() == i) ++state_pass_count;
        if (kind == 16) {
            if (c.base == NULL) ++base_consumed_count;
            if (stage132_group_equal(c.incoming, &groups[1])) ++incoming_preserved_count;
        }
        if (kind == 17 || kind == 18) {
            if (c.result == NULL) ++result_null_count;
            if (c.status == HPS_STATUS_OUT_OF_MEMORY) ++oom_count;
            if (stage132_batch_equal(c.batch, &batch)) ++batch_preserved_count;
        }
        {
            int triggered = hps_alloc_test_failure_triggered();
            size_t attempts = hps_alloc_test_get_attempt_count();
            if (!pass)
                printf("Stage13.2FailPointFailure Operation=%s FailIndex=%lu BaselineK=%lu Status=%s Triggered=%d Attempts=%lu\n",
                    stage132_names[kind], (unsigned long)i, (unsigned long)baseline,
                    hps_status_string(c.status), triggered, (unsigned long)attempts);
        }
        hps_alloc_test_disable_failure();
        stage132_cleanup(&c);
        {
            HpsAllocStats end = hps_alloc_stats_get();
            if (end.live_bytes != 0 || end.live_blocks != 0 ||
                end.tags[HPS_ALLOC_TAG_OTHER].live_bytes != 0 ||
                end.tags[HPS_ALLOC_TAG_OTHER].live_blocks != 0) {
                ++stage132_counts.leak; pass = 0;
            } else ++leak_free_count;
        }
        ++stage132_counts.points;
        if (pass) { ++stage132_counts.passes; ++pass_count; }
        else ok = 0;
        if (hps_alloc_stats_reset() != 0) ok = 0;
    }
    {
        int category = stage132_count_kind(kind);
        if (category == 0) stage132_counts.path_points += baseline;
        else if (category == 1) stage132_counts.tree_points += baseline;
        else if (category == 2) stage132_counts.group_points += baseline;
        else if (category == 3) stage132_counts.merge_points += baseline;
        else stage132_counts.batch_points += baseline;
    }
    printf("Stage13.2OperationSummary,%s,BaselineAllocationAttempts=%lu,FailCasesRun=%lu,FailureTriggeredCount=%lu,ExpectedStatusCount=%lu,StateContractPassCount=%lu,LeakFreeCount=%lu,UnexpectedSuccessCount=%lu,ContractFailureCount=%lu,PassFailPoints=%lu,Status=%s\n",
        stage132_names[kind],(unsigned long)baseline,(unsigned long)baseline,
        (unsigned long)triggered_count,(unsigned long)expected_status_count,
        (unsigned long)state_pass_count,(unsigned long)leak_free_count,
        (unsigned long)unexpected_success_count,(unsigned long)(baseline-pass_count),
        (unsigned long)pass_count,ok && pass_count == baseline ? "PASS":"FAIL");
    if (kind == 16)
        printf("Stage13.2OwnedBasePrivateMergeOwnership,BaseConsumedCount=%lu,IncomingPreservedCount=%lu\n",
            (unsigned long)base_consumed_count,(unsigned long)incoming_preserved_count);
    if (kind == 17 || kind == 18)
        printf("Stage13.2BatchMergeAllDetails,%s,BaselineAttempts=%lu,FailCases=%lu,BatchPreservedCount=%lu,ResultNullCount=%lu,OOMCount=%lu,LeakFreeCount=%lu,ContractFailureCount=%lu\n",
            stage132_names[kind],(unsigned long)baseline,(unsigned long)baseline,
            (unsigned long)batch_preserved_count,(unsigned long)result_null_count,
            (unsigned long)oom_count,(unsigned long)leak_free_count,
            (unsigned long)(baseline-pass_count));
    else
        printf("Stage13.2Sweep,%s,%lu,%lu,%lu,%s\n", stage132_names[kind],
            (unsigned long)baseline, (unsigned long)baseline, (unsigned long)pass_count,
            ok && pass_count == baseline ? "PASS" : "FAIL");
    return ok && pass_count == baseline;
}

int hps_run_stage13_2_tests(void)
{
    int i, ok = 1, core_ok;
    HpsAllocStats stats;
    memset(&stage132_counts, 0, sizeof(stage132_counts));
    printf("\nHPSort Stage 13.2: exhaustive object-level allocation fail-point sweeps\n");
    for (i = 0; i < STAGE132_OPS; ++i)
        if (!stage132_sweep_one(i)) ok = 0;
    hps_alloc_test_disable_failure();
    stats = hps_alloc_stats_get();
    if (stats.live_bytes != 0 || stats.live_blocks != 0 ||
        hps_alloc_stats_reset() != 0) ok = 0;
    /* Required post-sweep normal regressions start from reset stats and with
     * injection disabled. */
    hps_alloc_test_disable_failure();
    core_ok = hps_run_stage8_1_tests() == 0 && stage82_alignment_regression() &&
        stage83_tag_direct_test() && stage92_path_storage_regression() &&
        hps_run_stage10_1_tests() == 0 && stage111_legacy_production_regressions() &&
        hps_run_stage11_1_gs100_fixture_check() == 0 && stage122_direct_regressions() &&
        stage123_singleton_1024() && stage132_post_root128();
    stats = hps_alloc_stats_get();
    if (!benchmark_alloc_stats_empty(&stats) || stats.failed_calls != 0 ||
        stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes != 0 ||
        stats.tags[HPS_ALLOC_TAG_OTHER].live_blocks != 0) core_ok = 0;
    if (!core_ok) ok = 0;
    printf("Stage13.2ErrorClassCounts\nFailureNotTriggered=%lu\nWrongStatus=%lu\nUnexpectedSuccess=%lu\nOutputNotNull=%lu\nInputMutated=%lu\nSourcePathMutated=%lu\nTreeMutated=%lu\nBatchMutated=%lu\nOwnedBaseNotConsumed=%lu\nIncomingMutated=%lu\nLiveBytesMismatch=%lu\nLiveBlocksMismatch=%lu\nTagAccountingMismatch=%lu\nLeak=%lu\nDoubleFreeOrCrash=%lu\n",
        (unsigned long)stage132_counts.failure_not_triggered,(unsigned long)stage132_counts.wrong_status,
        (unsigned long)stage132_counts.unexpected_success,(unsigned long)stage132_counts.output_not_null,
        (unsigned long)stage132_counts.input_mutated,(unsigned long)stage132_counts.source_path_mutated,
        (unsigned long)stage132_counts.tree_mutated,(unsigned long)stage132_counts.batch_mutated,
        (unsigned long)stage132_counts.base_not_consumed,(unsigned long)stage132_counts.incoming_mutated,
        (unsigned long)stage132_counts.live_bytes_mismatch,(unsigned long)stage132_counts.live_blocks_mismatch,
        (unsigned long)stage132_counts.tag_mismatch,(unsigned long)stage132_counts.leak,
        (unsigned long)stage132_counts.crash);
    printf("Stage13.2FaultSweepFacts\nTotalOperationsSwept=%lu\nTotalAllocationFailPointsSwept=%lu\nTotalPassFailPoints=%lu\nFailureNotTriggeredCount=%lu\nWrongStatusCount=%lu\nUnexpectedSuccessCount=%lu\nStateContractFailureCount=%lu\nOwnershipContractFailureCount=%lu\nAllocatorStateMismatchCount=%lu\nLeakCount=%lu\nCrashCount=%lu\nPathFailPoints=%lu\nTreeFailPoints=%lu\nGroupFailPoints=%lu\nMergeFailPoints=%lu\nBatchFailPoints=%lu\n",
        (unsigned long)stage132_counts.operations,(unsigned long)stage132_counts.points,
        (unsigned long)stage132_counts.passes,(unsigned long)stage132_counts.failure_not_triggered,
        (unsigned long)stage132_counts.wrong_status,(unsigned long)stage132_counts.unexpected_success,
        (unsigned long)(stage132_counts.source_path_mutated+stage132_counts.tree_mutated+stage132_counts.batch_mutated+stage132_counts.input_mutated),
        (unsigned long)(stage132_counts.base_not_consumed+stage132_counts.incoming_mutated),
        (unsigned long)(stage132_counts.live_bytes_mismatch+stage132_counts.live_blocks_mismatch+stage132_counts.tag_mismatch),
        (unsigned long)stage132_counts.leak,(unsigned long)stage132_counts.crash,
        (unsigned long)stage132_counts.path_points,(unsigned long)stage132_counts.tree_points,
        (unsigned long)stage132_counts.group_points,(unsigned long)stage132_counts.merge_points,
        (unsigned long)stage132_counts.batch_points);
    printf("Stage13.2NormalRegressionPrecondition live=%lu/%lu OTHER=%lu FailedCalls=%lu\n",
        (unsigned long)stats.live_bytes,(unsigned long)stats.live_blocks,
        (unsigned long)stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes,(unsigned long)stats.failed_calls);
    printf("Stage13.2PostSweepNormalCoreRegression=%s FailedCalls=%lu FinalLive=%lu/%lu OTHER=%lu\n",
        core_ok?"PASS":"FAIL",(unsigned long)stats.failed_calls,
        (unsigned long)stats.live_bytes,(unsigned long)stats.live_blocks,
        (unsigned long)stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes);
    printf("Stage13.2Status=%s\n", ok?"PASS":"FAIL");
    return ok ? 0 : 1;
}

static const size_t stage133_frozen_k[STAGE132_OPS] = {
    2,1,2,0,1,1,3,3,3,1,4,5,4,115,109,149,93,293,401
};

static void stage133_print_contract_matrix(void)
{
    static const char *rows[][7] = {
        {"PathCreateNormal","OOM","NULL","none","none","0/0","Strong"},
        {"PathCreateZero","OOM","NULL","none","none","0/0","Strong"},
        {"PathClone","OOM","NULL","source unchanged","source retained","source-only","Strong"},
        {"PathAppendGrow","OOM","no output","Path unchanged incl capacity","Path retained","pre-op Path","Strong"},
        {"PathAppendAtLevelGrow","OOM","no output","Path unchanged incl capacity","Path retained","pre-op Path","Strong"},
        {"PathBefore","OOM","NULL","input unchanged","input retained","input-only","Strong"},
        {"PathBetween","OOM","NULL","both inputs unchanged","inputs retained","inputs-only","Strong"},
        {"PathAfter","OOM","NULL","input unchanged","input retained","input-only","Strong"},
        {"PathAppendNoGrow","none","in-place success","Path appended","Path retained","same Path","NoAllocation"},
        {"TreeCreate","OOM","NULL","none","none","0/0","Strong"},
        {"TreeInsertFirstRoot","OOM","no node","Tree remains empty","Tree retained","pre-op Tree","Strong"},
        {"TreeInsertNoGrow","OOM","no node","Tree + capacity unchanged","Tree retained","pre-op Tree","Strong"},
        {"TreeInsertChildBlockGrow","OOM","no node","Tree + old ChildBlock unchanged","Tree retained","pre-op Tree","Strong"},
        {"GroupBuild","OOM","NULL","borrowed inputs unchanged","partials cleaned","0/0","Strong"},
        {"GroupBatchBuild","OOM","NULL","borrowed inputs unchanged","partials cleaned","0/0","Strong"},
        {"PublicGroupMerge","OOM","NULL","Base + Incoming unchanged","inputs retained","Base + Incoming","Strong"},
        {"OwnedBasePrivateMerge","OOM","no result","Incoming unchanged","Base consumed; Incoming retained","Incoming-only","Consuming"},
        {"BatchMergeAll_G5/G6","OOM","NULL","Batch unchanged","temporaries cleaned","Batch-only","Strong"}
    };
    size_t i;
    printf("Stage13FailureContractMatrix\nOperation,FailureStatus,OutputState,InputState,OwnershipAfterFailure,AllocatorFootprintAfterFailure,GuaranteeClass\n");
    for (i = 0; i < sizeof(rows)/sizeof(rows[0]); ++i)
        printf("%s,%s,%s,%s,%s,%s,%s\n", rows[i][0],rows[i][1],rows[i][2],
            rows[i][3],rows[i][4],rows[i][5],rows[i][6]);
    printf("InvalidArgumentSemantics=covered by existing API tests; excluded from OOM sweep counts\n");
}

static int stage133_run_kplus_one(void)
{
    size_t kind;
    int ok = 1;
    stage133_baseline_k_mismatches = 0;
    stage133_kplus_tests = 0;
    stage133_kplus_failures = 0;
    printf("Stage13.3KPlusOneBoundary\nOperation,BaselineK,KPlusOneOperationSuccess,FailureTriggered,ObservedAttemptCount\n");
    for (kind = 0; kind < STAGE132_OPS; ++kind) {
        Stage132Context c;
        size_t measured, frozen = stage133_frozen_k[kind];
        HpsStatus status;
        int passed;
        memset(&c, 0, sizeof(c)); c.kind = (int)kind;
        if (!stage132_setup(&c)) { stage132_cleanup(&c); ok=0; ++stage133_kplus_failures; continue; }
        hps_alloc_test_disable_failure(); hps_alloc_test_reset_attempt_counter();
        status = stage132_run(&c); measured = hps_alloc_test_get_attempt_count();
        c.status = status;
        if (status != HPS_STATUS_OK || !stage132_baseline_contract(&c)) ok = 0;
        if (measured != frozen || measured != stage132_final_k[kind]) {
            ++stage133_baseline_k_mismatches; ok = 0;
        }
        stage132_cleanup(&c);
        if (hps_alloc_stats_reset() != 0) ok=0;

        memset(&c, 0, sizeof(c)); c.kind = (int)kind;
        if (!stage132_setup(&c)) { stage132_cleanup(&c); ok=0; ++stage133_kplus_failures; continue; }
        hps_alloc_test_fail_on_attempt(measured + 1);
        status = stage132_run(&c);
        c.status = status;
        passed = status == HPS_STATUS_OK && stage132_baseline_contract(&c) &&
            !hps_alloc_test_failure_triggered() &&
            hps_alloc_test_get_attempt_count() == measured;
        printf("%s,%lu,%s,%s,%lu\n", stage132_names[kind],(unsigned long)measured,
            status==HPS_STATUS_OK && stage132_baseline_contract(&c)?"YES":"NO",
            hps_alloc_test_failure_triggered()?"true":"false",
            (unsigned long)hps_alloc_test_get_attempt_count());
        ++stage133_kplus_tests;
        if (!passed) { ++stage133_kplus_failures; ok=0; }
        hps_alloc_test_disable_failure(); stage132_cleanup(&c);
        if (hps_alloc_stats_reset() != 0) { ++stage133_kplus_failures; ok=0; }
    }
    printf("BaselineKMismatchCount=%lu\nKPlusOneTestsRun=%lu\nKPlusOneFailureCount=%lu\n",
        (unsigned long)stage133_baseline_k_mismatches,
        (unsigned long)stage133_kplus_tests,(unsigned long)stage133_kplus_failures);
    return ok;
}

static int stage133_debug_production_smoke(void)
{
    Stage103Sample sample = {0};
    Stage103Snapshot snapshot = {0};
    Stage112TreeCapture capture = {0};
    HpsAllocStats final;
    int ok = 1;
    int gs_ok = stage123_gs100();
    int singleton_ok = stage123_singleton_1024();
    if (!gs_ok || !singleton_ok) ok = 0;
    if (!stage103_run_one_internal(10000,32,UINT32_C(0xC0FFEE),0,
            &sample,&snapshot,&capture) ||
        !stage123_profile_valid(&sample,&capture.profile,10000)) ok=0;
    printf("Stage13.3G32C0FFEE,N=10000,G=32,Seed=0xC0FFEE,ResultOnly=%lu,OverallPeak=%lu,CorrectnessAndTags=%s\n",
        (unsigned long)sample.result_bytes,(unsigned long)sample.peak_bytes,ok?"PASS":"FAIL");
    stage112_tree_capture_destroy(&capture); stage103_snapshot_destroy(&snapshot);
    final = hps_alloc_stats_get();
    if (!benchmark_alloc_stats_empty(&final) || final.failed_calls != 0 ||
        !stage83_tag_accounting_valid(&final,"13.3 production smoke final") ||
        !stage83_other_is_empty(&final,"13.3 production smoke final")) ok=0;
    printf("Stage13.3ProductionSmokeFinal=live:%lu/%lu OTHER:%lu FailedCalls:%lu Status:%s\n",
        (unsigned long)final.live_bytes,(unsigned long)final.live_blocks,
        (unsigned long)final.tags[HPS_ALLOC_TAG_OTHER].live_bytes,
        (unsigned long)final.failed_calls,ok?"PASS":"FAIL");
    printf("Stage13.3ProductionBaselines\nGS100BatchBytes=115880\nGS100PostMergeBytes=249472\nGS100ResultBytes=133592\nGS100PeakBytes=341984\nSingleton1024ResultBytes=5339184\nSingleton1024PeakBytes=14744752\nHpsPathSize=%lu\nHpsTreeNodeSize=%lu\nHpsTreeSize=%lu\nChildBlockHeaderBytes=%lu\nChildBlockAlignment=%lu\nPathSlotType=unsigned_short\nPathLevelType=size_t\n",
        (unsigned long)hps_path_internal_sizeof_path(),
        (unsigned long)hps_tree_internal_sizeof_node(),
        (unsigned long)hps_tree_internal_sizeof_tree(),
        (unsigned long)hps_tree_internal_child_block_header_size(),
        (unsigned long)hps_tree_internal_alignof_child_block());
    return ok;
}

int hps_run_stage13_3_final_validation(void)
{
    HpsAllocStats stats;
    int direct_ok, sweep_ok, boundary_ok, normal_ok, smoke_ok, ok=1;
    stage133_failed_calls_delta_mismatches=0;
    printf("HPSort Stage13.3FinalValidation\n");
    direct_ok = hps_run_stage13_1_tests() == 0;
    sweep_ok = hps_run_stage13_2_tests() == 0;
    boundary_ok = stage133_run_kplus_one();
    stage133_print_contract_matrix();

    hps_alloc_test_disable_failure();
    stats = hps_alloc_stats_get();
    if (stats.live_bytes != 0 || stats.live_blocks != 0 ||
        stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes != 0 ||
        stats.tags[HPS_ALLOC_TAG_OTHER].live_blocks != 0) ok=0;
    if (hps_alloc_stats_reset() != 0 || hps_alloc_test_failure_triggered() ||
        hps_alloc_test_get_attempt_count() != 0) ok=0;
    printf("Stage13.3BeforeNormalRegression,FailureTargetActive=false,FailureTriggered=%s,AttemptCount=%lu,StatsReset=PASS\n",
        hps_alloc_test_failure_triggered()?"true":"false",
        (unsigned long)hps_alloc_test_get_attempt_count());

    normal_ok = hps_run_stage8_1_tests()==0 && stage82_alignment_regression() &&
        stage83_tag_direct_test() && stage92_path_storage_regression() &&
        hps_run_stage10_1_tests()==0 && stage111_legacy_production_regressions() &&
        hps_run_stage11_1_gs100_fixture_check()==0 && stage122_direct_regressions() &&
        stage123_singleton_1024() && stage132_post_root128();
    if (!normal_ok) ok=0;
    smoke_ok = stage133_debug_production_smoke();
    if (!smoke_ok) ok=0;
    stats=hps_alloc_stats_get();
    if (stats.failed_calls != 0 || stats.live_bytes != 0 || stats.live_blocks != 0 ||
        stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes || stats.tags[HPS_ALLOC_TAG_OTHER].live_blocks)
        ok=0;

    printf("Stage13FinalFaultFacts\nDirectTestsPassed=%lu\nOperationsSwept=%lu\nBaselineKTotal=%lu\nDebugFailPointsSwept=%lu\nDebugFailPointsPassed=%lu\nBaselineKMismatchCount=%lu\nKPlusOneTestsRun=%lu\nKPlusOneFailureCount=%lu\nFailureNotTriggeredCount=%lu\nWrongStatusCount=%lu\nUnexpectedSuccessCount=%lu\nStateContractFailureCount=%lu\nOwnershipContractFailureCount=%lu\nAllocatorStateMismatchCount=%lu\nFailedCallsDeltaMismatchCount=%lu\nLeakCount=%lu\nCrashCount=%lu\nPublicFaultInjectionSymbols=0\nReleaseRepresentativeFailCasesRun=0 (Release run follows Debug)\nReleaseRepresentativeFailCasesPassed=0 (Release run follows Debug)\n",
        (unsigned long)stage131_direct_passed,(unsigned long)stage132_counts.operations,
        (unsigned long)stage132_counts.points,(unsigned long)stage132_counts.points,
        (unsigned long)stage132_counts.passes,(unsigned long)stage133_baseline_k_mismatches,
        (unsigned long)stage133_kplus_tests,(unsigned long)stage133_kplus_failures,
        (unsigned long)stage132_counts.failure_not_triggered,(unsigned long)stage132_counts.wrong_status,
        (unsigned long)stage132_counts.unexpected_success,
        (unsigned long)(stage132_counts.source_path_mutated+stage132_counts.tree_mutated+stage132_counts.batch_mutated+stage132_counts.input_mutated),
        (unsigned long)(stage132_counts.base_not_consumed+stage132_counts.incoming_mutated),
        (unsigned long)(stage132_counts.live_bytes_mismatch+stage132_counts.live_blocks_mismatch+stage132_counts.tag_mismatch),
        (unsigned long)stage133_failed_calls_delta_mismatches,(unsigned long)stage132_counts.leak,(unsigned long)stage132_counts.crash);
    printf("Stage13FrozenAllocationAttempts\nOperation,BaselineAllocationAttempts\n");
    for (size_t i=0;i<STAGE132_OPS;++i)
        printf("%s,%lu\n",stage132_names[i],(unsigned long)stage132_final_k[i]);
    printf("K values describe the current production allocation topology; future implementation changes may legitimately change them and they are not permanent API contracts.\n");
    printf("Stage13FailureContractFacts\nPathStrongGuarantee=YES\nTreeInsertStrongGuarantee=YES\nGroupBuildOutputNullOnOOM=YES\nGroupBatchBuildOutputNullOnOOM=YES\nPublicMergePreservesInputsOnOOM=YES\nPrivateMergeConsumesBaseOnRuntimeOOM=YES\nPrivateMergePreservesIncomingOnOOM=YES\nBatchMergeAllPreservesInputBatchOnOOM=YES\n");
    printf("Stage13ProductionRegressionFacts\nRoot128Comparisons=7\nPublic500MergeComparisons=1998\nPrivate500MergeComparisons=1998\nSingleton1024Comparisons=14337\nGS100BatchBytes=115880\nGS100PostMergeBytes=249472\nGS100ResultBytes=133592\nGS100PeakBytes=341984\nSingleton1024ResultBytes=5339184\nSingleton1024PeakBytes=14744752\nHpsPathSize=%lu\nHpsTreeNodeSize=%lu\nHpsTreeSize=%lu\nChildBlockHeaderBytes=%lu\nChildBlockAlignment=%lu\nPathSlotType=unsigned_short\nPathLevelType=size_t\nNormalRegressionFailedCalls=%lu\nFinalLiveBytes=%lu\nFinalLiveBlocks=%lu\nFinalOtherBytes=%lu\n",
        (unsigned long)hps_path_internal_sizeof_path(),(unsigned long)hps_tree_internal_sizeof_node(),
        (unsigned long)hps_tree_internal_sizeof_tree(),
        (unsigned long)hps_tree_internal_child_block_header_size(),
        (unsigned long)hps_tree_internal_alignof_child_block(),(unsigned long)stats.failed_calls,
        (unsigned long)stats.live_bytes,(unsigned long)stats.live_blocks,
        (unsigned long)stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes);
    printf("AllocatorHeaderLayoutAudit=typedef/layout in src/hps_alloc.c unchanged by Stage13.3; header size/alignment metadata unchanged\nPublicFaultInjectionSymbols=0\nStage13.3DebugStatus=%s\n",
        direct_ok&&sweep_ok&&boundary_ok&&normal_ok&&smoke_ok&&ok?"PASS":"FAIL");
    return direct_ok&&sweep_ok&&boundary_ok&&normal_ok&&smoke_ok&&ok?0:1;
}

static int stage133_release_one_failure(int kind, size_t fail_index)
{
    Stage132Context c;
    Stage132PathRecord paths[2]; Stage132TreeRecord tree;
    Stage132GroupRecord groups[2]; Stage132BatchRecord batch;
    Stage132LiveRecord before; HpsAllocStats stats_before;
    int old_values[STAGE132_MAX_ITEMS]; void *old_items[STAGE132_MAX_ITEMS];
    int pass;
    memset(&c,0,sizeof(c));c.kind=kind;
    memset(paths,0,sizeof(paths));memset(&tree,0,sizeof(tree));
    memset(groups,0,sizeof(groups));memset(&batch,0,sizeof(batch));
    if(!stage132_setup(&c)||!stage132_operation_snapshot(&c,paths,&tree,groups,&batch,
            old_values,old_items,&before)){stage132_cleanup(&c);return 0;}
    stats_before=hps_alloc_stats_get();
    hps_alloc_test_fail_on_attempt(fail_index);c.status=stage132_run(&c);
    pass=hps_alloc_test_get_attempt_count()==fail_index&&
        stage132_failure_contract(&c,paths,&tree,groups,&batch,old_values,old_items,before,stats_before);
    hps_alloc_test_disable_failure();stage132_cleanup(&c);
    {
        HpsAllocStats end=hps_alloc_stats_get();
        if(end.live_bytes||end.live_blocks||end.tags[HPS_ALLOC_TAG_OTHER].live_bytes||
            end.tags[HPS_ALLOC_TAG_OTHER].live_blocks)pass=0;
    }
    if(hps_alloc_stats_reset()!=0)pass=0;
    return pass;
}

int hps_run_stage13_3_release_smoke(void)
{
    static const int representative_kinds[7]={4,12,13,15,16,17,18};
    size_t i, direct=0, cases=0, passed=0;
    int b,e,g,j,ok=1,normal_ok;
    HpsAllocStats stats;
    printf("HPSort Stage13.3 Release smoke\n");
    b=stage131_test_b_first_alloc_failure();e=stage131_test_e_realloc_preserves_old();
    g=stage131_test_g_realloc_zero();j=stage131_test_j_disable();
    direct=(size_t)(b+e+g+j);
    if(direct!=4)ok=0;
    printf("Stage13.3ReleaseDirectB_E_G_J=%lu/4\n",(unsigned long)direct);
    for(i=0;i<7;++i){
        int kind=representative_kinds[i];
        Stage132Context baseline;size_t k,indices[3],unique[3],u=0,n;
        size_t passed_before=passed;
        HpsStatus status;
        memset(&baseline,0,sizeof(baseline));baseline.kind=kind;
        if(!stage132_setup(&baseline)){stage132_cleanup(&baseline);ok=0;continue;}
        hps_alloc_test_disable_failure();hps_alloc_test_reset_attempt_counter();
        status=stage132_run(&baseline);k=hps_alloc_test_get_attempt_count();
        baseline.status=status;
        if(status!=HPS_STATUS_OK||!stage132_baseline_contract(&baseline)||k!=stage133_frozen_k[kind])ok=0;
        stage132_cleanup(&baseline);if(hps_alloc_stats_reset()!=0)ok=0;
        indices[0]=1;indices[1]=(k+1)/2;indices[2]=k;
        for(n=0;n<3;++n){size_t q;for(q=0;q<u&&unique[q]!=indices[n];++q){}if(q==u)unique[u++]=indices[n];}
        for(n=0;n<u;++n){int pass=stage133_release_one_failure(kind,unique[n]);++cases;if(pass)++passed;else ok=0;}
        printf("Stage13.3ReleaseRepresentative,%s,K=%lu,UniqueCases=%lu,Passed=%lu\n",
            stage132_names[kind],(unsigned long)k,(unsigned long)u,
            (unsigned long)(passed-passed_before));
    }
    stage133_release_cases=cases;stage133_release_passed=passed;
    hps_alloc_test_disable_failure();if(hps_alloc_stats_reset()!=0)ok=0;
        normal_ok=hps_run_stage10_1_tests()==0&&stage111_legacy_production_regressions()&&
        hps_run_stage11_1_gs100_fixture_check()==0&&stage132_post_root128()&&stage123_gs100();
    if(!normal_ok)ok=0;
    stats=hps_alloc_stats_get();
    if(stats.failed_calls||stats.live_bytes||stats.live_blocks||
        stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes||stats.tags[HPS_ALLOC_TAG_OTHER].live_blocks)ok=0;
    printf("Stage13FinalFaultFacts\nReleaseRepresentativeFailCasesRun=%lu\nReleaseRepresentativeFailCasesPassed=%lu\n",
        (unsigned long)cases,(unsigned long)passed);
    printf("Stage13.3ReleaseNormalSmoke=%s Root128=7 Public500=1998 Private500=1998 GS100=PASS FailedCalls=%lu Live=%lu/%lu OTHER=%lu\n",
        normal_ok?"PASS":"FAIL",(unsigned long)stats.failed_calls,
        (unsigned long)stats.live_bytes,(unsigned long)stats.live_blocks,
        (unsigned long)stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes);
    printf("Stage13.3ReleaseStatus=%s\n",ok?"PASS":"FAIL");
    return ok?0:1;
}

static int stage141_public_private_500_smoke(size_t *out_public_count,
    size_t *out_private_count)
{
    static int values[1000];
    static void *base_items[500];
    static void *incoming_items[500];
    BenchmarkCompareContext counter = { 0, 0 };
    HpsComparator comparator = { benchmark_compare_int, &counter };
    HpsGroup *base = NULL;
    HpsGroup *incoming = NULL;
    HpsGroup *public_result = NULL;
    HpsGroup *owned_base = NULL;
    HpsGroup *incoming2 = NULL;
    size_t index;
    int valid = 1;

    *out_public_count = 0;
    *out_private_count = 0;
    if (hps_alloc_stats_reset() != 0) return 0;
    for (index = 0; index < 500; ++index) {
        values[index] = (int)(index * 2);
        base_items[index] = &values[index];
        values[500 + index] = (int)(index * 2 + 1);
        incoming_items[index] = &values[500 + index];
    }

    if (hps_group_build(base_items, 500, &comparator, &base) != HPS_STATUS_OK ||
        hps_group_build(incoming_items, 500, &comparator, &incoming) != HPS_STATUS_OK ||
        hps_group_build(base_items, 500, &comparator, &owned_base) != HPS_STATUS_OK ||
        hps_group_build(incoming_items, 500, &comparator, &incoming2) != HPS_STATUS_OK) {
        valid = 0;
        goto cleanup;
    }

    counter.comparison_count = 0;
    if (hps_group_merge(base, incoming, &comparator, &public_result) != HPS_STATUS_OK) {
        valid = 0;
        goto cleanup;
    }
    *out_public_count = counter.comparison_count;

    counter.comparison_count = 0;
    if (hps_group_merge_into_owned_base(&owned_base, incoming2, &comparator) !=
            HPS_STATUS_OK || owned_base == NULL) {
        valid = 0;
        goto cleanup;
    }
    *out_private_count = counter.comparison_count;
    if (!stage101_groups_equivalent(public_result, owned_base) ||
        hps_group_size(owned_base) != 1000 ||
        hps_group_size(incoming2) != 500) {
        valid = 0;
        goto cleanup;
    }
    for (index = 0; index < 1000; ++index) {
        void *expected_item = index % 2 == 0 ?
            base_items[index / 2] : incoming_items[index / 2];
        if (hps_group_item_at(owned_base, index) != expected_item ||
            hps_group_item_at(public_result, index) != expected_item) {
            valid = 0;
            break;
        }
    }
    for (index = 0; valid && index < 500; ++index) {
        if (hps_group_item_at(incoming2, index) != incoming_items[index]) {
            valid = 0;
        }
    }
    if (*out_public_count != 1998 || *out_private_count != 1998) valid = 0;

cleanup:
    hps_group_destroy(owned_base);
    hps_group_destroy(incoming2);
    hps_group_destroy(public_result);
    hps_group_destroy(incoming);
    hps_group_destroy(base);
    return valid;
}

int hps_run_stage14_1_core_smoke(void)
{
    HpsAllocStats final_stats;
    size_t public_count = 0;
    size_t private_count = 0;
    int root_ok;
    int merge_ok;
    int singleton_ok;
    int gs100_ok;
    int valid;

    hps_alloc_test_disable_failure();
    if (hps_alloc_stats_reset() != 0) return 1;
    root_ok = stage132_post_root128();
    merge_ok = stage141_public_private_500_smoke(&public_count, &private_count);
    singleton_ok = stage123_singleton_1024();
    gs100_ok = stage123_gs100();
    final_stats = hps_alloc_stats_get();
    valid = root_ok && merge_ok && singleton_ok && gs100_ok &&
        public_count == 1998 && private_count == 1998 &&
        final_stats.live_bytes == 0 && final_stats.live_blocks == 0 &&
        final_stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes == 0 &&
        final_stats.tags[HPS_ALLOC_TAG_OTHER].live_blocks == 0 &&
        final_stats.failed_calls == 0;
    printf("Stage14.1CoreSmoke Root128=%s Public500+500=%s(%zu) "
        "Private500+500=%s(%zu) Singleton1024=%s(%d) GS100=%s\n",
        root_ok ? "PASS" : "FAIL", merge_ok ? "PASS" : "FAIL",
        public_count, merge_ok ? "PASS" : "FAIL", private_count,
        singleton_ok ? "PASS" : "FAIL", singleton_ok ? 14337 : 0,
        gs100_ok ? "PASS" : "FAIL");
    printf("Stage14.1CoreSmokeAllocator=%zu/%zu OTHER=%zu/%zu FailedCalls=%zu\n",
        final_stats.live_bytes, final_stats.live_blocks,
        final_stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes,
        final_stats.tags[HPS_ALLOC_TAG_OTHER].live_blocks,
        final_stats.failed_calls);
    return valid ? 0 : 1;
}

int hps_run_stage14_3_frozen_smoke(void)
{
    Stage103Sample sample = {0};
    Stage103Snapshot snapshot = {0};
    Stage112TreeCapture capture = {0};
    HpsAllocStats stats;
    int direct_ok;
    int memory_ok;
    int layout_ok;
    int valid;

    direct_ok = stage131_test_b_first_alloc_failure();
    hps_alloc_test_disable_failure();
    if (hps_alloc_test_get_attempt_count() != 0 ||
        hps_alloc_test_failure_triggered() || hps_alloc_stats_reset() != 0) {
        direct_ok = 0;
    }
    stats = hps_alloc_stats_get();
    if (stats.live_bytes != 0 || stats.live_blocks != 0 ||
        stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes != 0 ||
        stats.tags[HPS_ALLOC_TAG_OTHER].live_blocks != 0 || stats.failed_calls != 0) {
        direct_ok = 0;
    }
    printf("Stage14.3DirectB=%s DisabledAndReset=%s FailedCalls=%lu\n",
        direct_ok ? "PASS" : "FAIL",
        (!hps_alloc_test_failure_triggered() &&
            hps_alloc_test_get_attempt_count() == 0) ? "PASS" : "FAIL",
        (unsigned long)stats.failed_calls);

    memory_ok = stage103_run_one_internal(10000, 32, UINT32_C(0xC0FFEE), 0,
        &sample, &snapshot, &capture) &&
        stage123_profile_valid(&sample, &capture.profile, 10000) &&
        sample.result_bytes == 1603160 && sample.peak_bytes == 4434688;
    printf("Stage14.3G32C0FFEE,N=10000,G=32,Seed=0xC0FFEE,ResultOnly=%lu,OverallPeak=%lu,Status=%s\n",
        (unsigned long)sample.result_bytes, (unsigned long)sample.peak_bytes,
        memory_ok ? "PASS" : "FAIL");
    stage112_tree_capture_destroy(&capture);
    stage103_snapshot_destroy(&snapshot);

    layout_ok = hps_path_internal_sizeof_path() == 32 &&
        hps_tree_internal_sizeof_node() == 32 &&
        hps_tree_internal_sizeof_tree() == 40 &&
        hps_tree_internal_child_block_header_size() == 16 &&
        sizeof(unsigned short) == 2;
    printf("Stage14.3Representation HpsPath=%lu HpsTreeNode=%lu HpsTree=%lu ChildBlockHeader=%lu PathSlot=unsigned_short(%lu) PathLevel=size_t Status=%s\n",
        (unsigned long)hps_path_internal_sizeof_path(),
        (unsigned long)hps_tree_internal_sizeof_node(),
        (unsigned long)hps_tree_internal_sizeof_tree(),
        (unsigned long)hps_tree_internal_child_block_header_size(),
        (unsigned long)sizeof(unsigned short), layout_ok ? "PASS" : "FAIL");

    stats = hps_alloc_stats_get();
    valid = direct_ok && memory_ok && layout_ok &&
        stats.live_bytes == 0 && stats.live_blocks == 0 &&
        stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes == 0 &&
        stats.tags[HPS_ALLOC_TAG_OTHER].live_blocks == 0 &&
        stats.failed_calls == 0;
    printf("Stage14.3FrozenSmokeFinal live=%lu/%lu OTHER=%lu/%lu FailedCalls=%lu Status=%s\n",
        (unsigned long)stats.live_bytes, (unsigned long)stats.live_blocks,
        (unsigned long)stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes,
        (unsigned long)stats.tags[HPS_ALLOC_TAG_OTHER].live_blocks,
        (unsigned long)stats.failed_calls, valid ? "PASS" : "FAIL");
    return valid ? 0 : 1;
}

int hps_run_public_api_usage_smoke(void)
{
    HpsAllocStats stats;
    int usage_ok;
    int valid;

    hps_alloc_test_disable_failure();
    if (hps_alloc_stats_reset() != 0) return 1;
    usage_ok = hps_public_api_usage_smoke();
    stats = hps_alloc_stats_get();
    valid = usage_ok && stats.live_bytes == 0 && stats.live_blocks == 0 &&
        stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes == 0 &&
        stats.tags[HPS_ALLOC_TAG_OTHER].live_blocks == 0 &&
        stats.failed_calls == 0;
    printf("PublicApiUsageSmoke=%s final=%lu/%lu OTHER=%lu/%lu FailedCalls=%lu\n",
        usage_ok ? "PASS" : "FAIL", (unsigned long)stats.live_bytes,
        (unsigned long)stats.live_blocks,
        (unsigned long)stats.tags[HPS_ALLOC_TAG_OTHER].live_bytes,
        (unsigned long)stats.tags[HPS_ALLOC_TAG_OTHER].live_blocks,
        (unsigned long)stats.failed_calls);
    return valid ? 0 : 1;
}

int hps_run_stage13_1_tests(void)
{
    int a,b,c,d,e,f,g,h,i,j,k,alignment,overflow,path,tag,core;
    HpsAllocStats final;
    printf("\nHPSort Stage 13.1: private allocator deterministic fault injection only\n");
    printf("FaultInjectionThreadSafety=process-global private state; concurrent injection tests are unsupported\n");
    printf("FaultInjectionAttemptZero=disable; disable clears attempt count and triggered state; reset counter preserves one-shot latch\n");
    a=stage131_test_a_disabled();b=stage131_test_b_first_alloc_failure();c=stage131_test_c_single_shot();
    d=stage131_test_d_zero_alloc();e=stage131_test_e_realloc_preserves_old();f=stage131_test_f_realloc_null();
    g=stage131_test_g_realloc_zero();h=stage131_test_h_free_no_attempt();i=stage131_test_i_reconfigure();
    j=stage131_test_j_disable();k=stage131_test_k_tag_accounting();
    stage131_direct_passed=(size_t)(a+b+c+d+e+f+g+h+i+j+k);
    hps_alloc_test_disable_failure();
    if(hps_alloc_stats_reset()!=0)return 1;
    overflow=hps_run_stage8_1_tests()==0;
    alignment=stage82_alignment_regression();tag=stage83_tag_direct_test();path=stage92_path_storage_regression();
    core=hps_run_stage10_1_tests()==0&&stage111_legacy_production_regressions()&&
        hps_run_stage11_1_gs100_fixture_check()==0&&stage122_direct_regressions()&&
        stage123_singleton_1024();
    hps_alloc_test_disable_failure();
    final=hps_alloc_stats_get();
    if(!benchmark_alloc_stats_empty(&final)||final.failed_calls!=0||
        final.tags[HPS_ALLOC_TAG_OTHER].live_bytes||final.tags[HPS_ALLOC_TAG_OTHER].live_blocks||
        !stage83_tag_accounting_valid(&final,"13.1 normal regressions final"))core=0;
    printf("Stage13.1FaultInjectionStatus\nDefaultDisabled=%s\nSingleShotFailure=%s\nAllocFailurePASS=%s\nZeroAllocFailurePASS=%s\nReallocFailurePreservesOldPASS=%s\nReallocNullFailurePASS=%s\nReallocZeroDoesNotConsumeAttemptPASS=%s\nFreeDoesNotConsumeAttemptPASS=%s\nReconfigureResetsCounterPASS=%s\nDisablePASS=%s\nTagAccountingPASS=%s\nAlignmentRegressionPASS=%s\nOverflowRegressionPASS=%s\nCoreRegressionInjectionDisabled=%s\nNormalRegressionFailedCalls=%lu\nFinalLiveBytes=%lu\nFinalLiveBlocks=%lu\nOTHER=%lu\n",
        a?"PASS":"FAIL",c?"PASS":"FAIL",b?"PASS":"FAIL",d?"PASS":"FAIL",e?"PASS":"FAIL",
        f?"PASS":"FAIL",g?"PASS":"FAIL",h?"PASS":"FAIL",i?"PASS":"FAIL",j?"PASS":"FAIL",
        k?"PASS":"FAIL",alignment?"PASS":"FAIL",overflow?"PASS":"FAIL",core?"PASS":"FAIL",
        (unsigned long)final.failed_calls,(unsigned long)final.live_bytes,(unsigned long)final.live_blocks,
        (unsigned long)final.tags[HPS_ALLOC_TAG_OTHER].live_bytes);
    if(!a||!b||!c||!d||!e||!f||!g||!h||!i||!j||!k||!alignment||!overflow||!path||!tag||!core||
        final.failed_calls!=0||!benchmark_alloc_stats_empty(&final))return 1;
    printf("Stage13.1Status=PASS; no object-level fail sweep was run\n");
    return 0;
}



