#include "evaluate.h"
#include "bitboard.h"
#include "kings.h"
#include "pawns.h"
#include "search.h"
#include "types.h"

/* Piece-Square Tables (White's perspective) */
/* PeSTO's tables or similar style are common for tapered eval */

const int pawn_pst[2][64] = {
    {// Middlegame
     0,   0,  0,   0,   0,   0,  0,  0,   98,  134, 61, 95,  68, 126, 34, -11,
     -6,  7,  26,  31,  65,  56, 25, -20, -14, 13,  6,  21,  23, 12,  17, -23,
     -27, -2, -5,  12,  17,  6,  10, -25, -26, -4,  -4, -10, 3,  3,   33, -12,
     -35, -1, -20, -23, -15, 24, 38, -22, 0,   0,   0,  0,   0,  0,   0,  0},
    {// Endgame
     0,   0,   0,   0,   0,   0,   0,   0,   178, 173, 158, 160, 148, 147, 133, 165,
     148, 134, 110, 118, 116, 109, 121, 125, 111, 105, 88,  83,  88,  87,  88,  102,
     61,  54,  47,  50,  46,  50,  52,  61,  46,  39,  31,  33,  31,  32,  36,  46,
     37,  31,  17,  16,  15,  14,  17,  37,  0,   0,   0,   0,   0,   0,   0,   0}};

const int knight_pst[2][64] = {
    {// Middlegame
     -167, -89, -34, -49, -10, -53, -80, -143, -71,  -58, -26, -11, 4,   -13, -57, -67,
     -48,  -9,  14,  21,  32,  19,  -11, -44,  -31,  4,   22,  31,  33,  21,  -1,  -31,
     -22,  11,  23,  27,  31,  28,  11,  -26,  -20,  18,  28,  33,  30,  26,  14,  -15,
     -55,  -27, 13,  1,   0,   4,   -30, -56,  -121, -83, -26, -21, -13, -42, -83, -127},
    {// Endgame
     -58, -38, -13, -28, -31, -27, -63, -99, -25, -8,  -2,  4,   2,   -5,  -21, -37,
     -24, -20, 4,   4,   6,   10,  -17, -26, -17, 1,   9,   15,  17,  14,  10,  -15,
     -16, 5,   14,  16,  20,  16,  6,   -16, -15, 2,   12,  12,  11,  12,  5,   -15,
     -23, -20, 5,   2,   2,   2,   -11, -25, -60, -20, -10, -15, -15, -20, -20, -73}};

const int bishop_pst[2][64] = {
    {// Middlegame
     -29, 4,  -82, -37, -38, -73, 13, -21, -28, 8,  18,  27,  27,  24,  -10, -26,
     -19, 15, 19,  22,  24,  17,  7,  -20, -13, 5,  24,  26,  27,  22,  14,  -10,
     -12, 6,  25,  28,  30,  20,  10, -6,  -18, 17, 11,  21,  20,  18,  12,  -3,
     -15, -1, -5,  12,  2,   10,  5,  -15, -22, -9, -10, -11, -12, -13, -14, -26},
    {// Endgame
     -14, -21, -11, -8, -7,  -9, -17, -24, -8, -4, 7,  -1,  -3,  13,  2,  -4, -3, 2,  12,  7,  7, 2,
     7,   -3,  -3,  6,  8,   14, 12,  7,   5,  -3, -5, 0,   7,   15,  8,  12, 7,  -6, -1,  3,  5, 6,
     13,  8,   7,   -5, -14, -4, 11,  -2,  3,  2,  2,  -14, -23, -11, -2, -5, -7, -8, -10, -23}};

const int rook_pst[2][64] = {
    {// Middlegame
     32,  42,  32,  51, 63, 9,  31, 43,  27,  32,  58,  62,  80, 67, 26,  44,
     -5,  19,  26,  36, 17, 45, 61, 16,  -24, -11, 7,   26,  24, 35, -8,  -20,
     -36, -26, -12, -1, 9,  -7, -6, -23, -45, -25, -16, -17, 3,  0,  -5,  -33,
     -44, -16, -20, -9, -1, 11, -6, -71, -19, -13, 1,   17,  16, 7,  -37, -26},
    {// Endgame
     13,  10,  18,  15, 12, 12, 8,  5,   11,  13,  13,  11, -3, 3,  8,   3,
     7,   7,   7,   5,  4,  -3, -5, -3,  1,   0,   2,   4,  -3, -4, -3,  -9,
     -5,  0,   2,   2,  -1, 2,  -3, -9,  -10, -7,  -3,  -7, -2, -2, -8,  -14,
     -13, -12, -11, -1, -2, 2,  -2, -15, -18, -16, -11, -9, -3, -7, -11, -24}};

