/**
 * @file sliding.c
 * @brief Sliding piece attack definitions and utilities.
 */

#include "sliding.h"
#include "defs.h"
#include <stdio.h>

// Note: Most sliding attack logic has been moved to magic.c
// for high-performance Magic Bitboard lookups.

// Mask bits for each square (used by init_magics in magic.c)
int rook_mask_bits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12, 11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10,
    10, 11, 11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10,
    10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11, 12, 11, 11, 11, 11, 11, 11, 12,
};

int bishop_mask_bits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 7, 7, 7, 7, 5, 5, 5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5, 5, 5, 7, 7, 7, 7, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 6,
};

/** @brief Set occupancy based on an index and mask */
U64 set_occupancy(int index, int bits_in_mask, U64 mask) {
    U64 occupancy = 0ULL;
    for (int i = 0; i < bits_in_mask; i++) {
        int square = get_lsb(mask);
        clear_bit(&mask, square);
        if (index & (1 << i))
            occupancy |= (1ULL << square);
    }
    return occupancy;
}
