#ifndef LKS_SLOT_CODEC_INTERNAL_H
#define LKS_SLOT_CODEC_INTERNAL_H

#include "layerkeysort.h"

enum { LKS_SLOT_TEXT_RADIX = 54, LKS_SLOT_TEXT_WIDTH = 3 };

/* Fixed-width, ASCII-ordered token. Returns zero for an out-of-range slot. */
int lks_slot_encode(unsigned int slot, char output[LKS_SLOT_TEXT_WIDTH]);

#endif
