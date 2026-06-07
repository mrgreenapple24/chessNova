#ifndef UCI_H
#define UCI_H

#include "types.h"

void uci_loop();
char* move_to_string(uint32_t move);
uint32_t parse_move(char *ptr, Board *board);
long long get_time_ms();

#endif // UCI_H
