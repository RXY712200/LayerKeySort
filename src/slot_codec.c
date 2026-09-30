#include "lks_slot_codec_internal.h"

/* ASCII-ordered alphanumerics omit 0/O/o and 1/I/i/L/l to make handwritten
 * Paths easier to inspect. No punctuation can collide with the '/' separator
 * or require escaping in common logs, URLs, and shell/config contexts. The
 * fixed three-character width covers all 65536 slots because 54^3 > 65536. */
static const char slot_alphabet[] =
    "23456789ABCDEFGHJKMNPQRSTUVWXYZabcdefghjkmnpqrstuvwxyz";

_Static_assert(sizeof(slot_alphabet) == LKS_SLOT_TEXT_RADIX + 1,
    "slot alphabet must contain exactly 54 characters");
_Static_assert(LKS_SLOT_TEXT_WIDTH == 3,
    "the Preview.3 slot grammar uses exactly three characters");
_Static_assert(LKS_SLOT_TEXT_RADIX * LKS_SLOT_TEXT_RADIX * LKS_SLOT_TEXT_RADIX >
    LKS_PATH_SLOT_MAX, "three radix-54 digits must cover every slot");

int lks_slot_encode(unsigned int slot, char output[LKS_SLOT_TEXT_WIDTH])
{
    int digit_index;
    unsigned int remaining_slot = slot;
    if (output == NULL || slot > LKS_PATH_SLOT_MAX) return 0;
    for (digit_index = LKS_SLOT_TEXT_WIDTH - 1; digit_index >= 0; --digit_index) {
        output[digit_index] = slot_alphabet[remaining_slot % LKS_SLOT_TEXT_RADIX];
        remaining_slot /= LKS_SLOT_TEXT_RADIX;
    }
    return 1;
}

int lks_slot_decode(const char input[LKS_SLOT_TEXT_WIDTH], unsigned int *out_slot)
{
    unsigned int value = 0;
    int i;
    if (input == NULL || out_slot == NULL) return 0;
    for (i = 0; i < LKS_SLOT_TEXT_WIDTH; ++i) {
        unsigned int rank;
        for (rank = 0; rank < LKS_SLOT_TEXT_RADIX; ++rank)
            if (input[i] == slot_alphabet[rank]) break;
        if (rank == LKS_SLOT_TEXT_RADIX) return 0;
        value = value * LKS_SLOT_TEXT_RADIX + rank;
    }
    if (value > LKS_PATH_SLOT_MAX) return 0;
    *out_slot = value;
    return 1;
}
