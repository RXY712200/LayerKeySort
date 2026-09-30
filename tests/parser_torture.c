#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "layerkeysort.h"

static uint32_t random32(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *state = x;
    return x;
}

static int parse_check(const char *input, int key)
{
    LksPath *path = NULL, *again = NULL;
    char canonical[256];
    LksStatus status = key ? lks_path_order_key_parse(input, &path) :
        lks_path_parse(input, &path);
    int cmp = 1, ok = 1;
    if (status == LKS_STATUS_OK) {
        size_t length;
        ok = path != NULL;
        length = key ? lks_path_order_key_length(path) : lks_path_text_length(path);
        if (!length || length >= sizeof(canonical)) ok = 0;
        if (ok) ok = (key ? lks_path_order_key_format(path, canonical,
            sizeof(canonical)) : lks_path_format(path, canonical,
            sizeof(canonical))) == LKS_STATUS_OK;
        if (ok) ok = strcmp(input, canonical) == 0;
        if (ok) ok = (key ? lks_path_order_key_parse(canonical, &again) :
            lks_path_parse(canonical, &again)) == LKS_STATUS_OK && again;
        if (ok) ok = lks_path_compare(path, again, &cmp) == LKS_STATUS_OK && cmp == 0;
    } else if (path != NULL) ok = 0;
    lks_path_destroy(again);
    lks_path_destroy(path);
    return ok;
}

static int make_valid(uint32_t *seed, char display[256], char key[256])
{
    unsigned int slot = random32(seed) % 65536u;
    LksDirection direction = (random32(seed) & 1u) ?
        LKS_DIRECTION_NEGATIVE : LKS_DIRECTION_POSITIVE;
    size_t level = random32(seed) % 65537u;
    LksPath *path = lks_path_create_at_level(direction, slot, level);
    int ok = path != NULL;
    unsigned int i, depth = random32(seed) % 4u;
    for (i = 0; ok && i < depth; ++i) {
        level += 1u + random32(seed) % 257u;
        ok = lks_path_append_at_level(path, random32(seed) % 65536u,
            level) == LKS_STATUS_OK;
    }
    if (ok) ok = lks_path_format(path, display, 256) == LKS_STATUS_OK &&
        lks_path_order_key_format(path, key, 256) == LKS_STATUS_OK;
    lks_path_destroy(path);
    return ok;
}

static void mutate(char text[256], uint32_t *seed)
{
    size_t length = strlen(text), at;
    unsigned int mode = random32(seed) % 8u;
    if (length == 0) return;
    at = random32(seed) % length;
    switch (mode) {
    case 0: text[at] = '\0'; break;                    /* truncate */
    case 1: text[at] = "!/:0gFz "[random32(seed) % 8u]; break;
    case 2: if (length + 1 < 256) { memmove(text + at + 1,
        text + at, length - at + 1); text[at] = '/'; } break;
    case 3: memmove(text + at, text + at + 1, length - at); break;
    case 4: if (length + 1 < 256) { text[length] = '!'; text[length + 1] = 0; } break;
    case 5: text[0] = '3'; break;
    case 6: if (length > 3) text[3] = '2'; break;
    default: text[at] = (char)('0' + random32(seed) % 10u); break;
    }
}

static int parse_number(const char *text, unsigned long long *out)
{
    char *end = NULL;
    errno = 0;
    *out = strtoull(text, &end, 0);
    return errno != ERANGE && text[0] != '-' && end != text && *end == 0;
}

int main(int argc, char **argv)
{
    size_t iterations = 2000, i;
    uint32_t seed = UINT32_C(0xBD24A511);
    int seen_iterations = 0, seen_seed = 0, arg;
    for (arg = 1; arg + 1 < argc; arg += 2) {
        unsigned long long value;
        if (!parse_number(argv[arg + 1], &value)) break;
        if (!strcmp(argv[arg], "--iterations") && !seen_iterations &&
            value > 0 && value <= SIZE_MAX) {
            iterations = (size_t)value; seen_iterations = 1;
        } else if (!strcmp(argv[arg], "--seed") && !seen_seed &&
            value <= UINT32_MAX) {
            seed = value ? (uint32_t)value : UINT32_C(0x9E3779B9);
            seen_seed = 1;
        } else break;
    }
    if (arg != argc) {
        fputs("Usage: layerkeysort_parser_torture [--iterations N] [--seed S]\n", stderr);
        return 2;
    }
    for (i = 0; i < iterations; ++i) {
        char display[256], key[256], random_text[256];
        size_t j, length;
        if (!make_valid(&seed, display, key)) return 1;
        if (!parse_check(display, 0) || !parse_check(key, 1)) goto failed;
        mutate(display, &seed); mutate(key, &seed);
        if (!parse_check(display, 0) || !parse_check(key, 1)) goto failed;
        length = random32(&seed) % 128u;
        for (j = 0; j < length; ++j)
            random_text[j] = (char)(random32(&seed) % 128u);
        random_text[length] = 0;
        if (!parse_check(random_text, 0) || !parse_check(random_text, 1) ||
            !parse_check("000", 0) || !parse_check("LK1:1!", 1)) goto failed;
        continue;
failed:
        fprintf(stderr, "parser torture failure case=%zu seed-state=%08X\n",
            i, (unsigned int)seed);
        return 1;
    }
    printf("Parser torture cases=%zu PASS\n", iterations);
    return 0;
}
