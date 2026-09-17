#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "defs.h"

void generate_all_moves(const Board* pos, MoveList* list);
bool is_square_attacked(const Board* pos, int sq, int attacker_side);
long long perft_test(Board* pos, int depth);
void perft_divide(Board* pos, int depth);
void add_quiet_move(const Board* pos, uint32_t move, MoveList* list);
void add_capture_move(const Board* pos, uint32_t move, MoveList* list);

#endif
