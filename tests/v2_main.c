#include <stdio.h>
#include "property.h"

int lks_run_v2_preview_tests(void);
int lks_public_api_usage_smoke(void);

int main(void)
{
    if (lks_run_v2_preview_tests() != 0) return 1;
    if (!lks_public_api_usage_smoke()) return 1;
    if (lks_run_stage14_1_property_tests() != 0) return 1;
    if (lks_run_stage14_2_stress_tests() != 0) return 1;
    if (lks_run_stage14_2_oom_tests() != 0) return 1;
    puts("LayerKeySort v2 preview tests PASS");
    return 0;
}
