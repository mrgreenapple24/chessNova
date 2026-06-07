#include "defs.h"
#include <stdio.h>
#include <stdlib.h>

/** @brief Character representation of each piece type. */
char piece_char[] = "PNBRQKpnbrqk";

/**
 * @brief Prints the current board state to the console.
 * @param board Pointer to the Board structure.
 */
void print_board(Board *board) {
    printf("\n");
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file <= 7; file++) {
            int square = rank * 8 + file;
            int piece = -1;

            for (int bb_piece = wp; bb_piece <= bk; bb_piece++) {
                if ((board->bitboards[bb_piece] >> square) & 1ULL) {
                    piece = bb_piece;
                    break;
                }
            }

            if (piece == -1) printf(" . ");
            else printf(" %c ", piece_char[piece]);
        }
        printf("  %d\n", rank + 1);
    }
    printf("\n a  b  c  d  e  f  g  h\n\n");
}

/**
 * @brief Prints the given bitboard to the console.
 * @param bitboard The 64-bit integer representing the bitboard.
 */
void print_bitboard(U64 bitboard) {
    printf("\n");
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file <= 7; file++) {
            int square = rank * 8 + file;

            if ((bitboard >> square) & 1ULL)
                printf(" 1 ");
            else
                printf(" . ");
        }
        printf("  %d\n", rank + 1);
    }
    printf("\n a  b  c  d  e  f  g  h\n\n");
    printf(" Bitboard: 0x%llx\n\n", (unsigned long long)bitboard);
}

/**
 * @brief Resets the board to its initial state.
 * @param board Pointer to the Board structure.
 */
void reset_board(Board *board) {
    for (int i = 0; i <= bk; i++) {
        board->bitboards[i] = 0;
    }
    for (int i = 0; i < 64; i++) {
        board->pieces[i] = EMPTY;
    }

    board->occupancies[0] = 0;
    board->occupancies[1] = 0;
    board->occupancies[2] = 0;
    board->side = 0;
    board->enpassant = NO_SQ;
    board->castle = 0;
    board->ply = 0;
    board->hisply = 0;
    board->fiftyMove = 0;
}

/**
 * @brief Parses a FEN string and sets the board state accordingly.
 * @param fen The FEN string to parse.
 * @param board Pointer to the Board structure.
 */
void parse_fen(const char *fen, Board *board) {
    reset_board(board);

    int rank = RANK_8;
    int file = FILE_A;

    while (rank >= RANK_1 && *fen != ' ') {
        int count = 1;
        int piece = -1;

        switch (*fen) {
            case 'P': piece = wp; break;
            case 'N': piece = wn; break;
            case 'B': piece = wb; break;
            case 'R': piece = wr; break;
            case 'Q': piece = wq; break;
            case 'K': piece = wk; break;
            case 'p': piece = bp; break;
            case 'n': piece = bn; break;
            case 'b': piece = bb; break;
            case 'r': piece = br; break;
            case 'q': piece = bq; break;
            case 'k': piece = bk; break;

            case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8':
                piece = EMPTY;
                count = *fen - '0';
                break;

            case '/':
                rank--;
                file = FILE_A;
                fen++;
                continue;

            default:
                fen++;
                continue;
        }

        for (int i = 0; i < count; i++) {
            int sq = rank * 8 + file;
            if (piece != EMPTY) {
                set_bit(&board->bitboards[piece], sq);
                board->pieces[sq] = piece;
            } else {
                board->pieces[sq] = EMPTY;
            }
            file++;
        }
        fen++;
    }

    // Side to move
    while (*fen == ' ') fen++;
    board->side = (*fen == 'w') ? white : black;
    fen++;

    // Castling rights
    while (*fen == ' ') fen++;
    if (*fen != '-') {
        while (*fen != ' ' && *fen != '\0') {
            switch (*fen) {
                case 'K': board->castle |= WKCA; break;
                case 'Q': board->castle |= WQCA; break;
                case 'k': board->castle |= BKCA; break;
                case 'q': board->castle |= BQCA; break;
            }
            fen++;
        }
    } else {
        fen++;
    }

    // En passant square
    while (*fen == ' ') fen++;
    if (*fen != '-') {
        int f = fen[0] - 'a';
        int r = fen[1] - '1';
        board->enpassant = r * 8 + f;
        fen += 2;
    } else {
        board->enpassant = NO_SQ;
    }

    // Update occupancies
    board->occupancies[white] = 0ULL;
    board->occupancies[black] = 0ULL;
    for (int p = wp; p <= wk; p++) board->occupancies[white] |= board->bitboards[p];
    for (int p = bp; p <= bk; p++) board->occupancies[black] |= board->bitboards[p];
    board->occupancies[both] = board->occupancies[white] | board->occupancies[black];
}

