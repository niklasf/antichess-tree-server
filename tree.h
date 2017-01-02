#ifndef TREE_H_
#define TREE_H_

#include <stdint.h>
#include <stdbool.h>

#include "san.h"

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

    node_t *root;
    size_t num_pages;
    uint32_t size;

    node_t *nodes;

    hash_entry_t *hashtable;
    size_t num_hash_entries;

    uint64_t *arr;
} tree_t;

#define MAX_LEGAL_MOVES 256

typedef struct query_result {
    move_t moves[MAX_LEGAL_MOVES];
    uint32_t sizes[MAX_LEGAL_MOVES];
    size_t num_children;
} query_result_t;

void query_result_clear(query_result_t *result);
void query_result_sort(query_result_t *result);

bool tree_open(tree_t *tree, const char *filename);
void tree_debug(const tree_t *tree, bool dump_hashtable);
bool tree_query(tree_t *tree, const move_t *moves, size_t moves_len, query_result_t *result);
void tree_close(tree_t *tree);

#endif  // #ifndef TREE_H_
