#ifndef SEARCH_H
#define SEARCH_H

#include "defs.h"

#define INFINITY 30000
#define MATE_SCORE 29000

#ifndef MAX_PLY
#define MAX_PLY 64
#endif

// Move ordering base scores
#define TT_MOVE_SCORE 2000000
#define CAPTURE_SCORE_BASE 1000000
#define KILLER_1_SCORE 900000
#define KILLER_2_SCORE 800000

typedef struct {
    U64 starttime;
    U64 stoptime;
    int depth;
    int depthset;
    int timeset;
    int movestogo;
    int infinite;

    U64 nodes;

    int quit;
    int stopped;

    int seldepth;

    uint32_t killer_moves[2][MAX_PLY];
    int history_moves[12][64];

    uint32_t pv_table[64][64];
    int pv_length[64];
} SearchInfo;

/**
 * Performs a search to find the best move in the current position.
 * @param board Pointer to the Board structure.
 * @param info Pointer to the SearchInfo structure.
 * @return The packed 32-bit best move found.
 */
extern uint32_t search_best_move(Board* board, SearchInfo* info);

#endif // SEARCH_H