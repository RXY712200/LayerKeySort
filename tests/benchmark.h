#ifndef HPS_TEST_BENCHMARK_H
#define HPS_TEST_BENCHMARK_H

int hps_run_group_size_benchmark(void);
int hps_run_group_size_robustness_benchmark(void);
int hps_run_group_size_scale_benchmark(void);
int hps_run_group_count_cost_benchmark(void);
int hps_run_stage7_6_benchmark(void);
int hps_run_stage8_1_tests(void);
int hps_run_stage8_2_benchmark(void);
int hps_run_stage8_3_benchmark(void);
int hps_run_stage9_1_analysis(void);
int hps_run_stage9_2_optimization_tests(void);
int hps_run_stage9_3_final_validation(void);
int hps_run_stage10_1_tests(void);
int hps_run_stage10_2_tests(void);
int hps_run_stage10_3_validation(void);
int hps_run_stage11_1_baseline(void);
int hps_run_stage11_1_gs100_fixture_check(void);
int hps_run_stage11_2_tree_profile(void);
int hps_run_stage12_2_validation(void);
int hps_run_stage12_3_validation(void);
int hps_run_stage12_3_release_smoke(void);
int hps_run_stage13_1_tests(void);
int hps_run_stage13_2_tests(void);
int hps_run_stage13_3_final_validation(void);
int hps_run_stage13_3_release_smoke(void);
int hps_run_stage14_1_core_smoke(void);
int hps_run_stage14_3_frozen_smoke(void);
int hps_run_public_api_usage_smoke(void);

#endif /* HPS_TEST_BENCHMARK_H */



