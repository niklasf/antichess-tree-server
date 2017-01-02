#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "san.h"

void test_san() {
    char san[MAX_SAN];
    board_t board;
    board_reset(&board);

    board_move(&board, move_parse("e2e3"));
    board_move(&board, move_parse("b7b6"));
    board_move(&board, move_parse("a2a4"));
    board_move(&board, move_parse("c8a6"));

    move_t Bxa6 = move_parse("f1a6");
    board_san(&board, Bxa6, san);
    assert(strcmp(san, "Bxa6") == 0);

    board_move(&board, Bxa6);
    board_move(&board, move_parse("b8a6"));
    board_move(&board, move_parse("b2b4"));
    board_move(&board, move_parse("a6b4"));
    board_move(&board, move_parse("d1h5"));

    move_t Nxc2 = move_parse("b4c2");
    board_san(&board, Nxc2, san);
    assert(strcmp(san, "Nxc2") == 0);
}

int main() {
    test_san();
    return 0;
}
