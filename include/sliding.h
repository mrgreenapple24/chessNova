#ifndef SLIDING_H
#define SLIDING_H
#include "bitboard.h"
#include "types.h"

/* Magic Bitboard Structures */
typedef struct {
    U64* attacks;
    U64 mask;
    U64 magic;
    int shift;
} Magic;

extern Magic rook_magics[64];
extern Magic bishop_magics[64];

/* Function to get sliding attacks using Magic Bitboards */
static inline U64 get_bishop_attacks(int sq, U64 occ) {
    occ &= bishop_magics[sq].mask;
    occ *= bishop_magics[sq].magic;
    return bishop_magics[sq].attacks[occ >> bishop_magics[sq].shift];
}

static inline U64 get_rook_attacks(int sq, U64 occ) {
    occ &= rook_magics[sq].mask;
    occ *= rook_magics[sq].magic;
    return rook_magics[sq].attacks[occ >> rook_magics[sq].shift];
}

static inline U64 get_queen_attacks(int sq, U64 occ) {
    return get_bishop_attacks(sq, occ) | get_rook_attacks(sq, occ);
}

void init_magics();

#endif
