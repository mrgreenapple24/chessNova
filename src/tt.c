#include "tt.h"
#include "search.h" // For MATE_SCORE definition
#include <stdlib.h>
#include <string.h>

TranspositionTable TT = {NULL, 0, 0};

int16_t score_to_tt(int score, int ply) {
    if (score > MATE_SCORE - 1000) {
        return (int16_t)(score + ply);
    }
    if (score < -MATE_SCORE + 1000) {
        return (int16_t)(score - ply);
    }
    return (int16_t)score;
}

int score_from_tt(int16_t score, int ply) {
    if (score > MATE_SCORE - 1000) {
        return (int)(score - ply);
    }
    if (score < -MATE_SCORE + 1000) {
        return (int)(score + ply);
    }
    return (int)score;
}

void init_tt(int size_mb) {
    free_tt();

    if (size_mb <= 0)
        size_mb = 16; // Default fallback to 16MB

    size_t bytes = (size_t)size_mb * 1024 * 1024;
    uint32_t entries = (uint32_t)(bytes / sizeof(TTEntry));

    // Round down entries to nearest power of 2
    uint32_t power_of_two = 1;
    while ((power_of_two << 1) != 0 && (power_of_two << 1) <= entries) {
        power_of_two <<= 1;
    }

    TT.numEntries = power_of_two;
    TT.currentAge = 0;
    TT.pTable = (TTEntry*)calloc(TT.numEntries, sizeof(TTEntry));
}

void clear_tt(void) {
    if (TT.pTable) {
        memset(TT.pTable, 0, TT.numEntries * sizeof(TTEntry));
    }
}

void free_tt(void) {
    if (TT.pTable) {
        free(TT.pTable);
        TT.pTable = NULL;
    }
    TT.numEntries = 0;
    TT.currentAge = 0;
}

void increment_tt_age(void) {
    TT.currentAge++;
}

bool probe_tt(U64 key, int depth, int alpha, int beta, int ply, int* score, uint32_t* move) {
    if (!TT.pTable || TT.numEntries == 0 || key == 0) {
        if (move)
            *move = 0;
        return false;
    }

    uint32_t index = (uint32_t)(key & (TT.numEntries - 1));
    TTEntry* entry = &TT.pTable[index];

    if (entry->key == key) {
        if (move)
            *move = entry->move;

        if (entry->depth >= depth) {
            int tt_score = score_from_tt(entry->score, ply);

            if (entry->flag == TT_EXACT) {
                *score = tt_score;
                return true;
            }
            if (entry->flag == TT_ALPHA && tt_score <= alpha) {
                *score = alpha;
                return true;
            }
            if (entry->flag == TT_BETA && tt_score >= beta) {
                *score = beta;
                return true;
            }
        }
    } else {
        if (move)
            *move = 0;
    }

    return false;
}

uint32_t probe_tt_move(U64 key) {
    if (!TT.pTable || TT.numEntries == 0 || key == 0)
        return 0;

    uint32_t index = (uint32_t)(key & (TT.numEntries - 1));
    TTEntry* entry = &TT.pTable[index];

    if (entry->key == key) {
        return entry->move;
    }
    return 0;
}

int get_hashfull(void) {
    if (!TT.pTable || TT.numEntries == 0)
        return 0;

    int sample_size = (TT.numEntries < 1000) ? (int)TT.numEntries : 1000;
    int occupied = 0;

    for (int i = 0; i < sample_size; i++) {
        if (TT.pTable[i].key != 0) {
            occupied++;
        }
    }

    return (occupied * 1000) / sample_size;
}

void store_tt(U64 key, uint32_t move, int score, int depth, uint8_t flag, int ply) {
    if (!TT.pTable || TT.numEntries == 0 || key == 0)
        return;

    uint32_t index = (uint32_t)(key & (TT.numEntries - 1));
    TTEntry* entry = &TT.pTable[index];

    bool replace = false;

    if (entry->key == 0 || entry->key == key) {
        replace = true;
    } else if (entry->age != TT.currentAge) {
        replace = true;
    } else if (depth >= entry->depth) {
        replace = true;
    }

    if (replace) {
        if (entry->key == key && move == 0) {
            move = entry->move;
        }

        entry->key = key;
        entry->move = move;
        entry->score = score_to_tt(score, ply);
        entry->depth = (int16_t)depth;
        entry->flag = flag;
        entry->age = TT.currentAge;
    }
}