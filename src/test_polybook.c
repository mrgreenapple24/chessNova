#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "defs.h"
#include "polyglot.h"
#include "polykeys.h"

#ifdef _WIN32
#include <io.h>
#define dup _dup
#define dup2 _dup2
#define NULL_DEVICE "NUL"
#else
#include <unistd.h>
#define NULL_DEVICE "/dev/null"
#endif

// Declaration of internal function from polybook.c to test consistency
U64 PolyKeyFromBoard(const Board *board);

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

void test_hash(const char *fen, uint64_t expected_hash, const char *description) {
    Board board;
    parse_fen(fen, &board);
    
    uint64_t hash = polyglot_hash(&board);
    uint64_t key_from_polybook = PolyKeyFromBoard(&board);
    
    if (hash != expected_hash || key_from_polybook != hash) {
        unmute_stdout();
        fprintf(stderr, "FAIL: %s\n", description);
        fprintf(stderr, "  FEN: %s\n", fen);
        fprintf(stderr, "  Calculated Hash: 0x%016lX\n", hash);
        fprintf(stderr, "  Expected Hash:   0x%016lX\n", expected_hash);
        fprintf(stderr, "  Polybook Hash:   0x%016lX\n", key_from_polybook);
        exit(1);
    }
}

int main() {
    init_magics();
    init_evaluation_masks();
    
    mute_stdout();
    
    // 1. Test hash correctness against standard Polyglot reference values
    test_hash(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        0x463B96181691FC9CULL,
        "Starting Position"
    );
    
    test_hash(
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1",
        0x823C9B50FD114196ULL,
        "After 1. e4"
    );
    
    test_hash(
        "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
        0x0844931A6EF4B9A0ULL,
        "After 1. e4 e5"
    );
    
    test_hash(
        "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2",
        0xD3207FEC0612D89DULL,
        "After 1. e4 e5 2. Nf3"
    );
    
    // 2. Test Polybook functionality
    FILE *f_test = fopen("book.bin", "rb");
    if (f_test) {
        strncpy(book_file_path, "book.bin", sizeof(book_file_path) - 1);
        fclose(f_test);
    } else {
        f_test = fopen("../book.bin", "rb");
        if (f_test) {
            strncpy(book_file_path, "../book.bin", sizeof(book_file_path) - 1);
            fclose(f_test);
        } else {
            f_test = fopen("../../book.bin", "rb");
            if (f_test) {
                strncpy(book_file_path, "../../book.bin", sizeof(book_file_path) - 1);
                fclose(f_test);
            } else {
                strncpy(book_file_path, "book.bin", sizeof(book_file_path) - 1);
            }
        }
    }
    InitPolyBook();
    
    if (!use_book) {
        unmute_stdout();
        fprintf(stderr, "FAIL: Failed to load book.bin\n");
        exit(1);
    }
    
    Board board;
    parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &board);
    
    uint32_t book_move = GetBookMove(&board);
    if (book_move == 0) {
        unmute_stdout();
        fprintf(stderr, "FAIL: Book move for start position not found\n");
        CleanPolyBook();
        exit(1);
    }
    
    CleanPolyBook();
    
    unmute_stdout();
    printf("Polybook tests passed!\n");
    return 0;
}
