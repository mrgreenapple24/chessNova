#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "defs.h"

#ifdef _WIN32
#include <io.h>
#define dup _dup
#define dup2 _dup2
#define NULL_DEVICE "NUL"
#else
#include <unistd.h>
#define NULL_DEVICE "/dev/null"
#endif

int saved_stdout_fd = -1;

void mute_stdout(void) {
    fflush(stdout);
    saved_stdout_fd = dup(1);
    freopen(NULL_DEVICE, "w", stdout);
}

void unmute_stdout(void) {
    if (saved_stdout_fd != -1) {
        fflush(stdout);
        dup2(saved_stdout_fd, 1);
        #ifdef _WIN32
        _close(saved_stdout_fd);
        #else
        close(saved_stdout_fd);
        #endif
        saved_stdout_fd = -1;
    }
}

void test_search(char *fen, int depth, int expected_from, int expected_to, int check_promoted, char *description) {
    Board board;
    parse_fen(fen, &board);

    SearchInfo info;
    memset(&info, 0, sizeof(SearchInfo));
    info.depth = depth;
    info.starttime = get_time_ms();

    uint32_t move = search_best_move(&board, &info);
    
    int from = GET_FROM(move);
    int to = GET_TO(move);
    int promoted = GET_PROMOTED(move);
    
    if (from != expected_from || to != expected_to || (check_promoted && promoted == EMPTY)) {
        unmute_stdout();
        fprintf(stderr, "FAIL: %s\n", description);
        fprintf(stderr, "  FEN: %s\n", fen);
        fprintf(stderr, "  Expected move from %d to %d\n", expected_from, expected_to);
        fprintf(stderr, "  Found move: %s\n", move_to_string(move));
        exit(1);
    }
}

int main() {
    init_magics();
    init_evaluation_masks();

    mute_stdout();

    // Mate in 1
    // f7 (53) -> e8 (60), check_promoted=1
    test_search("4k3/5P2/8/8/8/8/8/4K3 w - - 0 1", 3, 53, 60, 1, "Mate in 1 (Promotion)");

    // Capture hanging piece
    // e2 (12) -> d3 (19)
    test_search("4k3/8/8/8/8/3q4/4P3/4K3 w - - 0 1", 3, 12, 19, 0, "Capture hanging Queen");

    // Scholar's Mate (Defend)
    // g8 (62) -> f6 (45)
    test_search("rnb1kbnr/pppp1ppp/8/4p3/2B1P3/5Q2/PPPP1PPP/RNB1K1NR b KQkq - 0 1", 3, 62, 45, 0, "Defend Scholar's Mate");

    // Middle game position
    // e1 (4) -> g1 (6)
    test_search("r1bqk2r/pppp1ppp/2n2n2/4p3/2B1P3/2P2N2/PP1P1PPP/RNBQK2R w KQkq - 0 5", 6, 4, 6, 0, "Standard Opening (Depth 6)");

    unmute_stdout();
    printf("Search tests passed!\n");
    return 0;
}
