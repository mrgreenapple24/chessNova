#include "search.h"
#include "tt.h"
#include <stdio.h>
#include <string.h>
#include "polyglot.h"

#include "tt.h"

/**
 * Standard MVV-LVA (Most Valuable Victim - Least Valuable Attacker) Table.
 * Values: [Victim Type % 6][Attacker Type % 6]
 * Higher scores prioritize capturing high-value pieces with low-value attackers.
 */
static const int MVV_LVA[6][6] = {
    {105, 104, 103, 102, 101, 100}, // Victim: Pawn
    {205, 204, 203, 202, 201, 200}, // Victim: Knight
    {305, 304, 303, 302, 301, 300}, // Victim: Bishop
    {405, 404, 403, 402, 401, 400}, // Victim: Rook
    {505, 504, 503, 502, 501, 500}, // Victim: Queen
    {605, 604, 603, 602, 601, 600}  // Victim: King
};
/**
 * Checks if the search should be stopped due to time or other conditions.
 */
static void check_up(SearchInfo* info) {
    if (info->timeset && get_time_ms() > (long long)info->stoptime) {
        info->stopped = 1;
    }
}

/**
 * Helper to check if the current side has non-pawn material.
 */
static int has_non_pawn_material(const Board* board) {
    if (board->side == white) {
        return board->bitboards[wn] || board->bitboards[wb] || board->bitboards[wr] ||
               board->bitboards[wq];
    } else {
        return board->bitboards[bn] || board->bitboards[bb] || board->bitboards[br] ||
               board->bitboards[bq];
    }
}

/**
 * Scores moves in the move list based on TT best move, MVV-LVA, Killer Moves, and History
 * Heuristic.
 */
static void score_moves(SearchInfo* info, MoveList* list, Board* board, uint32_t tt_move) {
    for (int i = 0; i < list->count; i++) {
        uint32_t move = list->moves[i].move;
        int piece = board->pieces[GET_FROM(move)];
        int to = GET_TO(move);

        // 1. Hash/TT Move (Top Priority)
        if (tt_move != 0 && move == tt_move) {
            list->moves[i].score = TT_MOVE_SCORE;
        }
        // 2. MVV-LVA Captures
        else if (move & MFLAG_CAP) {
            int captured = GET_CAPTURED(move);
            int victim_type = (captured == EMPTY) ? 0 : (captured % 6);
            int attacker_type = (piece == EMPTY) ? 0 : (piece % 6);

            list->moves[i].score = CAPTURE_SCORE_BASE + MVV_LVA[victim_type][attacker_type];
        }
        // 3. Killer Moves & Scaled History Heuristic (Quiet Moves)
        else {
            if (board->ply < MAX_PLY && info->killer_moves[0][board->ply] == move) {
                list->moves[i].score = KILLER_1_SCORE;
            } else if (board->ply < MAX_PLY && info->killer_moves[1][board->ply] == move) {
                list->moves[i].score = KILLER_2_SCORE;
            } else {
                list->moves[i].score = info->history_moves[piece][to];
            }
        }
    }
}

/**
 * Selection sort helper: Picks the best remaining move in the list and swaps it to current index.
 */
