#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef uint64_t U64;

#define MAX_MOVES 256
#define MAX_PLY 1024
#define notAFile 0xfefefefefefefefeULL
#define notHFile 0x7f7f7f7f7f7f7f7fULL

#define rank1 0x00000000000000FFULL
#define rank2 0x000000000000FF00ULL
#define rank7 0x00FF000000000000ULL
#define rank8 0xFF00000000000000ULL

enum { white, black, both };
enum { wp, wn, wb, wr, wq, wk, bp, bn, bb, br, bq, bk, EMPTY };

enum {
    a1,
    b1,
    c1,
    d1,
    e1,
    f1,
    g1,
    h1,
    a2,
    b2,
    c2,
    d2,
    e2,
    f2,
    g2,
    h2,
    a3,
    b3,
    c3,
    d3,
    e3,
    f3,
    g3,
    h3,
    a4,
    b4,
    c4,
    d4,
    e4,
    f4,
    g4,
    h4,
    a5,
    b5,
    c5,
    d5,
    e5,
    f5,
    g5,
    h5,
    a6,
    b6,
    c6,
    d6,
    e6,
    f6,
    g6,
    h6,
    a7,
    b7,
    c7,
    d7,
    e7,
    f7,
    g7,
    h7,
    a8,
    b8,
    c8,
    d8,
    e8,
    f8,
    g8,
    h8,
    NO_SQ,
    OFFBOARD
};

/**
 * Move Bit Format:
 * 0-5: From (6 bits)
 * 6-11: To (6 bits)
 * 12-15: Captured (4 bits)
 * 16-19: Promoted (4 bits)
 * 20: EnPassant (1 bit)
 * 21: PawnStart (1 bit)
 * 22: Castle (1 bit)
 * 23: Capture Flag (1 bit)
 */

typedef struct {
    uint32_t move;
    int score; // Used for move ordering
} Move;

typedef struct {
    Move moves[MAX_MOVES];
    int count;
} MoveList;

/* Macros to extract move info */
#define GET_FROM(m) ((m) & 0x3F)
#define GET_TO(m) (((m) >> 6) & 0x3F)
#define GET_CAPTURED(m) (((m) >> 12) & 0xF)
#define GET_PROMOTED(m) (((m) >> 16) & 0xF)

/* Flags */
#define MFLAG_EP 0x100000
#define MFLAG_PS 0x200000
#define MFLAG_CA 0x400000
#define MFLAG_CAP 0x800000

/* Macros to pack a move (f=from, t=to, cap=captured, pro=promoted, fl=flags) */
#define MOVE(f, t, cap, pro, fl) ((f) | ((t) << 6) | ((cap) << 12) | ((pro) << 16) | (fl))

enum { WKCA = 1, WQCA = 2, BKCA = 4, BQCA = 8 }; // castling rights

enum { Nort, Sout, East, West, NoEa, NoWe, SoEa, SoWe };

enum { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NONE };
enum { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NONE };
#endif
