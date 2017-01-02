#include <assert.h>
#include <string.h>

#include "san.h"

void test_san() {
    board_t board;
    board_reset(&board);

    board_move(&board, move_parse("e2e3"));
    board_move(&board, move_parse("b7b6"));
    board_move(&board, move_parse("a2a4"));
    board_move(&board, move_parse("c8a6"));

    char san[MAX_SAN];
    move_t move = move_parse("f1a6");
    board_san(&board, move, san);
    assert(strcmp(san, "Bxa6") == 0);
}

int main() {
    test_san();
    return 0;
}
