/**
 * @file polybook.c
 * @brief Polyglot opening book reader implementation.
 * 
 * This file handles parsing and querying Polyglot-compatible chess opening book files (.bin).
 * It reads the book into memory, calculates keys based on the board, and returns randomly selected book moves.
 */

#include "defs.h"
#include "polykeys.h"
#include "polyglot.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/** @brief Number of entries successfully parsed from the opening book file. */
long NumEntries = 0;

/** @brief Dynamic array containing all loaded opening book entries. */
PolyglotEntry *entries = NULL;

/**
 * @brief Piece code mapping from engine representation to Polyglot standard format.
 *
 * Indexed by the engine's piece values: wp, wn, wb, wr, wq, wk, bp, bn, bb, br, bq, bk, EMPTY.
 */
const int PolyKindOfPiece[13] = {
    1, 3, 5, 7, 9, 11, 0, 2, 4, 6, 8, 10, -1
};

/**
 * @brief Initializes and loads the Polyglot opening book into memory.
 * 
 * Attempts to open the book file at the path defined by book_file_path.
 * If successful, determines the size, allocates memory, and reads all entries into memory.
 */
void InitPolyBook(void) {
    use_book = false;
    
    FILE *pFile = fopen(book_file_path, "rb");
    
    if(pFile == NULL) {
        printf("Book File Not Read\n");
    } else {
        fseek(pFile, 0, SEEK_END);
        long position = ftell(pFile);
        
        if(position < (long)sizeof(PolyglotEntry)) {
            printf("No Entries Found\n");
            fclose(pFile);
            return;
        }
        
        NumEntries = position / sizeof(PolyglotEntry);
        printf("%ld Entries Found In File\n", NumEntries);
        
        entries = (PolyglotEntry*)malloc(NumEntries * sizeof(PolyglotEntry));
        if (entries == NULL) {
            printf("Memory allocation failed for book entries\n");
            fclose(pFile);
            return;
        }
        rewind(pFile);
        
        size_t returnValue = fread(entries, sizeof(PolyglotEntry), NumEntries, pFile);
        printf("fread() %zu Entries Read in from file\n", returnValue);
        
        if(NumEntries > 0) {
            use_book = true;
        }
        fclose(pFile);
    }
}

/**
 * @brief Frees the memory allocated for the opening book entries.
 */
void CleanPolyBook(void) {
    if (entries) {
        free(entries);
        entries = NULL;
    }
}

/**
 * @brief Checks if there is a capturing pawn available for the active side to perform an en passant capture.
 *
 * @param board Pointer to the Board structure.
 * @return true if a capturing pawn exists, false otherwise.
 */
bool HasPawnForCapture(const Board *board) {
    int sqWithPawn = 0;
    int targetPce = (board->side == white) ? wp : bp;
    if(board->enpassant != NO_SQ) {
        if(board->side == white) {
            sqWithPawn = board->enpassant - 8;
        } else {
            sqWithPawn = board->enpassant + 8;
        }
        
        int file = board->enpassant % 8;
        if(file > 0 && board->pieces[sqWithPawn - 1] == targetPce) {
            return true;
        }
        if(file < 7 && board->pieces[sqWithPawn + 1] == targetPce) {
            return true;
        } 
    }
    return false;
}

/**
 * @brief Computes a Polyglot Zobrist key for the current board position.
 * 
 * Hashing includes pieces placement, castling permissions, en passant target squares
 * (only when capturable), and the active player's side to move.
 *
 * @param board Pointer to the Board structure.
 * @return U64 The calculated Polyglot-compatible Zobrist key.
 */