/**
 * @brief Returns the piece at a given square.
 * @param board Pointer to the Board structure.
 * @param sq The square to check.
 * @return The piece type (e.g., wp, bn, or EMPTY).
 */
int get_piece_at(const Board *board, int sq) {
    return board->pieces[sq];
}

/**
 * @brief Internal helper to move a piece on the board bitboards.
 * @param board Pointer to the Board structure.
 * @param from Source square.
 * @param to Destination square.
 * @param piece The piece being moved.
 */
static void move_piece(Board *board, int from, int to, int piece) {
    clear_bit(&board->bitboards[piece], from);
    set_bit(&board->bitboards[piece], to);
    int side = (piece <= wk) ? white : black;
    clear_bit(&board->occupancies[side], from);
    set_bit(&board->occupancies[side], to);
    clear_bit(&board->occupancies[both], from);
    set_bit(&board->occupancies[both], to);
    board->pieces[from] = EMPTY;
    board->pieces[to] = piece;
}

/**
 * @brief Internal helper to remove a piece from the board bitboards.
 * @param board Pointer to the Board structure.
 * @param sq The square to clear.
 * @param piece The piece to remove.
 */
static void remove_piece(Board *board, int sq, int piece) {
    clear_bit(&board->bitboards[piece], sq);
    int side = (piece <= wk) ? white : black;
    clear_bit(&board->occupancies[side], sq);
    clear_bit(&board->occupancies[both], sq);
    board->pieces[sq] = EMPTY;
}

/**
 * @brief Internal helper to add a piece to the board bitboards.
 * @param board Pointer to the Board structure.
 * @param sq The destination square.
 * @param piece The piece to add.
 */
static void add_piece(Board *board, int sq, int piece) {
    set_bit(&board->bitboards[piece], sq);
    int side = (piece <= wk) ? white : black;
    set_bit(&board->occupancies[side], sq);
    set_bit(&board->occupancies[both], sq);
    board->pieces[sq] = piece;
}

/**
 * @brief Updates the board state by making a move.
 * @param board Pointer to the Board structure.
 * @param move The packed 32-bit move.
 * @return true if the move is legal, false if it leaves the king in check.
 */
