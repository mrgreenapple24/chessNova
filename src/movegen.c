/**
 * @file movegen.c
 * @brief Pseudo-legal move generation and legality checking.
 */

#include "movegen.h"
#include "kings.h"
#include "knights.h"
#include "sliding.h"

/**
 * @brief Internal helper to add a move to the move list.
 * @param list Pointer to the MoveList.
 * @param move The packed 32-bit move.
 */
static void add_move(MoveList* list, uint32_t move) {
    list->moves[list->count].move = move;
    list->moves[list->count].score = 0;
    list->count++;
}

/**
 * @brief Checks if a square is attacked by any piece of a given side.
 * @param pos Pointer to the Board structure.
 * @param sq The square to check for attackers.
 * @param side The side that is potentially attacking the square.
 * @return true if the square is attacked, false otherwise.
 */
bool is_square_attacked(const Board* pos, int sq, int side) {
    // Pawns
    if (side == white) {
        if (arrbPawnAttacks[sq] & pos->bitboards[wp])
            return true;
    } else {
        if (arrwPawnAttacks[sq] & pos->bitboards[bp])
            return true;
    }
    // Knights
    if (arrKnightAttacks[sq] & ((side == white) ? pos->bitboards[wn] : pos->bitboards[bn]))
        return true;
    // Kings
    if (arrKingAttacks[sq] & ((side == white) ? pos->bitboards[wk] : pos->bitboards[bk]))
        return true;

    // Sliders (Rooks, Bishops, Queens)
    U64 occ = 0ULL;
    for (int p = wp; p <= bk; p++)
        occ |= pos->bitboards[p];

    U64 rooks_queens = (side == white) ? (pos->bitboards[wr] | pos->bitboards[wq])
                                       : (pos->bitboards[br] | pos->bitboards[bq]);
    if (get_rook_attacks(sq, occ) & rooks_queens)
        return true;

    U64 bishops_queens = (side == white) ? (pos->bitboards[wb] | pos->bitboards[wq])
                                         : (pos->bitboards[bb] | pos->bitboards[bq]);
    if (get_bishop_attacks(sq, occ) & bishops_queens)
        return true;

    return false;
}

/**
 * @brief Generates all pseudo-legal moves for the current side to move.
 * @param pos Pointer to the Board structure.
 * @param list Pointer to the MoveList to populate.
 */
