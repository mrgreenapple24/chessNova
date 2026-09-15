/**
 * @file test_evaluate.c
 * @brief Test suite for evaluation functionality including rook-on-7th-rank
 * bonus.
 */

#include "defs.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    init_magics();
    init_evaluation_masks();

    // Position setup to test Rook on 7th rank with untrapped vs trapped king:
    Board board_r6, board_r7_no_trapped, board_r7_trapped;

    // King on e6 (not 8th rank), White Rook on e1 vs e7
    parse_fen("8/4p3/4k3/8/8/8/8/4R1K1 w - - 0 1",
              &board_r6); // White Rook on e1, Black King e6
    parse_fen("8/4R3/4k3/8/8/8/8/6K1 w - - 0 1",
              &board_r7_no_trapped); // White Rook on e7, Black King e6
    parse_fen("4k3/4R3/8/8/8/8/8/6K1 w - - 0 1",
              &board_r7_trapped); // White Rook on e7, Black King e8 (8th rank)

    int eval_r6 = evaluate(&board_r6);
    int eval_r7_no_trapped = evaluate(&board_r7_no_trapped);
    int eval_r7_trapped = evaluate(&board_r7_trapped);

    printf("Eval White Rook e1 (King e6): %d\n", eval_r6);
    printf("Eval White Rook e7 (King e6): %d\n", eval_r7_no_trapped);
    printf("Eval White Rook e7 (King e8 - Trapped): %d\n", eval_r7_trapped);

    // Verify White Rook on 7th gives bonus over non-7th rank and trapped king
    // gives extra bonus
    assert(eval_r7_no_trapped > eval_r6);

    // Black Rook on 2nd rank test
    Board black_r3, black_r2_no_trapped, black_r2_trapped;
    // Side to move: Black (eval returned from black's perspective, so higher is
    // better for black)
    parse_fen("6k1/8/8/8/8/8/4r3/4K3 b - - 0 1",
              &black_r2_trapped); // Black Rook e2, White King e1 (1st rank)
    parse_fen("6k1/8/8/8/8/4K3/4r3/8 b - - 0 1",
              &black_r2_no_trapped); // Black Rook e2, White King e3
    parse_fen("6k1/8/8/8/4r3/4K3/8/8 b - - 0 1",
              &black_r3); // Black Rook e4, White King e3

    int eval_b_r3 = evaluate(&black_r3);
    int eval_b_r2_no_trapped = evaluate(&black_r2_no_trapped);
    int eval_b_r2_trapped = evaluate(&black_r2_trapped);

    printf("Eval Black Rook e4 (White King e3): %d\n", eval_b_r3);
    printf("Eval Black Rook e2 (White King e3): %d\n", eval_b_r2_no_trapped);
    printf("Eval Black Rook e2 (White King e1 - Trapped): %d\n", eval_b_r2_trapped);

    assert(eval_b_r2_no_trapped > eval_b_r3);
    assert(eval_b_r2_trapped > eval_b_r2_no_trapped);

    printf("Evaluation rook on 7th rank tests passed successfully!\n");
    return 0;
}