U64 PolyKeyFromBoard(const Board *board) {
    U64 finalKey = 0;
    
    for(int sq = 0; sq < 64; ++sq) {
        int piece = board->pieces[sq];
        if(piece != EMPTY) {
            int polyPiece = PolyKindOfPiece[piece];
            if (polyPiece != -1) {
                finalKey ^= Random64Poly[(64 * polyPiece) + sq];
            }
        }
    }
    
    // castling
    if(board->castle & WKCA) finalKey ^= Random64Poly[768];
    if(board->castle & WQCA) finalKey ^= Random64Poly[769];
    if(board->castle & BKCA) finalKey ^= Random64Poly[770];
    if(board->castle & BQCA) finalKey ^= Random64Poly[771];
    
    // enpassant
    if(board->enpassant != NO_SQ && HasPawnForCapture(board)) {
        int file = board->enpassant % 8;
        finalKey ^= Random64Poly[772 + file];
    }
    
    if(board->side == white) {
        finalKey ^= Random64Poly[780];
    }
    return finalKey;
}

/**
 * @brief Swaps the byte-order of a 16-bit unsigned integer (big-endian to little-endian).
 *
 * @param x The 16-bit integer to swap.
 * @return unsigned short The swapped integer.
 */
static unsigned short endian_swap_u16(unsigned short x) { 
    return (x >> 8) | (x << 8); 
} 

/**
 * @brief Swaps the byte-order of a 64-bit unsigned integer (big-endian to little-endian).
 *
 * @param x The 64-bit integer to swap.
 * @return U64 The swapped integer.
 */
static U64 endian_swap_u64(U64 x) { 
    return (x >> 56) | 
           ((x << 40) & 0x00FF000000000000ULL) | 
           ((x << 24) & 0x0000FF0000000000ULL) | 
           ((x << 8)  & 0x000000FF00000000ULL) | 
           ((x >> 8)  & 0x00000000FF000000ULL) | 
           ((x >> 24) & 0x0000000000FF0000ULL) | 
           ((x >> 40) & 0x000000000000FF00ULL) | 
           (x << 56); 
}

/**
 * @brief Converts a 16-bit Polyglot move representation to our engine's packed 32-bit move representation.
 *
 * @param polyMove The 16-bit Polyglot move.
 * @param board Pointer to the Board structure.
 * @return uint32_t The packed 32-bit move, or 0 if invalid.
 */
uint32_t ConvertPolyMoveToInternalMove(unsigned short polyMove, Board *board) {
    int ff = (polyMove >> 6) & 7;
    int fr = (polyMove >> 9) & 7;
    int tf = (polyMove >> 0) & 7;
    int tr = (polyMove >> 3) & 7;
    int pp = (polyMove >> 12) & 7;
    
    char moveString[6];
    if(pp == 0) {
        sprintf(moveString, "%c%c%c%c",
        'a' + ff,
        '1' + fr,
        'a' + tf,
        '1' + tr);
    } else {
        char promChar = 'q';
        switch(pp) {
            case 1: promChar = 'n'; break;
            case 2: promChar = 'b'; break;
            case 3: promChar = 'r'; break;
        }
        sprintf(moveString, "%c%c%c%c%c",
        'a' + ff,
        '1' + fr,
        'a' + tf,
        '1' + tr,
        promChar);
    }
    
    return parse_move(moveString, board);
}

/**
 * @brief Queries the loaded opening book for a playable move.
 * 
 * Computes the key, searches all matching entries, and randomly selects
 * a candidate move among the retrieved book options.
 *
 * @param board Pointer to the Board structure.
 * @return uint32_t A packed 32-bit move found in the book, or 0 if no moves are found.
 */
uint32_t GetBookMove(Board *board) {
    if (!use_book || entries == NULL || NumEntries == 0) return 0;
    
    unsigned short move;
    const int MAXBOOKMOVES = 32;
    uint32_t bookMoves[MAXBOOKMOVES];
    int count = 0;
    
    U64 polyKey = PolyKeyFromBoard(board);
    
    for(PolyglotEntry *entry = entries; entry < entries + NumEntries; entry++) {
        if(polyKey == endian_swap_u64(entry->key)) {
            move = endian_swap_u16(entry->move);
            uint32_t tempMove = ConvertPolyMoveToInternalMove(move, board);
            if(tempMove != 0) {
                bookMoves[count++] = tempMove;
                if(count >= MAXBOOKMOVES) break;
            }
        }
    }
    
    if(count != 0) {
        int randMove = rand() % count;
        return bookMoves[randMove];
    } else {
        return 0;
    }
}