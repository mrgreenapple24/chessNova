#ifndef BITBOARD_H
#define BITBOARD_H

#include "board.h"
#include "types.h"
#include <stdbool.h>
#include <stdint.h>

#if defined(_MSC_VER)
#include <intrin.h>

static inline int count_bits(uint64_t x) {
    return __popcnt64(x);
}

static inline int get_lsb(uint64_t x) {
    unsigned long index;
    _BitScanForward64(&index, x);
    return index;
}

#else

static inline int count_bits(uint64_t x) {
    return __builtin_popcountll(x);
}

static inline int get_lsb(uint64_t x) {
    return __builtin_ctzll(x);
}

#endif

U64 shiftSouth(U64 b);
U64 shiftNorth(U64 b);

U64 shiftEast(U64 b);
U64 shiftNortheast(U64 b);
U64 shiftSoutheast(U64 b);
U64 shiftWest(U64 b);
U64 shiftSouthwest(U64 b);
U64 shiftNorthwest(U64 b);

U64 get_occupied(Board board);
U64 get_empty(U64 occupied);

extern U64 arrFiles[8];

bool test_bit(U64 b, int square);
void set_bit(U64* b, int square);
void clear_bit(U64* b, int square);
void toggle_bit(U64* b, int square);
int count_bits(U64 bb);
int get_lsb(U64 bb);
int pop_lsb(U64* bb);

#endif // BITBOARD_H