const int queen_pst[2][64] = {
    {// Middlegame
     -28, 0,   29, 12,  59, 44, 43, 45, -24, -39, -5,  1,   -16, 57,  28,  54,
     -13, -17, 7,  8,   29, 56, 47, 57, -27, -27, -16, -16, -1,  17,  -2,  1,
     -9,  -26, -9, -11, -2, -8, -2, -3, -14, 2,   -11, -2,  -5,  2,   14,  5,
     -35, -8,  11, 2,   8,  15, -3, 1,  -1,  -18, -9,  10,  -15, -25, -31, -50},
    {// Endgame
     -69, -57, -47, -26, -47, -59, -43, -54, -48, -32, -14, -10, -13, -20, -43, -48,
     -48, -28, -22, -2,  14,  -9,  -25, -42, -10, 20,  28,  37,  31,  32,  6,   -7,
     -9,  22,  33,  45,  44,  38,  22,  -9,  -25, -5,  13,  25,  13,  11,  -3,  -24,
     -31, -27, -18, -14, -17, -10, -20, -31, -47, -40, -34, -45, -34, -40, -41, -49}};

const int king_pst[2][64] = {
    {// Middlegame
     -65, 23,  16,  -15, -56, -34, 2,   13,  29,  -1,  -20, -7,  -8,  -4,  -38, -29,
     -9,  24,  2,   -16, -20, 6,   22,  -22, -17, -20, -12, -27, -30, -25, -14, -36,
     -49, -44, -27, -39, -46, -44, -33, -51, -14, -14, -20, -35, -32, -18, -11, -51,
     -30, -6,  -13, -24, -26, -16, -5,  -33, -31, -20, -25, -45, -55, -13, -12, -32},
    {// Endgame
     -74, -35, -18, -18, -11, -15, -38, -50, -12, 17,  14,  17,  17,  38,  23,  -10,
     10,  17,  23,  15,  20,  45,  44,  13,  -8,  22,  24,  27,  26,  33,  26,  3,
     -18, -4,  21,  24,  27,  23,  9,   -11, -19, -3,  11,  21,  23,  16,  7,   -9,
     -27, -11, 4,   13,  14,  4,   -5,  -17, -53, -34, -21, -11, -28, -14, -24, -43}};

/* Phase values for each piece */
const int knight_phase = 1;
const int bishop_phase = 1;
const int rook_phase = 2;
const int queen_phase = 4;
const int total_phase = 24; // (2*1 + 2*1 + 2*2 + 1*4) * 2

/* Masks for evaluation */
U64 file_masks[64];
U64 adj_file_masks[64];
U64 passed_masks[2][64];

void init_evaluation_masks() {
    for (int sq = 0; sq < 64; sq++) {
        int f = sq % 8;
        int r = sq / 8;

        file_masks[sq] = arrFiles[f];

        adj_file_masks[sq] = 0;
        if (f > 0)
            adj_file_masks[sq] |= arrFiles[f - 1];
        if (f < 7)
            adj_file_masks[sq] |= arrFiles[f + 1];

        passed_masks[white][sq] = 0;
        passed_masks[black][sq] = 0;

        for (int rank = r + 1; rank < 8; rank++) {
            passed_masks[white][sq] |= (1ULL << (rank * 8 + f));
            if (f > 0)
                passed_masks[white][sq] |= (1ULL << (rank * 8 + f - 1));
            if (f < 7)
                passed_masks[white][sq] |= (1ULL << (rank * 8 + f + 1));
        }

        for (int rank = r - 1; rank >= 0; rank--) {
            passed_masks[black][sq] |= (1ULL << (rank * 8 + f));
            if (f > 0)
                passed_masks[black][sq] |= (1ULL << (rank * 8 + f - 1));
            if (f < 7)
                passed_masks[black][sq] |= (1ULL << (rank * 8 + f + 1));
        }
    }
}

