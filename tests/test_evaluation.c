/**
 * @file test_evaluation.c
 * @brief Test suite for chessNova static evaluation accuracy.
 */

#include "defs.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Tests the evaluation of a position against an expected score.
 *
 * @param fen The FEN string to parse.
 * @param expected The expected evaluation score.
 * @param tolerance Allowed difference between actual and expected score.
 * @param description A brief explanation of the test case.
 */
void test_evaluation(char* fen, int expected, int tolerance, char* description) {
    Board board;
    parse_fen(fen, &board);

    int score = evaluate(&board);

    if (abs(score - expected) > tolerance) {
        fprintf(stderr, "FAIL: %s\n", description);
        fprintf(stderr, "  FEN: %s\n", fen);
        fprintf(stderr, "  Expected: %d\n", expected);
        fprintf(stderr, "  Found: %d\n", score);
        fprintf(stderr, "  Difference: %d\n", abs(score - expected));
        exit(1);
    }
}

/**
 * @brief Main execution entry for the evaluation test suite.
 *
 * Runs several positions designed to test material, pawn structure,
 * king safety, and positional evaluation.
 *
 * @return int Exit status (0 for success, 1 for failure).
 */
int main() {
    init_magics();
    init_evaluation_masks();

    /*
     * Starting position.
     *
     * A symmetric starting position should evaluate to approximately 0.
     */
    test_evaluation("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 0, 0,
                    "Starting Position");

    /*
     * White has an extra queen.
     */
    test_evaluation("4k3/8/8/8/8/8/4Q3/4K3 w - - 0 1", 900, 50, "White Extra Queen");

    /*
     * Black has an extra queen.
     */
    test_evaluation("4k3/4q3/8/8/8/8/8/4K3 w - - 0 1", -900, 50, "Black Extra Queen");

    /*
     * White has an extra pawn.
     */
    test_evaluation("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1", 100, 20, "White Extra Pawn");

    /*
     * Black has an extra pawn.
     */
    test_evaluation("4k3/4p3/8/8/8/8/8/4K3 w - - 0 1", -100, 20, "Black Extra Pawn");

    printf("Evaluation tests passed!\n");

    return 0;
}