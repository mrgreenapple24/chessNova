#include "knights.h"

/** @brief Attacks that can be made by the knight for each square. */
U64 arrKnightAttacks[64] = {0x20400,
                            0x50800,
                            0xa1100,
                            0x142200,
                            0x284400,
                            0x508800,
                            0xa01000,
                            0x402000,
                            0x2040004,
                            0x5080008,
                            0xa110011,
                            0x14220022,
                            0x28440044,
                            0x50880088,
                            0xa0100010,
                            0x40200020,
                            0x204000402,
                            0x508000805,
                            0xa1100110a,
                            0x1422002214,
                            0x2844004428,
                            0x5088008850,
                            0xa0100010a0,
                            0x4020002040,
                            0x20400040200,
                            0x50800080500,
                            0xa1100110a00,
                            0x142200221400,
                            0x284400442800,
                            0x508800885000,
                            0xa0100010a000,
                            0x402000204000,
                            0x2040004020000,
                            0x5080008050000,
                            0xa1100110a0000,
                            0x14220022140000,
                            0x28440044280000,
                            0x50880088500000,
                            0xa0100010a00000,
                            0x40200020400000,
                            0x204000402000000,
                            0x508000805000000,
                            0xa1100110a000000,
                            0x1422002214000000,
                            0x2844004428000000,
                            0x5088008850000000,
                            0xa0100010a0000000,
                            0x4020002040000000,
                            0x400040200000000,
                            0x800080500000000,
                            0x1100110a00000000,
                            0x2200221400000000,
                            0x4400442800000000,
                            0x8800885000000000,
                            0x100010a000000000,
                            0x2000204000000000,
                            0x4020000000000,
                            0x8050000000000,
                            0x110a0000000000,
                            0x22140000000000,
                            0x44280000000000,
                            0x88500000000000,
                            0x10a00000000000,
                            0x20400000000000};

/** @brief Returns the attacks made by knight for the given square. */
U64 sqknightAttacks(int sq) {
    return arrKnightAttacks[sq];
}

/** @brief Returns the attacks made by knights for the given bitboard. */
U64 knightAttacks(U64 knights) {
    U64 l1 = (knights >> 1) & (0x7f7f7f7f7f7f7f7f);
    U64 l2 = (knights >> 2) & (0x3f3f3f3f3f3f3f3f);
    U64 r1 = (knights << 1) & (0xfefefefefefefefe);
    U64 r2 = (knights << 2) & (0xfcfcfcfcfcfcfcfc);
    U64 h1 = l1 | r1;
    U64 h2 = l2 | r2;
    return (h1 << 16) | (h1 >> 16) | (h2 << 8) | (h2 >> 8);
}

/** @brief Returns the bitboard with the knight's attacks and the knight itself.
 */
U64 knightFill(U64 knights) {
    return knightAttacks(knights) | knights;
}

/** @brief Returns the square of the fork target square for the given bitboard
 * of targets. */
U64 forkTargetSquare(U64 targets) {
    U64 west, east, attak, forks;
    east = shiftEast(targets);
    west = shiftWest(targets);
    attak = east << 16;
    forks = (west << 16) & attak;
    attak |= west << 16;
    forks |= (east >> 16) & attak;
    attak |= east >> 16;
    forks |= (west >> 16) & attak;
    attak |= west >> 16;
    east = shiftEast(east);
    west = shiftWest(west);
    forks |= (east << 8) & attak;
    attak |= east << 8;
    forks |= (west << 8) & attak;
    attak |= west << 8;
    forks |= (east >> 8) & attak;
    attak |= east >> 8;
    forks |= (west >> 8) & attak;
    return forks;
}

/** @brief Returns the distance between two squares using knight moves. */
int calcKnightDistance(U64 b1, U64 b2) {
    int d = 0;
    while ((b1 & b2) == 0) {
        b1 = knightAttacks(b1); // as long as sets are disjoint
        d++;                    // increment distance
    }
    return d;
}

/** @brief Returns the distance between two squares using knight moves. */
int knightDistance(int a, int b) {
    return calcKnightDistance((U64)(1) << a, (U64)(1) << b);
}
