/**
 * @file test_perft.c
 * @brief Test suite for move generation correctness using perft.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "defs.h"

int main() {
    // Initialize engine lookup tables (adjust these if your engine uses different init functions)
    init_magics();
    
    Board start_pos, kiwipete, pos3;

    // ---------------------------------------------------------
    // TEST 1: The Standard Starting Position
    // ---------------------------------------------------------
    parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &start_pos);
    
    long long start_d4 = perft_test(&start_pos, 4);
    long long start_d5 = perft_test(&start_pos, 5);
    
    printf("Perft Start Position (Depth 4): %lld\n", start_d4);
    printf("Perft Start Position (Depth 5): %lld\n", start_d5);
    
    assert(start_d4 == 197281LL);
    assert(start_d5 == 4865609LL);

    // ---------------------------------------------------------
    // TEST 2: "Kiwipete" 
    // Tests late castling rights, discovered checks, and promotions.
    // ---------------------------------------------------------
    parse_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", &kiwipete);
    
    long long kiwi_d3 = perft_test(&kiwipete, 3);
    long long kiwi_d4 = perft_test(&kiwipete, 4);
    
    printf("Perft Kiwipete (Depth 3): %lld\n", kiwi_d3);
    printf("Perft Kiwipete (Depth 4): %lld\n", kiwi_d4);
    
    assert(kiwi_d3 == 97862LL);
    assert(kiwi_d4 == 4085603LL);

    // ---------------------------------------------------------
    // TEST 3: Endgame Tactics (Position 3)
    // Explicitly stresses en passant logic and pinned pieces.
    // ---------------------------------------------------------
    parse_fen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", &pos3);
    
    long long pos3_d4 = perft_test(&pos3, 4);
    long long pos3_d5 = perft_test(&pos3, 5);
    
    printf("Perft Position 3 (Depth 4): %lld\n", pos3_d4);
    printf("Perft Position 3 (Depth 5): %lld\n", pos3_d5);
    
    assert(pos3_d4 == 43238LL);
    assert(pos3_d5 == 674624LL);

    // If we make it here, no assertions failed!
    printf("\nAll perft move generation tests passed successfully!\n");
    
    return 0;
}