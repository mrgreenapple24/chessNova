#include "defs.h"

U64 arrFiles[8] = {0x101010101010101,  0x202020202020202,  0x404040404040404,  0x808080808080808,
                   0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080};

/** @brief Shifts a bitboard south (down).
 * @param b The bitboard to shift.
 * @return The shifted bitboard.
 */
U64 shiftSouth(U64 b) {
    return b >> 8;
}
/** @brief Shifts a bitboard north (up).
 * @param b The bitboard to shift.
 * @return The shifted bitboard.
 */
U64 shiftNorth(U64 b) {
    return b << 8;
}

/** @brief Returns a bitboard with all occupied squares set.
 * @param board The board to get the occupied squares from.
 * @return A bitboard with all occupied squares set.
 */
U64 get_occupied(Board board) {
    U64 occupied = 0ULL;

    for (int piece = wp; piece <= bk; piece++) {
        occupied |= board.bitboards[piece];
    }

    return occupied;
}

/** @brief Returns a bitboard with all empty squares set.
 * @param occupied The bitboard with all occupied squares set.
 * @return A bitboard with all empty squares set.
 */
U64 get_empty(U64 occupied) {
    return ~occupied;
}

/** @brief Shifts a bitboard east (right).
 * @param b The bitboard to shift.
 * @return The shifted bitboard.
 */
U64 shiftEast(U64 b) {
    return (b << 1) & notAFile;
}
/** @brief Shifts a bitboard northeast (up and right).
 * @param b The bitboard to shift.
 * @return The shifted bitboard.
 */
U64 shiftNortheast(U64 b) {
    return (b << 9) & notAFile;
}
/** @brief Shifts a bitboard southeast (down and right).
 * @param b The bitboard to shift.
 * @return The shifted bitboard.
 */
U64 shiftSoutheast(U64 b) {
    return (b >> 7) & notAFile;
}
/** @brief Shifts a bitboard west (left).
 * @param b The bitboard to shift.
 * @return The shifted bitboard.
 */
U64 shiftWest(U64 b) {
    return (b >> 1) & notHFile;
}
/** @brief Shifts a bitboard southwest (down and left).
 * @param b The bitboard to shift.
 * @return The shifted bitboard.
 */
U64 shiftSouthwest(U64 b) {
    return (b >> 9) & notHFile;
}
/** @brief Shifts a bitboard northwest (up and left).
 * @param b The bitboard to shift.
 * @return The shifted bitboard.
 */
U64 shiftNorthwest(U64 b) {
    return (b << 7) & notHFile;
}

/** @brief Tests if a bit is set at the given square.
 * @param b The bitboard to test.
 * @param square The square to test.
 * @return True if the bit is set, false otherwise.
 */
bool test_bit(U64 b, int square) {
    return (b & (1ULL << square));
}

/** @brief Sets a bit at the given square.
 * @param b The bitboard to set.
 * @param square The square to set.
 */
void set_bit(U64* b, int square) {
    *b |= (1ULL << square);
}

/** @brief Clears a bit at the given square.
 * @param b The bitboard to clear.
 * @param square The square to clear.
 */
void clear_bit(U64* b, int square) {
    *b &= ~(1ULL << square);
}

/** @brief Toggles a bit at the given square.
 * @param b The bitboard to toggle.
 * @param square The square to toggle.
 */
void toggle_bit(U64* b, int square) {
    *b ^= (1ULL << square);
}
/*
U64 attacksTo(U64 occupied, int sq, Board *board) {
   U64 knights, kings, bishopsQueens, rooksQueens;
   knights        = board->bitboards[wn] | board->bitboards[bn];
   kings          = board->bitboards[wk] | board->bitboards[bk];
   bishopsQueens  = board->bitboards[wq] | board->bitboards[bq];
   rooksQueens    = board->bitboards[wr] | board->bitboards[br];

   return (arrwPawnAttacks[sq] & board->bitboards[bp])
        | (arrbPawnAttacks[sq] & board->bitboards[wp])
        | (arrKnightAttacks      [sq] & knights)
        | (arrKingAttacks        [sq] & kings)
        | (bishopAttacks(occupied,sq) & bishopsQueens)
        | (rookAttacks  (occupied,sq) & rooksQueens)
        ;
}
*/

/**
 * @brief Pops the least significant bit set in a 64-bit integer.
 * @param bb The bitboard to pop.
 * @return The index of the least significant bit set.
 */
int pop_lsb(U64* bb) {
    int index = get_lsb(*bb);
    *bb &= (*bb - 1);
    return index;
}
