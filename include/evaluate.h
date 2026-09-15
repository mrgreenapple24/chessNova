#ifndef EVALUATE_H
#define EVALUATE_H

#include "board.h"

/* Piece values - Middlegame */
#define MG_PAWN_VALUE 82
#define MG_KNIGHT_VALUE 337
#define MG_BISHOP_VALUE 365
#define MG_ROOK_VALUE 477
#define MG_QUEEN_VALUE 1025
#define MG_KING_VALUE 0

/* Piece values - Endgame */
#define EG_PAWN_VALUE 94
#define EG_KNIGHT_VALUE 281
#define EG_BISHOP_VALUE 297
#define EG_ROOK_VALUE 512
#define EG_QUEEN_VALUE 936
#define EG_KING_VALUE 0

/* Positional Constants - Middlegame */
#define MG_BISHOP_PAIR 30
#define MG_ISOLATED_PAWN -10
#define MG_DOUBLED_PAWN -15
#define MG_ROOK_ON_7TH 20
#define MG_ROOK_ON_7TH_TRAPPED_KING 10

/* Positional Constants - Endgame */
#define EG_BISHOP_PAIR 50
#define EG_ISOLATED_PAWN -20
#define EG_DOUBLED_PAWN -20
#define EG_PASSED_PAWN 20
#define EG_ROOK_ON_7TH 35
#define EG_ROOK_ON_7TH_TRAPPED_KING 15

/* King Safety */
#define MG_KING_ATTACK_PENALTY -5
#define EG_KING_ATTACK_PENALTY -2
#define MG_PAWN_STORM_ATTACK_PENALTY -10
#define EG_PAWN_STORM_ATTACK_PENALTY -20

#define FLIP(sq) ((sq) ^ 56)

/**
 * Initializes masks used for evaluation (e.g., pawn masks).
 */
extern void init_evaluation_masks();

/**
 * Returns a score for the current position from the perspective of the side to
 * move. Positive score is good for the side to move.
 */
int evaluate(const Board* board);

#endif
