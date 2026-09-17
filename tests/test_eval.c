#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitboard.h"
#include "board.h"
#include "evaluate.h"
#include "kings.h"
#include "pawns.h"
#include "types.h"

/* Helper macro for test assertion reporting */
#define RUN_TEST(test_func)                                                                        \
    do {                                                                                           \
        printf("Running %s...", #test_func);                                                       \
        test_func();                                                                               \
        printf(" PASSED\n");                                                                       \
    } while (0)

/* Test Cases */

static void test_initial_position(void) {
    Board board;
    parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &board);

    int score = evaluate(&board);

    // Starting position should be perfectly symmetrical (0 evaluation)
    assert(score == 0 && "Initial position evaluation must be 0");
}

static void test_side_to_move_perspective(void) {
    Board board_white, board_black;
    // Identical quiet position, only active side changes
    const char* fen_w = "r1bqk2r/pp1pppbp/2n2np1/8/3NP3/2N1B3/PPP2PPP/R2QKB1R w KQkq - 0 1";
    const char* fen_b = "r1bqk2r/pp1pppbp/2n2np1/8/3NP3/2N1B3/PPP2PPP/R2QKB1R b KQkq - 0 1";

    parse_fen(fen_w, &board_white);
    parse_fen(fen_b, &board_black);

    int score_w = evaluate(&board_white);
    int score_b = evaluate(&board_black);

    // Relative evaluation must negate when side to move flips
    assert(score_w == -score_b && "Evaluation must flip sign for opposite side to move");
}

static void test_material_advantage(void) {
    Board board;
    // White is up a Queen
    parse_fen("rnb1kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &board);

    int score = evaluate(&board);

    // Evaluation should strongly favor White (> +500 centipawns)
    assert(score > 500 && "White up a queen should produce a strong positive evaluation");
}

static void test_pawn_structure_evaluation(void) {
    Board board_passed, board_doubled;

    // White has a clear passed pawn on d6
    parse_fen("8/8/3P4/8/8/8/4k3/4K3 w - - 0 1", &board_passed);
    // White has doubled pawns on d2 and d3
    parse_fen("8/8/8/8/8/3P4/3P4/4K2k w - - 0 1", &board_doubled);

    int passed_score = evaluate(&board_passed);
    int doubled_score = evaluate(&board_doubled);

    assert(passed_score > 0 && "Passed pawn position should evaluate positively");
    (void)doubled_score; // Suppress unused variable warning
}

int main(void) {
    printf("===============================\n");
    printf("  Running Evaluation Unit Tests\n");
    printf("===============================\n\n");

    // Run mask initialization before testing evaluation logic
    init_evaluation_masks();

    RUN_TEST(test_initial_position);
    RUN_TEST(test_side_to_move_perspective);
    RUN_TEST(test_material_advantage);
    RUN_TEST(test_pawn_structure_evaluation);

    printf("\nAll tests passed successfully!\n");
    return 0;
}