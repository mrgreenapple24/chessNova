/**
 * @file polyglot.c
 * @brief Polyglot Zobrist hash generator and dynamic book move lookup implementation.
 * 
 * Provides functions to generate Zobrist hashes conforming to the Polyglot opening book specification,
 * as well as functions to dynamically search and read entries from a Polyglot book binary on disk.
 */

#include "polyglot.h"
#include "polykeys.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* Platform-specific headers for endianness */
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <stdlib.h>  // for _byteswap_uint64
    #pragma comment(lib, "ws2_32.lib")

    #define bswap64(x) _byteswap_uint64(x)
    #define bswap32(x) ntohl(x)
    #define bswap16(x) ntohs(x)
#elif defined(__APPLE__)
    #include <arpa/inet.h>  // for ntohl/ntohs
    #include <libkern/OSByteOrder.h>

    #define bswap64(x) OSSwapInt64(x)
    #define bswap32(x) ntohl(x)
    #define bswap16(x) ntohs(x)
#elif defined(__linux__)
    #include <arpa/inet.h>
    #include <endian.h>

    #define bswap64(x) be64toh(x)
    #define bswap32(x) be32toh(x)
    #define bswap16(x) be16toh(x)
#else
    /* Fallback for other systems */
    #include <arpa/inet.h>

    /**
     * @brief Fallback helper to swap the byte order of a 64-bit unsigned integer.
     * 
     * @param x The 64-bit integer to swap.
     * @return uint64_t The byte-swapped integer.
     */
    static inline uint64_t bswap64(uint64_t x) {
        return ((x & 0x00000000000000FFULL) << 56) |
               ((x & 0x000000000000FF00ULL) << 40) |
               ((x & 0x0000000000FF0000ULL) << 24) |
               ((x & 0x00000000FF000000ULL) << 8) |
               ((x & 0x000000FF00000000ULL) >> 8) |
               ((x & 0x0000FF0000000000ULL) >> 24) |
               ((x & 0x00FF000000000000ULL) >> 40) |
               ((x & 0xFF00000000000000ULL) >> 56);
    }
    #define bswap32(x) ntohl(x)
    #define bswap16(x) ntohs(x)
#endif

#ifdef _MSC_VER
#  define U64(u) (u##ui64)
#else
#  define U64(u) (u##ULL)
#endif

/** @brief File path to the opening book binary file. Default is "book.bin". */
char book_file_path[256] = "book.bin";

/** @brief Configures whether opening book querying is enabled. */
bool use_book = true;

/**
 * @brief Maps an engine piece value to its corresponding Polyglot piece format code.
 * 
 * @param piece The engine's piece code (e.g. wp, bn, etc.).
 * @return int The Polyglot piece value, or -1 if invalid or empty.
 */
static int piece_to_polyglot(int piece) {
  switch (piece) {
    case bp: return 0;
    case wp: return 1;
    case bn: return 2;
    case wn: return 3;
    case bb: return 4;
    case wb: return 5;
    case br: return 6;
    case wr: return 7;
    case bq: return 8;
    case wq: return 9;
    case bk: return 10;
    case wk: return 11;
    default: return -1;
  }
}

/**
 * @brief Computes a Polyglot Zobrist hash for the current board position.
 * 
 * Includes active pieces, castling permissions, en passant squares (if capturable),
 * and side to move (White).
 * 
 * @param board Pointer to the Board structure.
 * @return uint64_t The computed 64-bit Polyglot Zobrist key.
 */