static void pick_next_move(int move_num, MoveList* list) {
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
static int quiescence(Board* board, SearchInfo* info, int alpha, int beta) {
    if ((info->nodes & 2047) == 0) {
        check_up(info);
    }
    info->nodes++;

    int stand_pat = evaluate(board);
    if (stand_pat >= beta)
        return beta;
    if (alpha < stand_pat)
        alpha = stand_pat;

    MoveList list;
    generate_all_moves(board, &list);
    score_moves(info, &list, board, 0);

    for (int i = 0; i < list.count; i++) {
        pick_next_move(i, &list);
        uint32_t move = list.moves[i].move;
        if (!(move & MFLAG_CAP))
            continue;

        if (!make_move(board, move))
            continue;

        int score = -quiescence(board, info, -beta, -alpha);
        unmake_move(board);

        if (info->stopped)
            return 0;

        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }
    return alpha;
}

/**
 * Alpha-Beta pruning search with PVS, NMP, LMR, and TT Integration.
 */
static int alpha_beta(Board* board, SearchInfo* info, int depth, int alpha, int beta) {
    if (board->ply >= MAX_PLY - 1) { // Guard against ply overflow
        return evaluate(board);
    }

    if (depth <= 0)
        return quiescence(board, info, alpha, beta);

    if ((info->nodes & 2047) == 0) {
        check_up(info);
    }
    info->nodes++;

    // TT Probe
    int tt_score = 0;
    uint32_t tt_move = 0;
    if (probe_tt(board->posKey, depth, alpha, beta, board->ply, &tt_score, &tt_move)) {
        return tt_score;
    }

    U64 king_bb = board->bitboards[(board->side == white) ? wk : bk];
    if (king_bb == 0)
        return -MATE_SCORE + board->ply;
    int king_sq = get_lsb(king_bb);
    bool in_check = is_square_attacked(board, king_sq, board->side ^ 1);

    int old_alpha = alpha;
    uint32_t best_move = 0;

    // Null Move Pruning
    if (depth >= 3 && !in_check && has_non_pawn_material(board)) {
        make_null_move(board);
        int score = -alpha_beta(board, info, depth - 1 - 2, -beta, -beta + 1);
        unmake_null_move(board);
        if (info->stopped)
            return 0;
        if (score >= beta)
            return beta;
    }

    MoveList list;
    generate_all_moves(board, &list);
    score_moves(info, &list, board, tt_move);

    int legal_moves = 0;

    for (int i = 0; i < list.count; i++) {
        pick_next_move(i, &list);
        uint32_t move = list.moves[i].move;

        if (!make_move(board, move))
            continue;
        legal_moves++;

        int score;

        // Principal Variation Search (PVS)
        if (legal_moves == 1) {
            // First move: search with full window [alpha, beta]
            score = -alpha_beta(board, info, depth - 1, -beta, -alpha);
        } else {
            // Late Move Reduction (LMR) check
            if (legal_moves > 4 && depth >= 3 && !in_check && !(move & MFLAG_CAP) &&
                GET_PROMOTED(move) == EMPTY) {

                U64 enemy_king_bb = board->bitboards[(board->side == white) ? wk : bk];
                if (enemy_king_bb) {
                    int enemy_king_sq = get_lsb(enemy_king_bb);
                    bool gives_check = is_square_attacked(board, enemy_king_sq, board->side ^ 1);
                    if (!gives_check) {
                        score = -alpha_beta(board, info, depth - 2, -alpha - 1, -alpha);
                    } else {
                        score = alpha + 1;
                    }
                } else {
                    score = alpha + 1;
                }
            } else {
                score = alpha + 1; // Force zero-window search below
            }

            // Zero-window search for non-PV moves
            if (score > alpha) {
                score = -alpha_beta(board, info, depth - 1, -alpha - 1, -alpha);
                // Re-search with full window if move raised alpha
                if (score > alpha && score < beta) {
                    score = -alpha_beta(board, info, depth - 1, -beta, -alpha);
                }
            }
        }

        unmake_move(board);

        if (info->stopped)
            return 0;

        // Beta Cutoff
        if (score >= beta) {
            if (!(move & MFLAG_CAP)) {
                if (board->ply < MAX_PLY) {
                    info->killer_moves[1][board->ply] = info->killer_moves[0][board->ply];
                    info->killer_moves[0][board->ply] = move;
                }

                int piece = board->pieces[GET_FROM(move)];
                int to = GET_TO(move);
                // Scaled history heuristic for quiet move ordering
                info->history_moves[piece][to] += depth * depth * 100;
            }

            store_tt(board->posKey, move, beta, depth, TT_BETA, board->ply);
            return beta;
        }

        if (score > alpha) {
            alpha = score;
            best_move = move;
        }
    }

    if (legal_moves == 0) {
        if (in_check) {
            return -MATE_SCORE + board->ply;
        }
        return 0; // Stalemate
    }

    uint8_t flag = (alpha > old_alpha) ? TT_EXACT : TT_ALPHA;
    store_tt(board->posKey, best_move, alpha, depth, flag, board->ply);

    return alpha;
}

uint32_t search_best_move(Board* board, SearchInfo* info) {
    info->stopped = 0;
    info->nodes = 0;

    uint32_t book_move = GetBookMove(board);
    if (book_move != 0) {
        return book_move;
    }

    uint32_t best_move = 0;
    int best_score = -INFINITY;
    int current_depth = 1;

    board->ply = 0; // Explicitly reset search ply to 0 at root

    memset(info->killer_moves, 0, sizeof(info->killer_moves));
    memset(info->history_moves, 0, sizeof(info->history_moves));

    increment_tt_age();

    for (current_depth = 1; current_depth <= info->depth; current_depth++) {
        uint32_t root_tt_move = 0;
        int dummy_score = 0;

        // Probe TT for root move found in shallower iterations
        probe_tt(board->posKey, current_depth, -INFINITY, INFINITY, board->ply, &dummy_score,
                 &root_tt_move);
        if (root_tt_move == 0) {
            root_tt_move = best_move;
        }

        MoveList list;
        generate_all_moves(board, &list);
        score_moves(info, &list, board, root_tt_move);

        uint32_t depth_best_move = 0;
        int depth_best_score = -INFINITY;
        int alpha = -INFINITY;
        int beta = INFINITY;
        int legal_moves = 0;

        for (int i = 0; i < list.count; i++) {
            pick_next_move(i, &list);
            uint32_t move = list.moves[i].move;

            if (!make_move(board, move))
                continue;

            legal_moves++;
            int score;

            // Root PVS window: search first move with full window, subsequent moves with
            // null-window
            if (legal_moves == 1) {
                score = -alpha_beta(board, info, current_depth - 1, -beta, -alpha);
            } else {
                score = -alpha_beta(board, info, current_depth - 1, -alpha - 1, -alpha);
                if (score > alpha && score < beta) {
                    // Full re-search if move is superior
                    score = -alpha_beta(board, info, current_depth - 1, -beta, -alpha);
                }
            }

            unmake_move(board);

            if (info->stopped)
                break;

            if (score > depth_best_score) {
                depth_best_score = score;
                depth_best_move = move;
                if (score > alpha) {
                    alpha = score;
                }
            }
        }

        if (info->stopped)
            break;

        best_move = depth_best_move;
        best_score = depth_best_score;

        if (best_move != 0) {
            store_tt(board->posKey, best_move, best_score, current_depth, TT_EXACT, 0);
        }

        printf("info score cp %d depth %d nodes %llu time %llu\n", best_score, current_depth,
               (unsigned long long)info->nodes,
               (unsigned long long)get_time_ms() - (unsigned long long)info->starttime);

        if (best_score > MATE_SCORE - 1000 || best_score < -MATE_SCORE + 1000)
            break;
    }

    return best_move;
}