void generate_all_moves(const Board* pos, MoveList* list) {
    list->count = 0;
    int side = pos->side;

    U64 occ = 0ULL;
    for (int p = wp; p <= bk; p++)
        occ |= pos->bitboards[p];

    U64 enemy_occ = 0ULL;
    int enemy_side = (side == white) ? black : white;
    for (int p = (enemy_side == white ? wp : bp); p <= (enemy_side == white ? wk : bk); p++)
        enemy_occ |= pos->bitboards[p];

    U64 friendly_occ = 0ULL;
    for (int p = (side == white ? wp : bp); p <= (side == white ? wk : bk); p++)
        friendly_occ |= pos->bitboards[p];

    U64 empty = ~occ;

    if (side == white) {
        U64 pawns = pos->bitboards[wp];
        U64 single_push = (pawns << 8) & empty;
        U64 double_push = ((single_push & 0x0000000000FF0000ULL) << 8) & empty;

        U64 temp_push = single_push;
        while (temp_push) {
            int to = pop_lsb(&temp_push);
            int from = to - 8;
            if (to >= a8) {
                add_move(list, MOVE(from, to, EMPTY, wn, 0));
                add_move(list, MOVE(from, to, EMPTY, wb, 0));
                add_move(list, MOVE(from, to, EMPTY, wr, 0));
                add_move(list, MOVE(from, to, EMPTY, wq, 0));
            } else
                add_move(list, MOVE(from, to, EMPTY, EMPTY, 0));
        }
        while (double_push) {
            int to = pop_lsb(&double_push);
            add_move(list, MOVE(to - 16, to, EMPTY, EMPTY, MFLAG_PS));
        }
        U64 left_caps = (pawns << 7) & enemy_occ & notHFile;
        while (left_caps) {
            int to = pop_lsb(&left_caps);
            int cap = get_piece_at(pos, to);
            if (to >= a8) {
                add_move(list, MOVE(to - 7, to, cap, wn, MFLAG_CAP));
                add_move(list, MOVE(to - 7, to, cap, wb, MFLAG_CAP));
                add_move(list, MOVE(to - 7, to, cap, wr, MFLAG_CAP));
                add_move(list, MOVE(to - 7, to, cap, wq, MFLAG_CAP));
            } else
                add_move(list, MOVE(to - 7, to, cap, EMPTY, MFLAG_CAP));
        }
        U64 right_caps = (pawns << 9) & enemy_occ & notAFile;
        while (right_caps) {
            int to = pop_lsb(&right_caps);
            int cap = get_piece_at(pos, to);
            if (to >= a8) {
                add_move(list, MOVE(to - 9, to, cap, wn, MFLAG_CAP));
                add_move(list, MOVE(to - 9, to, cap, wb, MFLAG_CAP));
                add_move(list, MOVE(to - 9, to, cap, wr, MFLAG_CAP));
                add_move(list, MOVE(to - 9, to, cap, wq, MFLAG_CAP));
            } else
                add_move(list, MOVE(to - 9, to, cap, EMPTY, MFLAG_CAP));
        }
        if (pos->enpassant != NO_SQ) {
            if ((pawns << 7) & (1ULL << pos->enpassant) & notHFile)
                add_move(list,
                         MOVE(pos->enpassant - 7, pos->enpassant, bp, EMPTY, MFLAG_EP | MFLAG_CAP));
            if ((pawns << 9) & (1ULL << pos->enpassant) & notAFile)
                add_move(list,
                         MOVE(pos->enpassant - 9, pos->enpassant, bp, EMPTY, MFLAG_EP | MFLAG_CAP));
        }
    } else {
        U64 pawns = pos->bitboards[bp];
        U64 single_push = (pawns >> 8) & empty;
        U64 double_push = ((single_push & 0x0000FF0000000000ULL) >> 8) & empty;

        U64 temp_push = single_push;
        while (temp_push) {
            int to = pop_lsb(&temp_push);
            int from = to + 8;
            if (to <= h1) {
                add_move(list, MOVE(from, to, EMPTY, bn, 0));
                add_move(list, MOVE(from, to, EMPTY, bb, 0));
                add_move(list, MOVE(from, to, EMPTY, br, 0));
                add_move(list, MOVE(from, to, EMPTY, bq, 0));
            } else
                add_move(list, MOVE(from, to, EMPTY, EMPTY, 0));
        }
        temp_push = double_push;
        while (temp_push) {
            int to = pop_lsb(&temp_push);
            add_move(list, MOVE(to + 16, to, EMPTY, EMPTY, MFLAG_PS));
        }
        U64 left_caps = (pawns >> 9) & enemy_occ & notHFile;
        while (left_caps) {
            int to = pop_lsb(&left_caps);
            int cap = get_piece_at(pos, to);
            if (to <= h1) {
                add_move(list, MOVE(to + 9, to, cap, bn, MFLAG_CAP));
                add_move(list, MOVE(to + 9, to, cap, bb, MFLAG_CAP));
                add_move(list, MOVE(to + 9, to, cap, br, MFLAG_CAP));
                add_move(list, MOVE(to + 9, to, cap, bq, MFLAG_CAP));
            } else
                add_move(list, MOVE(to + 9, to, cap, EMPTY, MFLAG_CAP));
        }
        U64 right_caps = (pawns >> 7) & enemy_occ & notAFile;
        while (right_caps) {
            int to = pop_lsb(&right_caps);
            int cap = get_piece_at(pos, to);
            if (to <= h1) {
                add_move(list, MOVE(to + 7, to, cap, bn, MFLAG_CAP));
                add_move(list, MOVE(to + 7, to, cap, bb, MFLAG_CAP));
                add_move(list, MOVE(to + 7, to, cap, br, MFLAG_CAP));
                add_move(list, MOVE(to + 7, to, cap, bq, MFLAG_CAP));
            } else
                add_move(list, MOVE(to + 7, to, cap, EMPTY, MFLAG_CAP));
        }
        if (pos->enpassant != NO_SQ) {
            if ((pawns >> 9) & (1ULL << pos->enpassant) & notHFile)
                add_move(list,
                         MOVE(pos->enpassant + 9, pos->enpassant, wp, EMPTY, MFLAG_EP | MFLAG_CAP));
            if ((pawns >> 7) & (1ULL << pos->enpassant) & notAFile)
                add_move(list,
                         MOVE(pos->enpassant + 7, pos->enpassant, wp, EMPTY, MFLAG_EP | MFLAG_CAP));
        }
    }

    U64 knights = (side == white) ? pos->bitboards[wn] : pos->bitboards[bn];
    while (knights) {
        int from = pop_lsb(&knights);
        U64 attacks = arrKnightAttacks[from] & ~friendly_occ;
        while (attacks) {
            int to = pop_lsb(&attacks);
            int cap = get_piece_at(pos, to);
            uint32_t flags = (cap != EMPTY) ? MFLAG_CAP : 0;
            add_move(list, MOVE(from, to, cap, EMPTY, flags));
        }
    }

    U64 king = (side == white) ? pos->bitboards[wk] : pos->bitboards[bk];
    if (king) {
        int from = get_lsb(king);
        U64 attacks = arrKingAttacks[from] & ~friendly_occ;
        while (attacks) {
            int to = pop_lsb(&attacks);
            int cap = get_piece_at(pos, to);
            uint32_t flags = (cap != EMPTY) ? MFLAG_CAP : 0;
            add_move(list, MOVE(from, to, cap, EMPTY, flags));
        }
        if (side == white) {
            if (pos->castle & WKCA) {
                if (!test_bit(occ, f1) && !test_bit(occ, g1)) {
                    if (!is_square_attacked(pos, e1, black) && !is_square_attacked(pos, f1, black))
                        add_move(list, MOVE(e1, g1, EMPTY, EMPTY, MFLAG_CA));
                }
            }
            if (pos->castle & WQCA) {
                if (!test_bit(occ, d1) && !test_bit(occ, c1) && !test_bit(occ, b1)) {
                    if (!is_square_attacked(pos, e1, black) && !is_square_attacked(pos, d1, black))
                        add_move(list, MOVE(e1, c1, EMPTY, EMPTY, MFLAG_CA));
                }
            }
        } else {
            if (pos->castle & BKCA) {
                if (!test_bit(occ, f8) && !test_bit(occ, g8)) {
                    if (!is_square_attacked(pos, e8, white) && !is_square_attacked(pos, f8, white))
                        add_move(list, MOVE(e8, g8, EMPTY, EMPTY, MFLAG_CA));
                }
            }
            if (pos->castle & BQCA) {
                if (!test_bit(occ, d8) && !test_bit(occ, c8) && !test_bit(occ, b8)) {
                    if (!is_square_attacked(pos, e8, white) && !is_square_attacked(pos, d8, white))
                        add_move(list, MOVE(e8, c8, EMPTY, EMPTY, MFLAG_CA));
                }
            }
        }
    }

    U64 bishops = (side == white) ? pos->bitboards[wb] : pos->bitboards[bb];
    while (bishops) {
        int from = pop_lsb(&bishops);
        U64 attacks = get_bishop_attacks(from, occ) & ~friendly_occ;
        while (attacks) {
            int to = pop_lsb(&attacks);
            int cap = get_piece_at(pos, to);
            uint32_t flags = (cap != EMPTY) ? MFLAG_CAP : 0;
            add_move(list, MOVE(from, to, cap, EMPTY, flags));
        }
    }

    U64 rooks = (side == white) ? pos->bitboards[wr] : pos->bitboards[br];
    while (rooks) {
        int from = pop_lsb(&rooks);
        U64 attacks = get_rook_attacks(from, occ) & ~friendly_occ;
        while (attacks) {
            int to = pop_lsb(&attacks);
            int cap = get_piece_at(pos, to);
            uint32_t flags = (cap != EMPTY) ? MFLAG_CAP : 0;
            add_move(list, MOVE(from, to, cap, EMPTY, flags));
        }
    }

    U64 queens = (side == white) ? pos->bitboards[wq] : pos->bitboards[bq];
    while (queens) {
        int from = pop_lsb(&queens);
        U64 attacks = get_queen_attacks(from, occ) & ~friendly_occ;
        while (attacks) {
            int to = pop_lsb(&attacks);
            int cap = get_piece_at(pos, to);
            uint32_t flags = (cap != EMPTY) ? MFLAG_CAP : 0;
            add_move(list, MOVE(from, to, cap, EMPTY, flags));
        }
    }
}

