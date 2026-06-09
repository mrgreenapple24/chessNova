#ifndef PAWNS_H
#define PAWNS_H

#include "types.h"
#include "bitboard.h"

// push

U64 wSinglePushTargets(U64 wpawns, U64 empty);
U64 bSinglePushTargets(U64 bpawns, U64 empty);
U64 wDblPushTargets(U64 wpawns, U64 empty);
U64 bDblPushTargets(U64 bpawns, U64 empty);
void push_white_pawn(U64 white_pawn, Board *board);
void push_dwhite_pawn(U64 pawn, Board *board);
void push_black_pawn(U64 black_pawn, Board *board);
void push_dblack_pawn(U64 pawn, Board *board);

U64 wPawnsAble2Push(U64 wpawns, U64 empty);
U64 bPawnsAble2Push(U64 bpawns, U64 empty);
U64 wPawnsAble2DblPush(U64 wpawns, U64 empty);
U64 bPawnsAble2DblPush(U64 bpawns, U64 empty);

// Attacks

U64 wPawnEastAttacks(U64 wpawns);
U64 wPawnWestAttacks(U64 wpawns);
U64 bPawnEastAttacks(U64 bpawns);
U64 bPawnWestAttacks(U64 bpawns);

U64 wPawnAnyAttacks(U64 wpawns);
U64 wPawnDblAttacks(U64 wpawns);
U64 wPawnSingleAttacks(U64 wpawns);

U64 bPawnAnyAttacks(U64 bpawns);
U64 bPawnDblAttacks(U64 bpawns);
U64 bPawnSingleAttacks(U64 bpawns);

U64 wSafePawnSquares(U64 wpawns, U64 bpawns);
U64 bSafePawnSquares(U64 bpawns, U64 wpawns);

U64 wPawnsAble2CaptureEast(U64 wpawns, U64 bpieces);
U64 wPawnsAble2CaptureWest(U64 wpawns, U64 bpieces);
U64 wPawnsAble2CaptureAny(U64 wpawns, U64 bpieces);

U64 bPawnsAble2CaptureEast(U64 bpawns, U64 wpieces);
U64 bPawnsAble2CaptureWest(U64 bpawns, U64 wpieces);
U64 bPawnsAble2CaptureAny(U64 bpawns, U64 wpieces);

U64 wRam(U64 wpawns, U64 bpawns);
U64 bRam(U64 wpawns, U64 bpawns);

U64 wEastLever(U64 wpawns, U64 bpawns);
U64 wWestLever(U64 wpawns, U64 bpawns);
U64 wAnyLever(U64 wpawns, U64 bpawns);
U64 bEastLever(U64 bpawns, U64 wpawns);
U64 bWestLever(U64 bpawns, U64 wpawns);
U64 bAnyLever(U64 bpawns, U64 wpawns);

U64 wInnerLever(U64 wpawns, U64 bpawns);
U64 wOuterLever(U64 wpawns, U64 bpawns);
U64 wCenterLever(U64 wpawns, U64 bpawns);
U64 bInnerLever(U64 bpawns, U64 wpawns);
U64 bOuterLever(U64 bpawns, U64 wpawns);
U64 bCenterLever(U64 bpawns, U64 wpawns);

U64 wPawnDefendedFromWest(U64 wpawns);
U64 wPawnDefendedFromEast(U64 wpawns);
U64 bPawnDefendedFromWest(U64 bpawns);
U64 bPawnDefendedFromEast(U64 bpawns);

U64 pawnsWithEastNeighbors(U64 pawns);
U64 pawnsWithWestNeighbors(U64 pawns);
U64 duo (U64 pawns);
U64 northFill(U64 wpawns);
U64 southFill(U64 bpawns);
U64 fileFill(U64 pawns);
U64 closedFiles(U64 wpawns,U64 bpawns);
U64 openFiles(U64 wpanws, U64 bpawns);

extern U64 arrwPawnAttacks[64];
extern U64 arrbPawnAttacks[64];

U64 wFrontspans(U64 wpawns);
U64 bRearspans(U64 wpawns);
U64 wRearspans(U64 wpawns);
U64 bFrontspans(U64 wpawns);

U64 eastAttackFileFill (U64 pawns);
U64 westAttackFileFill (U64 pawns);

uint8_t fileSet(U64 pawns);
uint8_t islandsEastfiles(U64 pawns);
uint8_t islandsWestfiles(U64 pawns);
uint8_t isolatedFiles(U64 pawns);
int countIslands(U64 pawns);
U64 noNeighbourOnEastFile (U64 pawns);
U64 noNeighbourOnWestFile (U64 pawns);
U64 isolanis(U64 pawns);
U64 halfIsolanis(U64 pawns);
U64 wOpenPawns(U64 wpawns, U64 bpawns);
U64 bOpenPawns(U64 bpawns, U64 wpawns);
U64 wHangingPawns(U64 wpawns, U64 bpawns);
#endif
