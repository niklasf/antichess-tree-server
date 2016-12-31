#ifndef SAN_H_
#define SAN_H_

#include <stdint.h>

static inline int popcount(uint64_t b) {
    return __builtin_popcountll(b);
}

typedef enum {
    kAll, kPawn, kKnight, kBishop, kRook, kQueen, kKing,
    kNone = 0
} piece_type_t;

typedef enum {
    kBlack, kWhite
} color_t;

typedef uint16_t move_t;

static const size_t MAX_UCI = 8;
void move_uci(move_t move, char *uci);
move_t move_parse(const char *uci);

typedef struct board {
    uint64_t occupied_co[2];
    uint64_t occupied[7];

    color_t turn;
    uint8_t ep_square;
} board_t;

static const size_t MAX_LEGAL_MOVES = 256;
static const size_t MAX_SAN = 8;
void board_reset(board_t *board);
void board_move(board_t *board, move_t move);
void board_san(board_t *board, move_t move, char *san);

#endif
