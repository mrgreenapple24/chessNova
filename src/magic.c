/**
 * @file magic.c
 * @brief Magic Bitboard initialization and attack table generation.
 */

#include "sliding.h"
#include <stdlib.h>
#include <time.h>

Magic rook_magics[64];
Magic bishop_magics[64];

U64 rook_attacks_table[0x19000];
U64 bishop_attacks_table[0x1480];

extern int rook_mask_bits[64];
extern int bishop_mask_bits[64];
extern U64 set_occupancy(int index, int bits_in_mask, U64 mask);

U64 mask_rook_attacks(int sq) {
    U64 attacks = 0ULL;
    int r = sq / 8, c = sq % 8;
    for (int i = r + 1; i <= 6; i++)
        attacks |= (1ULL << (i * 8 + c));
    for (int i = r - 1; i >= 1; i--)
        attacks |= (1ULL << (i * 8 + c));
    for (int i = c + 1; i <= 6; i++)
        attacks |= (1ULL << (r * 8 + i));
    for (int i = c - 1; i >= 1; i--)
        attacks |= (1ULL << (r * 8 + i));
    return attacks;
}

U64 mask_bishop_attacks(int sq) {
    U64 attacks = 0ULL;
    int r = sq / 8, c = sq % 8;
    for (int i = r + 1, j = c + 1; i <= 6 && j <= 6; i++, j++)
        attacks |= (1ULL << (i * 8 + j));
    for (int i = r + 1, j = c - 1; i <= 6 && j >= 1; i++, j--)
        attacks |= (1ULL << (i * 8 + j));
    for (int i = r - 1, j = c + 1; i >= 1 && j <= 6; i--, j++)
        attacks |= (1ULL << (i * 8 + j));
    for (int i = r - 1, j = c - 1; i >= 1 && j >= 1; i--, j--)
        attacks |= (1ULL << (i * 8 + j));
    return attacks;
}

U64 bishop_attacks_on_the_fly(int sq, U64 block) {
    U64 attacks = 0;
    int tr = sq / 8, tf = sq % 8;
    for (int r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block)
            break;
    }
    for (int r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block)
            break;
    }
    for (int r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block)
            break;
    }
    for (int r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block)
            break;
    }
    return attacks;
}

U64 rook_attacks_on_the_fly(int sq, U64 block) {
    U64 attacks = 0;
    int tr = sq / 8, tf = sq % 8;
    for (int r = tr + 1; r <= 7; r++) {
        attacks |= (1ULL << (r * 8 + tf));
        if ((1ULL << (r * 8 + tf)) & block)
            break;
    }
    for (int r = tr - 1; r >= 0; r--) {
        attacks |= (1ULL << (r * 8 + tf));
        if ((1ULL << (r * 8 + tf)) & block)
            break;
    }
    for (int f = tf + 1; f <= 7; f++) {
        attacks |= (1ULL << (tr * 8 + f));
        if ((1ULL << (tr * 8 + f)) & block)
            break;
    }
    for (int f = tf - 1; f >= 0; f--) {
        attacks |= (1ULL << (tr * 8 + f));
        if ((1ULL << (tr * 8 + f)) & block)
            break;
    }
    return attacks;
}

static U64 random_U64() {
    U64 u1, u2, u3, u4;
    u1 = (U64)(rand()) & 0xFFFF;
    u2 = (U64)(rand()) & 0xFFFF;
    u3 = (U64)(rand()) & 0xFFFF;
    u4 = (U64)(rand()) & 0xFFFF;
    return u1 | (u2 << 16) | (u3 << 32) | (u4 << 48);
}

static U64 find_magic(int sq, int bits, int bishop) {
    static U64 occupancies[4096], attacks[4096], used_attacks[4096];
    U64 mask = bishop ? mask_bishop_attacks(sq) : mask_rook_attacks(sq);
    int variations = 1 << bits;
    for (int i = 0; i < variations; i++) {
        occupancies[i] = set_occupancy(i, bits, mask);
        attacks[i] = bishop ? bishop_attacks_on_the_fly(sq, occupancies[i])
                            : rook_attacks_on_the_fly(sq, occupancies[i]);
    }
    for (int k = 0; k < 100000000; k++) {
        U64 magic = random_U64() & random_U64() & random_U64();
        if (count_bits((mask * magic) & 0xFF00000000000000ULL) < 6)
            continue;
        for (int i = 0; i < variations; i++)
            used_attacks[i] = 0ULL;
        int fail = 0;
        for (int i = 0; i < variations && !fail; i++) {
            int idx = (int)((occupancies[i] * magic) >> (64 - bits));
            if (used_attacks[idx] == 0ULL)
                used_attacks[idx] = attacks[i];
            else if (used_attacks[idx] != attacks[i])
                fail = 1;
        }
        if (!fail)
            return magic;
    }
    return 0ULL;
}

void init_magics() {
    srand(12345); // Fixed seed for reproducibility
    U64* rook_ptr = rook_attacks_table;
    U64* bishop_ptr = bishop_attacks_table;
    for (int sq = 0; sq < 64; sq++) {
        int b_bits = bishop_mask_bits[sq];
        bishop_magics[sq].mask = mask_bishop_attacks(sq);
        bishop_magics[sq].magic = find_magic(sq, b_bits, 1);
        bishop_magics[sq].shift = 64 - b_bits;
        bishop_magics[sq].attacks = bishop_ptr;
        int variations = 1 << b_bits;
        for (int i = 0; i < variations; i++) {
            U64 occ = set_occupancy(i, b_bits, bishop_magics[sq].mask);
            int magic_idx = (occ * bishop_magics[sq].magic) >> bishop_magics[sq].shift;
            bishop_magics[sq].attacks[magic_idx] = bishop_attacks_on_the_fly(sq, occ);
        }
        bishop_ptr += variations;

        int r_bits = rook_mask_bits[sq];
        rook_magics[sq].mask = mask_rook_attacks(sq);
        rook_magics[sq].magic = find_magic(sq, r_bits, 0);
        rook_magics[sq].shift = 64 - r_bits;
        rook_magics[sq].attacks = rook_ptr;
        variations = 1 << r_bits;
        for (int i = 0; i < variations; i++) {
            U64 occ = set_occupancy(i, r_bits, rook_magics[sq].mask);
            int magic_idx = (occ * rook_magics[sq].magic) >> rook_magics[sq].shift;
            rook_magics[sq].attacks[magic_idx] = rook_attacks_on_the_fly(sq, occ);
        }
        rook_ptr += variations;
    }
}
