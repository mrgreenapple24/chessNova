#include "search.h"
#include <stdio.h>
#include <string.h>

/**
 * Checks if the search should be stopped due to time or other conditions.
 */
static void check_up(SearchInfo *info) {
    if (info->timeset && get_time_ms() > (long long)info->stoptime) {
        info->stopped = 1;
    }
}

/**
 * Helper to check if the current side has non-pawn material.
 */
static int has_non_pawn_material(const Board *board) {
    if (board->side == white) {
        return board->bitboards[wn] || board->bitboards[wb] ||
               board->bitboards[wr] || board->bitboards[wq];
    } else {
        return board->bitboards[bn] || board->bitboards[bb] ||
               board->bitboards[br] || board->bitboards[bq];
    }
}

/**
 * Scores moves in the move list based on MVV-LVA and other heuristics.
 */
static void score_moves(SearchInfo *info, MoveList *list, Board *board) {
    for (int i = 0; i < list->count; i++) {
        uint32_t move = list->moves[i].move;
        int piece = board->pieces[GET_FROM(move)];
        int to = GET_TO(move);

        if (move & MFLAG_CAP) {
            int captured = GET_CAPTURED(move);
            // piece % 6 gives type: 0=P, 1=N, 2=B, 3=R, 4=Q, 5=K
            int victim_type = (captured == EMPTY) ? 0 : (captured % 6);
            int attacker_type = piece % 6;
            list->moves[i].score = 1000000 + (victim_type * 10) + (6 - attacker_type);
        } else {
            if (info->killer_moves[0][board->ply] == move) {
                list->moves[i].score = 900000;
            } else if (info->killer_moves[1][board->ply] == move) {
                list->moves[i].score = 800000;
            } else {
                list->moves[i].score = info->history_moves[piece][to];
            }
        }
    }
}

/**
 * Picks the next best move from the list and swaps it to the front.
 */
static void pick_next_move(int move_num, MoveList *list) {
    int best_score = -1;
    int best_idx = move_num;

    for (int i = move_num; i < list->count; i++) {
        if (list->moves[i].score > best_score) {
            best_score = list->moves[i].score;
            best_idx = i;
        }
    }

    Move temp = list->moves[move_num];
    list->moves[move_num] = list->moves[best_idx];
    list->moves[best_idx] = temp;
}

/**
 * Quiescence search to handle the horizon effect by searching captures until the position is quiet.
 */
static int quiescence(Board *board, SearchInfo *info, int alpha, int beta) {
    if ((info->nodes & 2047) == 0) {
        check_up(info);
    }
    info->nodes++;

    int stand_pat = evaluate(board);
    if (stand_pat >= beta) return beta;
    if (alpha < stand_pat) alpha = stand_pat;

    MoveList list;
    generate_all_moves(board, &list);
    score_moves(info, &list, board);

    for (int i = 0; i < list.count; i++) {
        pick_next_move(i, &list);
        uint32_t move = list.moves[i].move;
        if (!(move & MFLAG_CAP)) continue;

        if (!make_move(board, move)) continue;
        int score = -quiescence(board, info, -beta, -alpha);
        unmake_move(board);

        if (info->stopped) return 0;

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

/**
 * Alpha-Beta pruning search with NMP and LMR.
 */
static int alpha_beta(Board *board, SearchInfo *info, int depth, int alpha, int beta) {
    if (depth == 0) return quiescence(board, info, alpha, beta);

    if ((info->nodes & 2047) == 0) {
        check_up(info);
    }
    info->nodes++;

    U64 king_bb = board->bitboards[(board->side == white) ? wk : bk];
    if (king_bb == 0) return -MATE_SCORE + board->ply;
    int king_sq = get_lsb(king_bb);
    bool in_check = is_square_attacked(board, king_sq, board->side ^ 1);

    // --- Null Move Pruning ---
    if (depth >= 3 && !in_check && has_non_pawn_material(board)) {
        make_null_move(board);
        int score = -alpha_beta(board, info, depth - 1 - 2, -beta, -beta + 1);
        unmake_null_move(board);
        if (info->stopped) return 0;
        if (score >= beta) return beta;
    }

    MoveList list;
    generate_all_moves(board, &list);
    score_moves(info, &list, board);

    int legal_moves = 0;

    for (int i = 0; i < list.count; i++) {
        pick_next_move(i, &list);
        uint32_t move = list.moves[i].move;

        if (!make_move(board, move)) continue;
        legal_moves++;

        int score;
        // --- Late Move Reductions ---
        if (legal_moves > 4 && depth >= 3 && !in_check &&
            !(move & MFLAG_CAP) && GET_PROMOTED(move) == EMPTY) {

            // Check if the move gives check
            U64 enemy_king_bb = board->bitboards[(board->side == white) ? wk : bk];
            if (enemy_king_bb) {
                int enemy_king_sq = get_lsb(enemy_king_bb);
                bool gives_check = is_square_attacked(board, enemy_king_sq, board->side ^ 1);
                if (!gives_check) {
                    score = -alpha_beta(board, info, depth - 2, -alpha - 1, -alpha);
                } else {
                    score = alpha + 1; // Force re-search
                }
            } else {
                score = alpha + 1; // Force re-search
            }

            if (score > alpha) {
                score = -alpha_beta(board, info, depth - 1, -beta, -alpha);
            }
        } else {
            score = -alpha_beta(board, info, depth - 1, -beta, -alpha);
        }

        unmake_move(board);

        if (info->stopped) return 0;

        if (score >= beta) {
            if (!(move & MFLAG_CAP)) {
                info->killer_moves[1][board->ply] = info->killer_moves[0][board->ply];
                info->killer_moves[0][board->ply] = move;

                int piece = board->pieces[GET_FROM(move)];
                int to = GET_TO(move);
                info->history_moves[piece][to] += depth * depth;
            }
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    if (legal_moves == 0) {
        if (in_check) {
            return -MATE_SCORE + board->ply;
        }
        return 0; // Stalemate
    }

    return alpha;
}

uint32_t search_best_move(Board *board, SearchInfo *info) {
    uint32_t best_move = 0;
    int best_score = -INFINITY;
    int current_depth = 1;

    memset(info->killer_moves, 0, sizeof(info->killer_moves));
    memset(info->history_moves, 0, sizeof(info->history_moves));
    info->stopped = 0;
    info->nodes = 0;

    for (current_depth = 1; current_depth <= info->depth; current_depth++) {
        MoveList list;
        generate_all_moves(board, &list);
        score_moves(info, &list, board);

        uint32_t depth_best_move = 0;
        int depth_best_score = -INFINITY;

        for (int i = 0; i < list.count; i++) {
            pick_next_move(i, &list);
            uint32_t move = list.moves[i].move;

            if (!make_move(board, move)) continue;
            int score = -alpha_beta(board, info, current_depth - 1, -INFINITY, INFINITY);
            unmake_move(board);

            if (info->stopped) break;

            if (score > depth_best_score) {
                depth_best_score = score;
                depth_best_move = move;
            }
        }

        if (info->stopped) break;

        best_move = depth_best_move;
        best_score = depth_best_score;

        printf("info score cp %d depth %d nodes %llu time %llu\n",
               best_score, current_depth, (unsigned long long)info->nodes, (unsigned long long)get_time_ms() - (unsigned long long)info->starttime);

        if (best_score > MATE_SCORE - 100 || best_score < -MATE_SCORE + 100) break;
    }

    return best_move;
}
