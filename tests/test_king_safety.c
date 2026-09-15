#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "evaluate.h"
#include "board.h"

void test_pawn_shield_penalties(void) {
    printf("--- Running Pawn Shield Evaluation Tests ---\n");

    // 1. Kingside Shield Test
    Board intact_shield = {0};
    Board destroyed_shield = {0};
    
    parse_fen("r1bq1rk1/pppp1ppp/2n2n2/4p3/4P3/2N2N2/PPPP1PPP/R1BQ1RK1 w - - 0 1", &intact_shield);
    parse_fen("r1bq1rk1/pppp1ppp/2n2n2/4p3/4P1PP/2N2N2/PPPP1P2/R1BQ1RK1 w - - 0 1", &destroyed_shield);

    int score_ks_intact = evaluate(&intact_shield);
    int score_ks_destroyed = evaluate(&destroyed_shield);
    int diff_ks = score_ks_intact - score_ks_destroyed;

    printf("[Kingside]  Intact: %d | Destroyed: %d | Diff: %d (Expected > 400)\n", 
           score_ks_intact, score_ks_destroyed, diff_ks);
    assert(score_ks_intact > score_ks_destroyed + 400);


    // 2. Kingside Fianchetto Test
    Board fianchetto_with_bishop = {0};
    Board fianchetto_no_bishop = {0};

    parse_fen("r1bq1rk1/pppp1ppp/2n2n2/4p3/4P3/2N2NP1/PPPP1PBP/R1BQ1RK1 w - - 0 1", &fianchetto_with_bishop);
    parse_fen("r1bq1rk1/pppp1ppp/2n2n2/4p3/4P3/2N2NP1/PPPP1P1P/R1BQ1RK1 w - - 0 1", &fianchetto_no_bishop);

    int score_fianchetto = evaluate(&fianchetto_with_bishop);
    int score_fianchetto_hole = evaluate(&fianchetto_no_bishop);
    int diff_fianchetto = score_fianchetto - score_fianchetto_hole;

    printf("[Fianchetto] With B: %d | Without B: %d | Diff: %d (Expected > 200)\n", 
           score_fianchetto, score_fianchetto_hole, diff_fianchetto);
    assert(score_fianchetto > score_fianchetto_hole + 200);


    // 3. Queenside Shield Test
    Board queenside_intact = {0};
    Board queenside_destroyed = {0};

    parse_fen("2kr1bnr/ppp1pppp/2n5/3p4/3P4/2N5/PPPBPPPP/2KR1BNR w - - 0 1", &queenside_intact);
    parse_fen("2kr1bnr/ppp1pppp/2n5/3p4/1PPP4/2N5/P2BPPPP/2KR1BNR w - - 0 1", &queenside_destroyed);

    int score_qs_intact = evaluate(&queenside_intact);
    int score_qs_destroyed = evaluate(&queenside_destroyed);
    int diff_qs = score_qs_intact - score_qs_destroyed;

    printf("[Queenside] Intact: %d | Destroyed: %d | Diff: %d (Expected > 200)\n", 
           score_qs_intact, score_qs_destroyed, diff_qs);
    assert(score_qs_intact > score_qs_destroyed + 200);

    printf("\nALL PAWN SHIELD EVALUATION TESTS PASSED!\n");
}

int main(void) {
    test_pawn_shield_penalties();
    return 0;
}