uint64_t polyglot_hash(Board *board) {
    uint64_t hash = 0ULL;

    for (int sq = 0; sq < 64; sq++) {
        int piece = board->pieces[sq];
        if (piece != EMPTY) {
            int idx = piece_to_polyglot(piece);
            hash ^= Random64Poly[64 * idx + sq];
        }
    }

    if (board->castle & WKCA) hash ^= Random64Poly[768];
    if (board->castle & WQCA) hash ^= Random64Poly[769];
    if (board->castle & BKCA) hash ^= Random64Poly[770];
    if (board->castle & BQCA) hash ^= Random64Poly[771];

    if (board->enpassant != NO_SQ) {
        int file = board->enpassant % 8; // 0 for 'a', 7 for 'h'
        bool can_capture = false;

        if (board->side == white) {
            // White to move, look for White pawns that can capture
            if (file > 0 && board->pieces[board->enpassant - 9] == wp) can_capture = true;
            if (file < 7 && board->pieces[board->enpassant - 7] == wp) can_capture = true;
        } else {
            // Black to move, look for Black pawns that can capture
            if (file > 0 && board->pieces[board->enpassant + 7] == bp) can_capture = true;
            if (file < 7 && board->pieces[board->enpassant + 9] == bp) can_capture = true;
        }

        if (can_capture) {
            hash ^= Random64Poly[772 + file];
        }
    }

    if (board->side == white)
        hash ^= Random64Poly[780];

    return hash;
}

/**
 * @brief Converts a 16-bit Polyglot move representation to our engine's packed 32-bit move representation.
 * 
 * Generates all legal moves for the current board and returns the match.
 * 
 * @param pg_move The 16-bit Polyglot move.
 * @param board Pointer to the Board structure.
 * @return uint32_t The packed 32-bit move, or 0 if invalid.
 */
static uint32_t convert_polyglot_move(uint16_t pg_move, Board *board) {
    int to_sq = pg_move & 0x3F;
    int from_sq = (pg_move >> 6) & 0x3F;
    int promote = (pg_move >> 12) & 0x7;

    MoveList list;
    generate_all_moves(board, &list);

    for (int i = 0; i < list.count; i++) {
        uint32_t move = list.moves[i].move;
        if ((int)GET_FROM(move) == from_sq && (int)GET_TO(move) == to_sq) {
            int promoted = GET_PROMOTED(move);
            if (promote == 0 && promoted == EMPTY) return move;
            if (promote == 1 && (promoted == wn || promoted == bn)) return move;
            if (promote == 2 && (promoted == wb || promoted == bb)) return move;
            if (promote == 3 && (promoted == wr || promoted == br)) return move;
            if (promote == 4 && (promoted == wq || promoted == bq)) return move;
        }
    }

    return 0;
}

/**
 * @brief Checks the opening book for a move by dynamically scanning the book on disk using binary search.
 * 
 * @param board Pointer to the Board structure.
 * @return uint32_t A packed 32-bit move if found, or 0 if not found.
 */
uint32_t get_polyglot_move(Board *board) {
    if (!use_book) return 0;

    FILE *fp = fopen(book_file_path, "rb");
    if (!fp) return 0;

    uint64_t key = polyglot_hash(board);

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    long entries = size / sizeof(PolyglotEntry);

    long low = 0;
    long high = entries - 1;
    long first_entry = -1;

    while (low <= high) {
        long mid = low + (high - low) / 2;
        PolyglotEntry entry;
        fseek(fp, mid * sizeof(PolyglotEntry), SEEK_SET);
        if (fread(&entry, sizeof(PolyglotEntry), 1, fp) != 1) break;

        uint64_t entry_key = bswap64(entry.key);

        if (entry_key == key) {
            first_entry = mid;
            high = mid - 1;
        } else if (entry_key < key) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    if (first_entry == -1) {
        fclose(fp);
        return 0;
    }

    uint32_t candidate_moves[256];
    int candidate_weights[256];
    int count = 0;
    int total_weight = 0;

    fseek(fp, first_entry * sizeof(PolyglotEntry), SEEK_SET);
    PolyglotEntry entry;
    while (fread(&entry, sizeof(PolyglotEntry), 1, fp) == 1) {
        if (bswap64(entry.key) != key) break;

        uint32_t move = convert_polyglot_move(bswap16(entry.move), board);
        if (move) {
            candidate_moves[count] = move;
            candidate_weights[count] = bswap16(entry.weight);
            total_weight += candidate_weights[count];
            count++;
        }
        if (count >= 256) break;
    }

    fclose(fp);

    if (count == 0) return 0;

    if (total_weight > 0) {
        int r = rand() % total_weight;
        int sum = 0;
        for (int i = 0; i < count; i++) {
            sum += candidate_weights[i];
            if (r < sum) return candidate_moves[i];
        }
    }

    return candidate_moves[0];
}
