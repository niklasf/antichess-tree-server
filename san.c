#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "san.h"

#define BB_SQUARE(sq) (1ULL << (sq))

static inline int square_file(uint8_t square) {
    return square & 7;
}

static inline int square_rank(uint8_t square) {
    return square >> 3;
}

static int square_distance(uint8_t a, uint8_t b) {
    int rd = abs(square_rank(a) - square_rank(b));
    int fd = abs(square_file(a) - square_file(b));
    return (rd > fd) ? rd : fd;
}

const static int ROOK_DELTAS[] = { 8, 1, -8, -1, 0 };
const static int BISHOP_DELTAS[] = { 9, -9, 7, -7, 0 };
const static int KING_DELTAS[] = { 8, 1, -8, -1, 9, -9, 7, -7, 0 };
const static int KNIGHT_DELTAS[] = { 17, 15, 10, 6, -6, -10, -15, -17, 0 };

static uint64_t attacks_sliding(const int deltas[], uint8_t square, uint64_t occupied) {
    uint64_t attack = 0;

    for (int i = 0; deltas[i]; i++) {
        for (int s = square + deltas[i];
             s >= 0 && s < 64 && square_distance(s, s - deltas[i]) <= 2;
             s += deltas[i])
        {
            attack |= BB_SQUARE(s);
            if (occupied & BB_SQUARE(s)) break;
        }
    }

    return attack;
}

static inline uint8_t move_from(move_t move) {
    return (move >> 6) & 077;
}

static inline uint8_t move_to(move_t move) {
    return move & 077;
}

static piece_type_t move_promotion(move_t move) {
    static const piece_type_t promotions[] = { kNone, kNone, kKnight, kBishop, kRook, kQueen, kKing, kNone };
    return promotions[move >> 12];
}

void move_uci(move_t move, char *uci) {
    if (!move) {
        sprintf(uci, "(none)");
        return;
    }

    int from = move_from(move);
    int to = move_to(move);

    const char promotions[] = "\0nbrqk";

    sprintf(uci, "%c%c%c%c%c", 'a' + square_file(from), '1' + square_rank(from),
                               'a' + square_file(to),   '1' + square_rank(to),
                               promotions[move_promotion(move)]);
}

move_t move_parse(const char *uci) {
    const char promotions[] = "\0pnbrqk";

    if (strlen(uci) > 5 || strlen(uci) < 4) return 0;

    move_t move = (uci[2] - 'a') + ((uci[3] - '1') << 3) + ((uci[0] - 'a') << 6) + ((uci[1] - '1') << 9);

    if (uci[4]) {
        for (int k = 2; k <= 6; k++) {
            if (uci[4] == promotions[k]) {
                move |= k << 12;
                return move;
            }
        }

        return 0;
    }

    return move;
}

void board_reset(board_t *board) {
    board->occupied_co[kWhite] = 0xffffULL;
    board->occupied_co[kBlack] = 0xffff000000000000ULL;

    board->occupied[kAll] = 0xffff00000000ffffULL;
    board->occupied[kPawn] = 0xff00000000ff00ULL;
    board->occupied[kKnight] = 0x4200000000000042ULL;
    board->occupied[kBishop] = 0x2400000000000024ULL;
    board->occupied[kRook] = 0x8100000000000081ULL;
    board->occupied[kQueen] = 0x800000000000008ULL;
    board->occupied[kKing] = 0x1000000000000010ULL;

    board->turn = kWhite;
    board->ep_square = 0;
}

piece_type_t board_piece_type_at(const board_t *board, uint8_t square) {
    uint64_t bb = BB_SQUARE(square);
    for (piece_type_t pt = kPawn; pt <= kKing; pt++) {
        if (board->occupied[pt] & bb) return pt;
    }

    return kNone;
}

void board_move(board_t *board, move_t move) {
    if (!move) return;

    board->ep_square = 0;

    uint8_t from = move_from(move);
    uint8_t to = move_to(move);
    piece_type_t pt = board_piece_type_at(board, move_from(move));
    if (!pt) return;

    piece_type_t capture = board_piece_type_at(board, move_to(move));

    board->occupied_co[board->turn] &= ~BB_SQUARE(from);
    board->occupied[kAll] &= ~BB_SQUARE(from);
    board->occupied[pt] &= ~BB_SQUARE(from);

    if (capture) {
        board->occupied_co[!board->turn] &= ~BB_SQUARE(to);
        board->occupied[pt] &= ~BB_SQUARE(to);
    }

    if (pt == kPawn) {
        if (square_file(from) != square_file(to) && !capture) {
            uint64_t ep_mask = BB_SQUARE(to + (board->turn ? -8 : 8));
            board->occupied_co[!board->turn] &= ~ep_mask;
            board->occupied[kAll] &= ~ep_mask;
            board->occupied[kPawn] &= ~ep_mask;
        } else if (square_distance(from, to) == 2) {
            board->ep_square = from + (board->turn ? 8 : -8);
        }
    }

    if (move_promotion(move)) pt = move_promotion(move);
    board->occupied_co[board->turn] |= BB_SQUARE(to);
    board->occupied[kAll] |= BB_SQUARE(to);
    board->occupied[pt] |= BB_SQUARE(to);

    board->turn = !board->turn;
}

void board_san(board_t *board, move_t move, char *san) {
    move_uci(move, san);
}
