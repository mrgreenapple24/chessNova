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

    if (board->ply > info->seldepth) {
        info->seldepth = board->ply;
    }
    if (board->ply < 64) {
        info->pv_length[board->ply] = board->ply;
    }

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

    if (board->ply > info->seldepth) {
        info->seldepth = board->ply;
    }
    if (board->ply < 64) {
        info->pv_length[board->ply] = board->ply;
    }

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

            if (board->ply < 64) {
                info->pv_table[board->ply][board->ply] = move;
                for (int j = board->ply + 1; j < info->pv_length[board->ply + 1] && j < 64; j++) {
                    info->pv_table[board->ply][j] = info->pv_table[board->ply + 1][j];
                }
                info->pv_length[board->ply] = info->pv_length[board->ply + 1];
            }
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

/**
 * Extracts the Principal Variation line by following best moves stored in the TT.
 * Validates each move's legality to prevent invalid lines or infinite cycles.
 */
static int get_pv_line(Board* board, uint32_t* pv, int max_depth) {
    int count = 0;
    U64 visited[64];

    while (count < max_depth && count < 64) {
        visited[count] = board->posKey;

        uint32_t move = probe_tt_move(board->posKey);
        if (move == 0) {
            break;
        }

        // Verify that move is strictly legal in the current board position
        MoveList list;
        generate_all_moves(board, &list);
        bool legal = false;
        for (int i = 0; i < list.count; i++) {
            uint32_t m = list.moves[i].move;
            if (GET_FROM(m) == GET_FROM(move) && GET_TO(m) == GET_TO(move) &&
                GET_PROMOTED(m) == GET_PROMOTED(move)) {
                if (make_move(board, m)) {
                    legal = true;
                    move = m;
                }
                break;
            }
        }

        if (!legal) {
            break;
        }

        pv[count++] = move;

        // Repetition / cycle check
        bool cycle = false;
        for (int i = 0; i < count; i++) {
            if (visited[i] == board->posKey) {
                cycle = true;
                break;
            }
        }
        if (cycle) {
            break;
        }
    }

    // Unmake moves in reverse order
    for (int i = count - 1; i >= 0; i--) {
        unmake_move(board);
    }

    return count;
}

uint32_t search_best_move(Board* board, SearchInfo* info) {
    info->stopped = 0;
    info->nodes = 0;
    info->seldepth = 0;

    if (PieceKeys[0][0] == 0) {
        init_zobrist();
    }
    if (board->posKey == 0) {
        board->posKey = generate_pos_key(board);
    }

    if (TT.pTable == NULL) {
        init_tt(64);
    }

    uint32_t book_move = GetBookMove(board);
    if (book_move != 0) {
        printf("info depth 1 seldepth 1 score cp 0 time 0 nodes 0 nps 0 hashfull %d pv %s\n",
               get_hashfull(), move_to_string(book_move));
        fflush(stdout);
        return book_move;
    }

    uint32_t best_move = 0;
    uint32_t depth_best_move = 0;
    uint32_t root_tt_move = 0;
    int best_score = -INFINITY;
    int current_depth = 1;

    board->ply = 0; // Explicitly reset search ply to 0 at root

    memset(info->killer_moves, 0, sizeof(info->killer_moves));
    memset(info->history_moves, 0, sizeof(info->history_moves));
    memset(info->pv_table, 0, sizeof(info->pv_table));
    memset(info->pv_length, 0, sizeof(info->pv_length));

    increment_tt_age();

    for (current_depth = 1; current_depth <= info->depth; current_depth++) {
        info->seldepth = current_depth;
        root_tt_move = 0;
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

        depth_best_move = 0;
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

                info->pv_table[0][0] = move;
                for (int j = 1; j < info->pv_length[1] && j < 64; j++) {
                    info->pv_table[0][j] = info->pv_table[1][j];
                }
                info->pv_length[0] = info->pv_length[1];
            }
        }

        if (info->stopped)
            break;

        best_move = depth_best_move;
        best_score = depth_best_score;

        if (best_move != 0) {
            store_tt(board->posKey, best_move, best_score, current_depth, TT_EXACT, 0);
        }

        uint32_t pv[64];
        int pv_count = get_pv_line(board, pv, current_depth);
        if (pv_count == 0 && info->pv_length[0] > 0) {
            for (int j = 0; j < info->pv_length[0] && j < 64; j++) {
                pv[j] = info->pv_table[0][j];
            }
            pv_count = info->pv_length[0];
        }
        if (pv_count == 0 && best_move != 0) {
            pv[0] = best_move;
            pv_count = 1;
        }

        long long elapsed = (long long)get_time_ms() - (long long)info->starttime;
        if (elapsed < 0)
            elapsed = 0;
        unsigned long long nps =
            (elapsed > 0) ? ((unsigned long long)info->nodes * 1000 / (unsigned long long)elapsed)
                          : ((unsigned long long)info->nodes * 1000);

        printf("info depth %d seldepth %d score ", current_depth, info->seldepth);
        if (best_score > MATE_SCORE - 1000) {
            int mate_in_moves = (MATE_SCORE - best_score + 1) / 2;
            printf("mate %d ", mate_in_moves);
        } else if (best_score < -MATE_SCORE + 1000) {
            int mate_in_moves = -(MATE_SCORE + best_score + 1) / 2;
            printf("mate %d ", mate_in_moves);
        } else {
            printf("cp %d ", best_score);
        }

        printf("nodes %llu nps %llu time %llu hashfull %d pv",
               (unsigned long long)info->nodes,
               nps,
               (unsigned long long)elapsed,
               get_hashfull());

        for (int i = 0; i < pv_count; i++) {
            printf(" %s", move_to_string(pv[i]));
        }
        printf("\n");
        fflush(stdout);

        if (best_score > MATE_SCORE - 1000 || best_score < -MATE_SCORE + 1000)
            break;
    }

    if (best_move == 0) {
        if (depth_best_move != 0) {
            best_move = depth_best_move;
        } else if (root_tt_move != 0) {
            best_move = root_tt_move;
        } else {
            MoveList list;
            generate_all_moves(board, &list);
            for (int i = 0; i < list.count; i++) {
                if (make_move(board, list.moves[i].move)) {
                    unmake_move(board);
                    best_move = list.moves[i].move;
                    break;
                }
            }
        }
    }

    return best_move;
}