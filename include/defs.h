#ifndef DEFS_H
#define DEFS_H

#include "bitboard.h"
#include "board.h"
#include "evaluate.h"
#include "kings.h"
#include "knights.h"
#include "movegen.h"
#include "pawns.h"
#include "search.h"
#include "sliding.h"
#include "types.h"
#include "uci.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define startFEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#ifndef DEBUG
#define ASSERT(n)
#else
#define ASSERT(n)                                                                                  \
    if (!(n)) {                                                                                    \
        printf("%s - Failed", #n);                                                                 \
        printf("On %s ", __DATE__);                                                                \
        printf("At %s ", __TIME__);                                                                \
        printf("In File %s ", __FILE__);                                                           \
        printf("At Line %d\n", __LINE__);                                                          \
        exit(1);                                                                                   \
    }
#endif

#endif