/**
 * @brief Internal helper to print a move in coordinate notation.
 * @param move The packed 32-bit move.
 */
static void print_move_short(uint32_t move) {
    int from = GET_FROM(move);
    int to = GET_TO(move);
    printf("%c%d%c%d", 'a' + (from % 8), 1 + (from / 8), 'a' + (to % 8), 1 + (to / 8));
    if (GET_PROMOTED(move) != EMPTY) {
        char piece_char[] = "pnbrqk";
        int p = GET_PROMOTED(move);
        if (p >= bp)
            p -= 6;
        printf("%c", piece_char[p]);
    }
}

/**
 * @brief Runs perft and prints node counts for each top-level move.
 * @param pos Pointer to the Board structure.
 * @param depth The depth to search.
 */
void perft_divide(Board* pos, int depth) {
    MoveList list;
    generate_all_moves(pos, &list);
    long long total_nodes = 0;
    for (int i = 0; i < list.count; i++) {
        if (!make_move(pos, list.moves[i].move))
            continue;
        long long nodes = perft_test(pos, depth - 1);
        unmake_move(pos);
        total_nodes += nodes;
        print_move_short(list.moves[i].move);
        printf(": %lld\n", nodes);
    }
    printf("\nTotal nodes at depth %d: %lld\n", depth, total_nodes);
}

/**
 * @brief Performance test to count all leaf nodes at a certain depth.
 * @param pos Pointer to the Board structure.
 * @param depth The depth to search.
 * @return Total number of nodes reached.
 */
long long perft_test(Board* pos, int depth) {
    if (depth == 0)
        return 1ULL;
    MoveList list;
    generate_all_moves(pos, &list);
    long long nodes = 0;
    for (int i = 0; i < list.count; i++) {
        if (!make_move(pos, list.moves[i].move))
            continue;
        nodes += perft_test(pos, depth - 1);
        unmake_move(pos);
    }
    return nodes;
}
