#ifndef TT_H
#define TT_H

#include "types.h"
#include <stdbool.h>

// Bound flags matching standard alpha-beta semantics
#define TT_NONE 0
#define TT_EXACT 1
#define TT_ALPHA 2 // UPPER_BOUND (score <= alpha)
#define TT_BETA 3  // LOWER_BOUND (score >= beta)

typedef struct {
    U64 key;       // Full 64-bit Zobrist key for collision verification
    uint32_t move; // Best/refutation move packed as 32-bit uint32_t
    int16_t score; // Search evaluation / score
    int16_t depth; // Search depth remaining
    uint8_t flag;  // TT_EXACT, TT_ALPHA, or TT_BETA
    uint8_t age;   // Search age/generation for replacement policy
} TTEntry;

typedef struct {
    TTEntry* pTable;
    uint32_t numEntries; // Number of entries (power of 2)
    uint8_t currentAge;  // Global search generation/age
} TranspositionTable;

extern TranspositionTable TT;

/**
 * Initializes and allocates memory for the Transposition Table.
 * @param size_mb Allocation size in Megabytes.
 */
void init_tt(int size_mb);

/**
 * Clears all entries in the Transposition Table.
 */
void clear_tt(void);

/**
 * Frees allocated memory for the Transposition Table.
 */
void free_tt(void);

/**
 * Increments global search generation/age.
 */
void increment_tt_age(void);

/**
 * Probes the TT for an entry matching key.
 * Returns true if a valid depth cutoff occurred, setting *score.
 * Always retrieves *move if key matches (useful for move ordering).
 */
bool probe_tt(U64 key, int depth, int alpha, int beta, int ply, int* score, uint32_t* move);

/**
 * Retrieves the best move stored for the given key without cutoff evaluation.
 * Returns 0 if key not found.
 */
uint32_t probe_tt_move(U64 key);

/**
 * Returns hash table utilization per mille (0 to 1000).
 */
int get_hashfull(void);

/**
 * Stores a new search result entry into the TT using Depth-Preferred + Age replacement.
 */
void store_tt(U64 key, uint32_t move, int score, int depth, uint8_t flag, int ply);

/**
 * Adjusts mate score before storing into TT.
 */
int16_t score_to_tt(int score, int ply);

/**
 * Adjusts mate score retrieved from TT back to current search ply depth.
 */
int score_from_tt(int16_t score, int ply);

#endif // TT_H