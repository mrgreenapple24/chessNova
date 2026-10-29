#include "defs.h"
#include "polyglot.h"
#include "tt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
/* Windows timing using QueryPerformanceCounter */
static LARGE_INTEGER get_time_freq(void) {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return freq;
}

static LARGE_INTEGER last_time;
#else
#include <sys/time.h>
#endif

long long get_time_ms(void) {
#ifdef _WIN32
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (count.QuadPart * 1000) / freq.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

char* move_to_string(uint32_t move) {
    static char move_str[6];
    if (move == 0) {
        strcpy(move_str, "none");
        return move_str;
    }
    int from = GET_FROM(move);
    int to = GET_TO(move);
    int promoted = GET_PROMOTED(move);

    move_str[0] = 'a' + (from % 8);
    move_str[1] = '1' + (from / 8);
    move_str[2] = 'a' + (to % 8);
    move_str[3] = '1' + (to / 8);
    move_str[4] = '\0';

    if (promoted != EMPTY) {
        char pchar = ' ';
        switch (promoted) {
        case wn:
        case bn:
            pchar = 'n';
            break;
        case wb:
        case bb:
            pchar = 'b';
            break;
        case wr:
        case br:
            pchar = 'r';
            break;
        case wq:
        case bq:
            pchar = 'q';
            break;
        }
        move_str[4] = pchar;
        move_str[5] = '\0';
    }

    return move_str;
}

uint32_t parse_move(char* ptr, Board* board) {
    if (ptr[1] > '8' || ptr[1] < '1')
        return 0;
    if (ptr[0] > 'h' || ptr[0] < 'a')
        return 0;
    if (ptr[3] > '8' || ptr[3] < '1')
        return 0;
    if (ptr[2] > 'h' || ptr[2] < 'a')
        return 0;

    uint32_t from = (ptr[1] - '1') * 8 + (ptr[0] - 'a');
    uint32_t to = (ptr[3] - '1') * 8 + (ptr[2] - 'a');

    MoveList list;
    generate_all_moves(board, &list);

    for (int i = 0; i < list.count; i++) {
        uint32_t move = list.moves[i].move;
        if (GET_FROM(move) == from && GET_TO(move) == to) {
            int promoted = GET_PROMOTED(move);
            if (promoted != EMPTY) {
                if (ptr[4] == 'n' && (promoted == wn || promoted == bn))
                    return move;
                if (ptr[4] == 'b' && (promoted == wb || promoted == bb))
                    return move;
                if (ptr[4] == 'r' && (promoted == wr || promoted == br))
                    return move;
                if (ptr[4] == 'q' && (promoted == wq || promoted == bq))
                    return move;
                continue;
            }
            return move;
        }
    }

    return 0;
}

void parse_position(char* line, Board* board) {
    line += 9; // Skip "position "
    char* ptr = line;

    if (strncmp(line, "startpos", 8) == 0) {
        parse_fen(startFEN, board);
    } else {
        ptr = strstr(line, "fen");
        if (ptr == NULL) {
            parse_fen(startFEN, board);
        } else {
            ptr += 4;
            parse_fen(ptr, board);
        }
    }

    ptr = strstr(line, "moves");
    if (ptr != NULL) {
        ptr += 6;
        while (*ptr) {
            uint32_t move = parse_move(ptr, board);
            if (move == 0)
                break;
            make_move(board, move);
            while (*ptr && *ptr != ' ')
                ptr++;
            if (*ptr == ' ')
                ptr++;
        }
    }
}

static int initialized = 0;

void ensure_initialized() {
    if (!initialized) {
        init_zobrist();
        init_magics();
        init_evaluation_masks();
        InitPolyBook();
        if (TT.pTable == NULL) {
            init_tt(64);
        }
        initialized = 1;
    }
}

void uci_loop() {
    ensure_initialized();
    char line[2048];
    Board board;
    parse_fen(startFEN, &board);

    setbuf(stdout, NULL);

    while (1) {
        if (!fgets(line, sizeof(line), stdin))
            break;
        if (line[0] == '\n')
            continue;

        if (strncmp(line, "uci", 3) == 0) {
            printf("id name chessNova\n");
            printf("id author Mrgreenapple24\n");
            printf("option name Hash type spin default 64 min 1 max 1024\n");
            printf("option name Clear Hash type button\n");
            printf("option name OwnBook type check default true\n");
            printf("option name BookFile type string default book.bin\n");
            printf("uciok\n");
            fflush(stdout);
        } else if (strncmp(line, "isready", 7) == 0) {
            ensure_initialized();
            printf("readyok\n");
            fflush(stdout);
        } else if (strncmp(line, "position", 8) == 0) {
            ensure_initialized();
            parse_position(line, &board);
        } else if (strncmp(line, "ucinewgame", 10) == 0) {
            ensure_initialized();
            clear_tt();
            parse_fen(startFEN, &board);
        } else if (strncmp(line, "setoption", 9) == 0) {
            char* name = strstr(line, "name");
            char* value = strstr(line, "value");
            if (name) {
                name += 5;
                if (value) {
                    char* end = strstr(name, "value");
                    if (end)
                        *end = '\0';
                }

                /* Trim whitespace from name */
                while (*name == ' ')
                    name++;
                char* n_end = name + strlen(name) - 1;
                while (n_end > name && (*n_end == ' ' || *n_end == '\n' || *n_end == '\r')) {
                    *n_end = '\0';
                    n_end--;
                }

                if (value) {
                    value += 6;
                    /* Trim whitespace from value */
                    while (*value == ' ')
                        value++;
                    char* v_end = value + strlen(value) - 1;
                    while (v_end > value && (*v_end == ' ' || *v_end == '\n' || *v_end == '\r')) {
                        *v_end = '\0';
                        v_end--;
                    }
                }

                if (strcmp(name, "Hash") == 0 && value) {
                    int mb = atoi(value);
                    if (mb < 1)
                        mb = 1;
                    if (mb > 1024)
                        mb = 1024;
                    init_tt(mb);
                } else if (strcmp(name, "Clear Hash") == 0) {
                    clear_tt();
                } else if (strcmp(name, "OwnBook") == 0 && value) {
                    if (strcmp(value, "true") == 0)
                        use_book = true;
                    else if (strcmp(value, "false") == 0)
                        use_book = false;
                } else if (strcmp(name, "BookFile") == 0 && value) {
                    strncpy(book_file_path, value, 255);
                }
            }
        } else if (strncmp(line, "go", 2) == 0) {
            ensure_initialized();

            SearchInfo info;
            memset(&info, 0, sizeof(SearchInfo));

            int depth = -1;
            int movetime = -1;
            int time = -1;
            int inc = 0;
            int movestogo = 30;

            char* ptr = NULL;
            if ((ptr = strstr(line, "infinite"))) {
                info.infinite = 1;
            }
            if ((ptr = strstr(line, "winc")) && board.side == white)
                inc = atoi(ptr + 5);
            if ((ptr = strstr(line, "binc")) && board.side == black)
                inc = atoi(ptr + 5);
            if ((ptr = strstr(line, "wtime")) && board.side == white)
                time = atoi(ptr + 6);
            if ((ptr = strstr(line, "btime")) && board.side == black)
                time = atoi(ptr + 6);
            if ((ptr = strstr(line, "movestogo")))
                movestogo = atoi(ptr + 10);
            if ((ptr = strstr(line, "movetime")))
                movetime = atoi(ptr + 9);
            if ((ptr = strstr(line, "depth")))
                depth = atoi(ptr + 6);

            if (movetime != -1) {
                time = movetime;
                movestogo = 1;
            }

            info.starttime = get_time_ms();
            info.depth = (depth == -1) ? 64 : depth;

            if (time != -1) {
                info.timeset = 1;
                time /= movestogo;
                time -= 50; // buffer
                if (time < 0)
                    time = 0;
                info.stoptime = info.starttime + time + inc;
            }

            uint32_t move = search_best_move(&board, &info);
            printf("bestmove %s\n", move_to_string(move));
            fflush(stdout);
        } else if (strncmp(line, "quit", 4) == 0) {
            free_tt();
            break;
        }
    }
}
