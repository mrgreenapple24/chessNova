#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include "evaluate.h"
#include "board.h"

void test_pawn_shield_penalties(void) {
    printf("Running Pawn Shield Evaluation Tests...\n");

    // 1. Intact Kingside Shield vs. Completely Destroyed Kingside Shield
    Board intact_shield, destroyed_shield;
    
    // Pass FEN first, then Board pointer
    parse_fen("r1bq1rk1/pppp1ppp/2n2n2/4p3/4P3/2N2N2/PPPP1PPP/R1BQ1RK1 w - - 0 1", &intact_shield);
    parse_fen("r1bq1rk1/pppp1ppp/2n2n2/4p3/4P1PP/2N2N2/PPPP1P2/R1BQ1RK1 w - - 0 1", &destroyed_shield);

    int score_intact = evaluate(&intact_shield);
    int score_destroyed = evaluate(&destroyed_shield);

    printf("Intact Shield Score: %d | Destroyed Shield Score: %d\n", score_intact, score_destroyed);
    assert(score_intact > score_destroyed + 500 && "Engine failed to heavily penalize missing/advanced pawn shield");

    // 2. Kingside Fianchetto Shield: Pawn on g3 WITH Bishop on g2 vs WITHOUT Bishop on g2
    Board fianchetto_with_bishop, fianchetto_no_bishop;

    parse_fen("r1bq1rk1/pppp1ppp/2n2n2/4p3/4P3/2N2NP1/PPPP1PBP/R1BQ1RK1 w - - 0 1", &fianchetto_with_bishop);
    parse_fen("r1bq1rk1/pppp1ppp/2n2n2/4p3/4P3/2N2NP1/PPPP1P1P/R1BQ1RK1 w - - 0 1", &fianchetto_no_bishop);

    int score_fianchetto = evaluate(&fianchetto_with_bishop);
    int score_fianchetto_hole = evaluate(&fianchetto_no_bishop);

    printf("Fianchetto w/ Bishop: %d | Fianchetto w/o Bishop: %d\n", score_fianchetto, score_fianchetto_hole);
    assert(score_fianchetto > score_fianchetto_hole + 200 && "Engine failed to penalize un-fianchettoed g3 hole");

    // 3. Queenside Castle Shield: Intact vs. Destroyed Queenside Pawns
    Board queenside_intact, queenside_destroyed;

    parse_fen("2kr1bnr/ppp1pppp/2n5/3p4/3P4/2N5/PPPBPPPP/2KR1BNR w - - 0 1", &queenside_intact);
    parse_fen("2kr1bnr/ppp1pppp/2n5/3p4/1PPP4/2N5/P2BPPPP/2KR1BNR w - - 0 1", &queenside_destroyed);

    int score_qs_intact = evaluate(&queenside_intact);
    int score_qs_destroyed = evaluate(&queenside_destroyed);

    printf("Queenside Intact: %d | Queenside Destroyed: %d\n", score_qs_intact, score_qs_destroyed);
    assert(score_qs_intact > score_qs_destroyed + 400 && "Engine failed to penalize destroyed Queenside shield");

    printf("\nALL PAWN SHIELD EVALUATION TESTS PASSED!\n");
}

int main(void) {
    test_pawn_shield_penalties();
    return 0;
}