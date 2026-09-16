/**
 * @file test_pawnstorm.c
 * @brief Test suite for pawn storm evaluation logic.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "defs.h"
#include "evaluate.h"

int main() {
    init_magics();
    init_evaluation_masks();

    // 1. Test Black pawn storm on White King (White King on g1 - middle file g)
    // Position A: White King on g1, no black pawn attacking front squares f2, g2, h2
    // Position B: White King on g1, Black pawn on f3 attacking g2 & e2 (attaching 1 front square: g2)
    Board board_no_storm = {0}, board_storm = {0};
    parse_fen("7k/8/8/8/8/8/4P3/6K1 w - - 0 1", &board_no_storm);
    parse_fen("7k/8/8/8/8/5p2/4P3/6K1 w - - 0 1", &board_storm);

    int eval_no_storm = evaluate(&board_no_storm);
    int eval_storm = evaluate(&board_storm);

    printf("Eval White King g1 (No Black Pawn Storm): %d\n", eval_no_storm);
    printf("Eval White King g1 (Black Pawn Storm on g2): %d\n", eval_storm);

    // Black pawn storm attacking White King's front square should drop evaluation for White
    assert(eval_storm < eval_no_storm);

    // 2. Test Edge file (a-file) 2-square front logic for White King on a1
    // Front squares for King on a1 are a2 and b2.
    // Black pawn on c3 attacks b2 (1 front square).
    Board board_edge_no_storm, board_edge_storm;
    parse_fen("7k/8/8/8/8/8/8/K7 w - - 0 1", &board_edge_no_storm);
    parse_fen("7k/8/8/8/8/2p5/8/K7 w - - 0 1", &board_edge_storm);

    int eval_edge_no_storm = evaluate(&board_edge_no_storm);
    int eval_edge_storm = evaluate(&board_edge_storm);

    printf("Eval White King a1 (No Storm): %d\n", eval_edge_no_storm);
    printf("Eval White King a1 (Black Pawn on c3 attacking b2): %d\n", eval_edge_storm);

    assert(eval_edge_storm < eval_edge_no_storm);

    // 3. Test White pawn storm on Black King (Black King on g8)
    // Side to move: Black. Evaluated from Black's perspective.
    // Position 1: Black King g8, no white pawn storm.
    // Position 2: Black King g8, White pawn on f6 attacking g7 & e7.
    Board board_black_no_storm, board_black_storm;
    parse_fen("6k1/4p3/8/8/8/8/8/K7 b - - 0 1", &board_black_no_storm);
    parse_fen("6k1/4p3/5P2/8/8/8/8/K7 b - - 0 1", &board_black_storm);

    int eval_black_no_storm = evaluate(&board_black_no_storm);
    int eval_black_storm = evaluate(&board_black_storm);

    printf("Eval Black perspective (No White Pawn Storm): %d\n", eval_black_no_storm);
    printf("Eval Black perspective (White Pawn Storm on g7): %d\n", eval_black_storm);

    // White pawn storming Black king should decrease evaluation from Black's perspective
    assert(eval_black_storm < eval_black_no_storm);

    printf("Pawn storm evaluation tests passed successfully!\n");
    return 0;
}
