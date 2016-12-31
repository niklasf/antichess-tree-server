#include <stdio.h>

#include "san.h"

void move_uci(uint16_t move, char *uci) {
    if (!move) {
        sprintf(uci, "(none)");
        return;
    }

    int from = (move >> 6) & 077;
    int to = move & 077;

    sprintf(uci, "%c%c%c%c", 'a' + (from & 7), '1' + (from >> 3),
                             'a' + (to   & 7), '1' + (to   >> 3));

    const char promotions[] = "\0\0nbrqk\0";

    if (move & (7 << 12)) sprintf(uci + 4, "%c", promotions[move >> 12]);
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
