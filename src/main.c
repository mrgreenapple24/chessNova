/**
 * @file main.c
 * @brief Entry point for the chessNova engine.
 */

#include "defs.h"
#include "movegen.h"
#include "polyglot.h"
#include "sliding.h"
#include <stdio.h>
#include <time.h>

/**
 * @brief Main function. Initializes resources and launches the UCI input loop.
 *
 * @return int Exit status.
 */
int main() {
    init_magics();
    uci_loop();

    return 0;
}
