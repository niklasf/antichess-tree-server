#ifndef SAN_H_
#define SAN_H_

#include <stdint.h>

static inline int popcount(uint64_t b) {
    return __builtin_popcountll(b);
}

typedef uint16_t move_t;

static const size_t MAX_UCI = 8;
void move_uci(move_t move, char *uci);
move_t move_parse(const char *uci);

#endif
