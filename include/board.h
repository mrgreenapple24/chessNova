#ifndef BOARD_H
#define BOARD_H
#include "stdbool.h"
#include "types.h"
typedef struct {
    uint32_t move;
    int castle;
    int enpassant;
    int fiftyMove;
    U64 posKey;
} Undo;

typedef struct {
    int pieces[64];
    U64 bitboards[12];  // One for each piece type
    U64 occupancies[3]; // black white or both

    int side;
    int enpassant;
    int castle;
    int fiftyMove;
    int ply;
    int hisply;

    U64 posKey;

    Undo history[MAX_PLY]; // History stack for unmaking moves
} Board;

void print_board(Board* board);
void print_bitboard(U64 bitboard);
void reset_board(Board* board);
void parse_fen(const char* fen, Board* board);
int get_piece_at(const Board* board, int sq);
bool make_move(Board* board, uint32_t move);
void unmake_move(Board* board);
void make_null_move(Board* board);
void unmake_null_move(Board* board);

#endif
