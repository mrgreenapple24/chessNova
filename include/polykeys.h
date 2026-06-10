#ifndef POLYKEYS_H
#define POLYKEYS_H

/**
 * @file polykeys.h
 * @brief Declarations for Polyglot Zobrist hashing random keys.
 */

#include <stdint.h>
#include "types.h"

#ifdef _MSC_VER
#  define U64_POLY(u) (u##ui64)
#else
#  define U64_POLY(u) (u##ULL)
#endif

/**
 * @brief Table of 781 pseudo-random 64-bit keys used specifically for generating Polyglot Zobrist hashes.
 *
 * The keys are ordered as follows:
 * - 0 to 767: Piece on square (12 pieces * 64 squares)
 * - 768: White to move
 * - 769 to 772: Castling rights (WKCA, WQCA, BKCA, BQCA)
 * - 773 to 780: En passant file (files A-H, only if a double pawn push occurs and a capturing pawn is adjacent)
 */
extern const U64 Random64Poly[781];

#endif /* POLYKEYS_H */