bool make_move(Board *board, uint32_t move) {
    int from = GET_FROM(move);
    int to = GET_TO(move);
    int side = board->side;
    int piece = get_piece_at(board, from);
    int captured = GET_CAPTURED(move);

    board->history[board->hisply].move = move;
    board->history[board->hisply].castle = board->castle;
    board->history[board->hisply].enpassant = board->enpassant;
    board->history[board->hisply].fiftyMove = board->fiftyMove;
    board->history[board->hisply].posKey = board->posKey;

    if (move & MFLAG_CAP) {
        if (move & MFLAG_EP) {
            int ep_sq = (side == white) ? to - 8 : to + 8;
            remove_piece(board, ep_sq, (side == white) ? bp : wp);
        } else if (captured != EMPTY) {
            remove_piece(board, to, captured);
        }
        board->fiftyMove = 0;
    } else {
        board->fiftyMove++;
    }

    if (piece == wp || piece == bp) board->fiftyMove = 0;

    board->enpassant = NO_SQ;

    if (move & MFLAG_CA) {
        switch (to) {
            case g1: move_piece(board, h1, f1, wr); break;
            case c1: move_piece(board, a1, d1, wr); break;
            case g8: move_piece(board, h8, f8, br); break;
            case c8: move_piece(board, a8, d8, br); break;
        }
    }

    move_piece(board, from, to, piece);

    if (GET_PROMOTED(move) != EMPTY) {
        remove_piece(board, to, piece);
        add_piece(board, to, GET_PROMOTED(move));
    }

    if (move & MFLAG_PS) {
        board->enpassant = (side == white) ? from + 8 : from - 8;
    }

    if (piece == wk) board->castle &= ~(WKCA | WQCA);
    else if (piece == bk) board->castle &= ~(BKCA | BQCA);

    if (from == a1 || to == a1) board->castle &= ~WQCA;
    if (from == h1 || to == h1) board->castle &= ~WKCA;
    if (from == a8 || to == a8) board->castle &= ~BQCA;
    if (from == h8 || to == h8) board->castle &= ~BKCA;

    board->side ^= 1;
    board->ply++;
    board->hisply++;

    int king_sq = get_lsb(board->bitboards[(side == white) ? wk : bk]);
    if (is_square_attacked(board, king_sq, board->side)) {
        unmake_move(board);
        return false;
    }
    return true;
}

/**
 * @brief Reverts the board to the previous state using the history stack.
 * @param board Pointer to the Board structure.
 */
void unmake_move(Board *board) {
    board->hisply--;
    board->ply--;
    uint32_t move = board->history[board->hisply].move;
    int from = GET_FROM(move);
    int to = GET_TO(move);
    int side = board->side ^ 1;

    board->castle = board->history[board->hisply].castle;
    board->enpassant = board->history[board->hisply].enpassant;
    board->fiftyMove = board->history[board->hisply].fiftyMove;
    board->posKey = board->history[board->hisply].posKey;
    board->side = side;

    int piece = get_piece_at(board, to);

    if (GET_PROMOTED(move) != EMPTY) {
        remove_piece(board, to, piece);
        piece = (side == white) ? wp : bp;
        add_piece(board, to, piece);
    }

    move_piece(board, to, from, piece);

    if (move & MFLAG_CAP) {
        int captured = GET_CAPTURED(move);
        if (move & MFLAG_EP) {
            int ep_sq = (side == white) ? to - 8 : to + 8;
            add_piece(board, ep_sq, (side == white) ? bp : wp);
        } else {
            add_piece(board, to, captured);
        }
    }

    if (move & MFLAG_CA) {
        switch (to) {
            case g1: move_piece(board, f1, h1, wr); break;
            case c1: move_piece(board, d1, a1, wr); break;
            case g8: move_piece(board, f8, h8, br); break;
            case c8: move_piece(board, d8, a8, br); break;
        }
    }
}

/**
 * @brief Toggles the side to move without making a move on the board.
 * @param board Pointer to the Board structure.
 */
void make_null_move(Board *board) {
    board->history[board->hisply].move = 0;
    board->history[board->hisply].castle = board->castle;
    board->history[board->hisply].enpassant = board->enpassant;
    board->history[board->hisply].fiftyMove = board->fiftyMove;
    board->history[board->hisply].posKey = board->posKey;

    board->enpassant = NO_SQ;
    board->side ^= 1;
    board->hisply++;
    board->ply++;
}

/**
 * @brief Reverts a null move.
 * @param board Pointer to the Board structure.
 */
void unmake_null_move(Board *board) {
    board->hisply--;
    board->ply--;

    board->castle = board->history[board->hisply].castle;
    board->enpassant = board->history[board->hisply].enpassant;
    board->fiftyMove = board->history[board->hisply].fiftyMove;
    board->posKey = board->history[board->hisply].posKey;
    board->side ^= 1;
}
