#ifndef POLYGLOT_H
#define POLYGLOT_H

#include "defs.h"

/**
 * @file polyglot.h
 * @brief Declarations for Polyglot opening book and Zobrist hashing.
 */

/**
 * @struct PolyglotEntry
 * @brief Represents a single 16-byte record in a standard Polyglot opening book file.
 */
typedef struct {
    uint64_t key;     /**< 64-bit Zobrist key for the position. */
    uint16_t move;    /**< 16-bit move encoding in Polyglot format. */
    uint16_t weight;  /**< 16-bit weight of the move (higher is preferred). */
    uint32_t learn;   /**< 32-bit learning information (typically unused). */
} PolyglotEntry;

/**
 * @brief The path to the opening book binary file.
 */
extern char book_file_path[256];

/**
 * @brief Flag indicating whether the opening book should be used.
 */
extern bool use_book;

/**
 * @brief Generates a Polyglot-compatible Zobrist hash for the current board position.
 * 
 * @param board Pointer to the Board structure.
 * @return uint64_t The 64-bit Zobrist hash.
 */
uint64_t polyglot_hash(Board *board);

/**
 * @brief Checks the opening book dynamically for a move matching the current board state.
 * 
 * @param board Pointer to the Board structure.
 * @return uint32_t A packed 32-bit move if found, or 0 if not.
 */
uint32_t get_polyglot_move(Board *board);

/**
 * @brief Initializes and loads the opening book from book_file_path into memory.
 */
void InitPolyBook(void);

/**
 * @brief Cleans up and frees the memory allocated for the opening book.
 */
void CleanPolyBook(void);

/**
 * @brief Queries the loaded opening book in memory for a playable move.
 * 
 * @param board Pointer to the Board structure.
 * @return uint32_t A packed 32-bit move, or 0 if no book moves are found.
 */
uint32_t GetBookMove(Board *board);

#endif /* POLYGLOT_H */