int evaluate(const Board* board) {
    int mg_score = 0;
    int eg_score = 0;
    int game_phase = 0;
    U64 bitboard;
    int sq;

    int white_king_sq = (board->bitboards[wk]) ? get_lsb(board->bitboards[wk]) : NO_SQ;
    int black_king_sq = (board->bitboards[bk]) ? get_lsb(board->bitboards[bk]) : NO_SQ;

    if (white_king_sq == NO_SQ)
        return -MATE_SCORE;
    if (black_king_sq == NO_SQ)
        return MATE_SCORE;

    U64 white_king_zone = arrKingAttacks[white_king_sq];
    U64 black_king_zone = arrKingAttacks[black_king_sq];

    // --- White Pieces ---
    bitboard = board->bitboards[wp];
    /*
    while (bitboard) {
        sq = pop_lsb(&bitboard);
        int r = sq/8;
        int f = sq%8;
        r = 7-r;
        sq = 8*r + f;
        mg_score += MG_PAWN_VALUE + pawn_pst[0][sq];
        eg_score += EG_PAWN_VALUE + pawn_pst[1][sq];

        // Double pawn penalty
        if (count_bits(board->bitboards[wp] & file_masks[sq]) > 1) {
            mg_score += MG_DOUBLED_PAWN;
            eg_score += EG_DOUBLED_PAWN;
        }

        // Isolated pawn penalty
        if (!(board->bitboards[wp] & adj_file_masks[sq])) {
            mg_score += MG_ISOLATED_PAWN;
            eg_score += EG_ISOLATED_PAWN;
        }

        // Passed pawn bonus
        if (!(board->bitboards[bp] & passed_masks[white][sq])) {
            eg_score += EG_PASSED_PAWN;
        }
    }

    */

    while (bitboard) {
        sq = pop_lsb(&bitboard);
        int r = sq / 8;
        int f = sq % 8;
        r = 7 - r;
        sq = 8 * r + f;
        mg_score += MG_PAWN_VALUE + pawn_pst[0][sq]; // Assigning values to pawns and suming it up
        eg_score += EG_PAWN_VALUE + pawn_pst[1][sq];
    }
    bitboard = board->bitboards[bp];
    while (bitboard) {
        sq = pop_lsb(&bitboard);
        mg_score -= MG_PAWN_VALUE + pawn_pst[0][sq]; // Assigning values to pawns and suming it up
        eg_score -= EG_PAWN_VALUE + pawn_pst[1][sq];
    }
    for (int i = 0; i < count_bits(wPawnsInfrontOwn(board->bitboards[wp])); i++) {
        mg_score += MG_DOUBLED_PAWN; // Penalty for double pawns
        eg_score += EG_DOUBLED_PAWN;
    }
    for (int i = 0; i < count_bits(isolanis(board->bitboards[wp])); i++) {
        mg_score += MG_ISOLATED_PAWN; // Penalty for isolated pawns
        eg_score += EG_ISOLATED_PAWN;
    }
    for (int i = 0; i < count_bits(wPassedPawns(board->bitboards[wp], board->bitboards[bp])); i++) {
        eg_score += EG_PASSED_PAWN; // Higher score for passed pawns
    }
    for (int i = 0; i < count_bits(bPawnsInfrontOwn(board->bitboards[bp])); i++) {
        mg_score -= MG_DOUBLED_PAWN; // Higher score for black double pawns
        eg_score -= EG_DOUBLED_PAWN;
    }
    for (int i = 0; i < count_bits(isolanis(board->bitboards[bp])); i++) {
        mg_score -= MG_ISOLATED_PAWN; // Higher score for black isolated pawns
        eg_score -= EG_ISOLATED_PAWN;
    }
    for (int i = 0; i < count_bits(bPassedPawns(board->bitboards[wp], board->bitboards[bp])); i++) {
        eg_score -= EG_PASSED_PAWN; // Penalty for black passed pawns
    }

    bitboard = board->bitboards[wn];
    while (bitboard) {
        sq = pop_lsb(&bitboard);
        mg_score += MG_KNIGHT_VALUE + knight_pst[0][sq];
        eg_score += EG_KNIGHT_VALUE + knight_pst[1][sq];
        game_phase += knight_phase;
    }
    bitboard = board->bitboards[wb];
    if (count_bits(bitboard) >= 2) {
        mg_score += MG_BISHOP_PAIR;
        eg_score += EG_BISHOP_PAIR;
    }
    while (bitboard) {
        sq = pop_lsb(&bitboard);
        mg_score += MG_BISHOP_VALUE + bishop_pst[0][sq];
        eg_score += EG_BISHOP_VALUE + bishop_pst[1][sq];
        game_phase += bishop_phase;
    }
    bitboard = board->bitboards[wr];
    while (bitboard) {
        sq = pop_lsb(&bitboard);
        mg_score += MG_ROOK_VALUE + rook_pst[0][sq];
        eg_score += EG_ROOK_VALUE + rook_pst[1][sq];
        game_phase += rook_phase;

        // White rook on 7th rank bonus
        if ((1ULL << sq) & rank7) {
            mg_score += MG_ROOK_ON_7TH;
            eg_score += EG_ROOK_ON_7TH;
            if (black_king_sq != NO_SQ && ((1ULL << black_king_sq) & rank8)) {
                mg_score += MG_ROOK_ON_7TH_TRAPPED_KING;
                eg_score += EG_ROOK_ON_7TH_TRAPPED_KING;
            }
        }
    }

    bitboard = board->bitboards[wq];
    while (bitboard) {
        sq = pop_lsb(&bitboard);
        mg_score += MG_QUEEN_VALUE + queen_pst[0][sq];
        eg_score += EG_QUEEN_VALUE + queen_pst[1][sq];
        game_phase += queen_phase;
    }
    bitboard = board->bitboards[wk];
    while (bitboard) {
        sq = pop_lsb(&bitboard);
        mg_score += MG_KING_VALUE + king_pst[0][sq];
        eg_score += EG_KING_VALUE + king_pst[1][sq];
    }

    // --- Black Pieces ---
    /*
    bitboard = board->bitboards[bp];
    while (bitboard) {
        sq = pop_lsb(&bitboard);
        int r = sq/8;
        int f = sq%8;
        r = 7-r;
        sq = 8*r + f;
        mg_score -= (MG_PAWN_VALUE + pawn_pst[0][FLIP(sq)]);
        eg_score -= (EG_PAWN_VALUE + pawn_pst[1][FLIP(sq)]);

        // Double pawn penalty
        if (count_bits(board->bitboards[bp] & file_masks[sq]) > 1) {
            mg_score -= MG_DOUBLED_PAWN;
            eg_score -= EG_DOUBLED_PAWN;
        }

        // Isolated pawn penalty
        if (!(board->bitboards[bp] & adj_file_masks[sq])) {
            mg_score -= MG_ISOLATED_PAWN;
            eg_score -= EG_ISOLATED_PAWN;
        }

        // Passed pawn bonus
        if (!(board->bitboards[wp] & passed_masks[black][sq])) {
            eg_score -= EG_PASSED_PAWN;
        }
    }
        */
    bitboard = board->bitboards[bn];
    while (bitboard) {
        sq = pop_lsb(&bitboard);
        mg_score -= (MG_KNIGHT_VALUE + knight_pst[0][FLIP(sq)]);
        eg_score -= (EG_KNIGHT_VALUE + knight_pst[1][FLIP(sq)]);
        game_phase += knight_phase;
    }
    bitboard = board->bitboards[bb];
    if (count_bits(bitboard) >= 2) {
        mg_score -= MG_BISHOP_PAIR;
        eg_score -= EG_BISHOP_PAIR;
    }
    while (bitboard) {
        sq = pop_lsb(&bitboard);
        mg_score -= (MG_BISHOP_VALUE + bishop_pst[0][FLIP(sq)]);
        eg_score -= (EG_BISHOP_VALUE + bishop_pst[1][FLIP(sq)]);
        game_phase += bishop_phase;
    }
    bitboard = board->bitboards[br];
    while (bitboard) {
        sq = pop_lsb(&bitboard);
        mg_score -= (MG_ROOK_VALUE + rook_pst[0][FLIP(sq)]);
        eg_score -= (EG_ROOK_VALUE + rook_pst[1][FLIP(sq)]);
        game_phase += rook_phase;

        // Black rook on 2nd rank bonus
        if ((1ULL << sq) & rank2) {
            mg_score -= MG_ROOK_ON_7TH;
            eg_score -= EG_ROOK_ON_7TH;
            if (white_king_sq != NO_SQ && ((1ULL << white_king_sq) & rank1)) {
                mg_score -= MG_ROOK_ON_7TH_TRAPPED_KING;
                eg_score -= EG_ROOK_ON_7TH_TRAPPED_KING;
            }
        }
    }
    bitboard = board->bitboards[bq];
    while (bitboard) {
        sq = pop_lsb(&bitboard);
        mg_score -= (MG_QUEEN_VALUE + queen_pst[0][FLIP(sq)]);
        eg_score -= (EG_QUEEN_VALUE + queen_pst[1][FLIP(sq)]);
        game_phase += queen_phase;
    }
    bitboard = board->bitboards[bk];
    while (bitboard) {
        sq = pop_lsb(&bitboard);
        mg_score -= (MG_KING_VALUE + king_pst[0][FLIP(sq)]);
        eg_score -= (EG_KING_VALUE + king_pst[1][FLIP(sq)]);
    }

    // --- King Safety Calculation ---
    if (board->bitboards[wk] & ((1ULL << g1) | (1ULL << c1)))
        mg_score += 300;
    if (board->bitboards[bk] & ((1ULL << g8) | (1ULL << c8)))
        mg_score -= 300;
    // Penalize enemy pieces in the king zone
    U64 white_attackers = (board->bitboards[bn] | board->bitboards[bb] | board->bitboards[br] |
                           board->bitboards[bq] | board->bitboards[bp]) &
                          white_king_zone;
    U64 black_attackers = (board->bitboards[wn] | board->bitboards[wb] | board->bitboards[wr] |
                           board->bitboards[wq] | board->bitboards[wp]) &
                          black_king_zone;

    mg_score += count_bits(white_attackers) * MG_KING_ATTACK_PENALTY;
    eg_score += count_bits(white_attackers) * EG_KING_ATTACK_PENALTY;

    mg_score -= count_bits(black_attackers) * MG_KING_ATTACK_PENALTY;
    eg_score -= count_bits(black_attackers) * EG_KING_ATTACK_PENALTY;

    // --- Pawn Storm Calculation ---
    U64 black_pawn_attacks = bPawnAnyAttacks(board->bitboards[bp]);
    U64 white_pawn_attacks = wPawnAnyAttacks(board->bitboards[wp]);

    // 1. Black pawn storm on White King (attacks on squares in front of White King)
    int black_pawn_storm_on_white_king_count = 0;
    if (white_king_sq != NO_SQ) {
        int r = white_king_sq / 8;
        int f = white_king_sq % 8;
        if (r < 7) { // Rank must be 0 to 6
            int front_rank = r + 1;
            U64 white_king_front_mask = 0ULL;
            if (f == 0) { // a-file: 2 front squares (a & b files)
                white_king_front_mask |= (1ULL << (front_rank * 8 + 0));
                white_king_front_mask |= (1ULL << (front_rank * 8 + 1));
            } else if (f == 7) { // h-file: 2 front squares (g & h files)
                white_king_front_mask |= (1ULL << (front_rank * 8 + 6));
                white_king_front_mask |= (1ULL << (front_rank * 8 + 7));
            } else { // b-g files: 3 front squares (file-1, file, file+1)
                white_king_front_mask |= (1ULL << (front_rank * 8 + f - 1));
                white_king_front_mask |= (1ULL << (front_rank * 8 + f));
                white_king_front_mask |= (1ULL << (front_rank * 8 + f + 1));
            }
            black_pawn_storm_on_white_king_count = count_bits(white_king_front_mask & black_pawn_attacks);
        }
    }

    // 2. White pawn storm on Black King (attacks on squares in front of Black King)
    int white_pawn_storm_on_black_king_count = 0;
    if (black_king_sq != NO_SQ) {
        int r = black_king_sq / 8;
        int f = black_king_sq % 8;
        if (r > 0) { // Rank must be 1 to 7
            int front_rank = r - 1;
            U64 black_king_front_mask = 0ULL;
            if (f == 0) { // a-file: 2 front squares (a & b files)
                black_king_front_mask |= (1ULL << (front_rank * 8 + 0));
                black_king_front_mask |= (1ULL << (front_rank * 8 + 1));
            } else if (f == 7) { // h-file: 2 front squares (g & h files)
                black_king_front_mask |= (1ULL << (front_rank * 8 + 6));
                black_king_front_mask |= (1ULL << (front_rank * 8 + 7));
            } else { // b-g files: 3 front squares (file-1, file, file+1)
                black_king_front_mask |= (1ULL << (front_rank * 8 + f - 1));
                black_king_front_mask |= (1ULL << (front_rank * 8 + f));
                black_king_front_mask |= (1ULL << (front_rank * 8 + f + 1));
            }
            white_pawn_storm_on_black_king_count = count_bits(black_king_front_mask & white_pawn_attacks);
        }
    }

    // Apply storm attack penalties
    mg_score += black_pawn_storm_on_white_king_count * MG_PAWN_STORM_ATTACK_PENALTY;
    eg_score += black_pawn_storm_on_white_king_count * EG_PAWN_STORM_ATTACK_PENALTY;

    mg_score -= white_pawn_storm_on_black_king_count * MG_PAWN_STORM_ATTACK_PENALTY;
    eg_score -= white_pawn_storm_on_black_king_count * EG_PAWN_STORM_ATTACK_PENALTY;

    // Tapered evaluation interpolation
    int mg_phase = game_phase;
    if (mg_phase > total_phase)
        mg_phase = total_phase;
    int eg_phase = total_phase - mg_phase;

    int final_score = (mg_score * mg_phase + eg_score * eg_phase) / total_phase;

    return (board->side == white) ? final_score : -final_score;
}
