#ifndef POLYGLOT_H
#define POLYGLOT_H

#include "defs.h"

/* Polyglot book entry structure */
typedef struct {
    uint64_t key;     /* 64-bit Zobrist hash */
    uint16_t move;    /* 16-bit move */
    uint16_t weight;  /* 16-bit weight */
    uint32_t learn;   /* 32-bit learn field */
} PolyglotEntry;

/* Global book settings */
extern char book_file_path[256];
extern bool use_book;

/**
 * Generates a Polyglot-compatible Zobrist hash for the current board.
 */
uint64_t polyglot_hash(Board *board);

/**
 * Checks the opening book for a move.
 * Returns a packed 32-bit move if found, or 0 if not.
 */
uint32_t get_polyglot_move(Board *board);

void InitPolyBook(void);
void CleanPolyBook(void);
uint32_t GetBookMove(Board *board);

#endif /* POLYGLOT_H */
