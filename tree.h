#ifndef TREE_H_
#define TREE_H_

#include <stdint.h>
#include <stdbool.h>

typedef uint16_t move_t;

typedef struct node {
    uint32_t data;
    uint16_t move;
} __attribute__((packed)) node_t;

_Static_assert(sizeof(node_t) == 6, "node_t packed");

typedef struct hash_entry {
    uint32_t index;
    uint32_t size;
} hash_entry_t;

static const size_t HASHTABLE_LEN = 0x100000;

typedef struct tree {
    int fd;

    uint32_t prolog_len;
    move_t *prolog;

    uint32_t size;
    node_t *root;
    node_t *nodes;

    hash_entry_t *hashtable;
    size_t num_hash_entries;

    uint64_t *arr;
} tree_t;

static const size_t MAX_RESULTS = 256;

typedef struct query_result {
    move_t move;
    uint32_t size;
} query_result_t;

static const size_t MAX_UCI = 8;
void move_to_uci(uint16_t move, char *uci);

bool node_has_child(const node_t *node);

bool tree_open(const char *filename, tree_t *tree);
void tree_debug(const tree_t *tree, bool dump_hashtable);
const node_t *tree_move(const tree_t *tree, move_t move, const node_t *node);
size_t tree_query(tree_t *tree, const node_t *node, query_result_t *results, size_t num_children);

#endif  // #ifndef TREE_H_
