#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "tree.h"

static inline int popcount(uint64_t b) {
    return __builtin_popcountll(b);
}

static inline bool arr_get_bit(const uint64_t *arr, size_t n) {
    return !!(arr[n >> 6] & (1ULL << (n & 0x3f)));
}

static inline void arr_set_bit(uint64_t *arr, size_t n) {
    arr[n >> 6] |= (1ULL << (n & 0x3f));
}

static uint32_t compute_hash(uint32_t n) {
    uint32_t k = n * n;
    k += (n >> 10) ^ ((~n) & 0x3ff);
    k += ((n >> 5) ^ ((~n) & 0x7fff)) << 5;
    k += (((~n) >> 15) ^ (n & 0x1f)) << 5;
    k += (n >> 4) & 0x55aa55;
    k += ((~n) >> 8) & 0xaa55aa;
    return k & 0xfffff;
}

static const node_t *tree_next(const tree_t *tree, const node_t *node) {
    if (tree->root == node) return tree->nodes;
    else return node + 1;
}

static node_t *tree_from_index(const tree_t *tree, uint32_t index) {
    if (!index) return NULL;
    else return tree->nodes + index - 1;
}

static uint32_t tree_index(const tree_t *tree, const node_t *node) {
    if (tree->root == node) return 0;
    else return node - tree->nodes + 1;
}

static bool node_is_trans(const node_t *node) {
    return (node->data & (1U << 31)) == (1U << 31);
}

static bool node_trans_and_sibling(const node_t *node) {
    return (node->data & (3U << 30)) == (3U << 30);
}

static uint32_t node_trans_index(const node_t *node) {
    return node->data & 0x3fffffff;
}

bool node_has_child(const node_t *node) {
    return ((node->data & (3U << 30)) == (1U << 30)) && ((node->data & 0x3fffffff) != 0x3fffffff);
}

static const node_t *tree_trans(const tree_t *tree, const node_t *node) {
    return tree_from_index(tree, node_trans_index(node));
}

static const node_t *tree_trans_ns(const tree_t *tree, const node_t *node) {
    if ((node->data & 0x3fffffff) != 0x3fffffff) return tree_from_index(tree, node->data & 0x3fffffff);
    else return ((node->data & (1U << 30)) == (1U << 30)) ? tree_next(tree, node) : NULL;
}

static const node_t *tree_next_sibling(const tree_t *tree, const node_t *node) {
    if (node_trans_and_sibling(node)) return tree_next(tree, node);
    else return node_is_trans(node) ? NULL : tree_trans_ns(tree, node);
}

static uint32_t tree_lookup_subtree_size(const tree_t *tree, const node_t *node) {
    while (node_is_trans(node)) node = tree_trans(tree, node);

    uint32_t index = tree_index(tree, node);
    uint32_t bucket = compute_hash(index);
    while (tree->hashtable[bucket].index) {
        if (index == tree->hashtable[bucket].index) return tree->hashtable[bucket].size;
        bucket++;
        if (bucket == hashtable_len) bucket = 0;
    }

    return 0;
}

static bool tree_save_subtree_size(tree_t *tree, const node_t *node, uint32_t size) {
    if (tree->num_hash_entries > hashtable_len / 8) {
        // Do not fill table too much.
        return false;
    }

    while (node_is_trans(node)) node = tree_trans(tree, node);

    uint32_t bucket = compute_hash(tree_index(tree, node));
    while (tree->hashtable[bucket].index) {
        bucket++;
        if (bucket == hashtable_len) bucket = 0;
    }

    tree->hashtable[bucket].index = tree_index(tree, node);
    tree->hashtable[bucket].size = size;
    tree->num_hash_entries++;

    if (!node_has_child(node)) return true;

    if (!tree_next_sibling(tree, tree_next(tree, node)))
        return tree_save_subtree_size(tree, tree_next(tree, node), (size > 0) ? (size - 1) : 0);

    return true;
}

bool tree_open(const char *filename, tree_t *tree) {
    tree->fd = open(filename, O_RDONLY);
    if (tree->fd == -1) return false;

    struct stat sb;
    if (fstat(tree->fd, &sb) == -1) return false;

    tree->root = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, tree->fd, 0);
    if (tree->root == MAP_FAILED) return false;

    tree->prolog_len = tree->root->move;
    tree->prolog = (move_t *)(tree->root + 1);
    tree->nodes = (node_t *)(tree->prolog + tree->prolog_len);

    tree->size = node_trans_index(tree->root);
    if (!tree->size) return false;

    tree->arr = calloc(tree->size / 8 + 64, 1);
    if (!tree->arr) return false;

    tree->num_hash_entries = 0;
    tree->hashtable = calloc(hashtable_len, sizeof(hash_entry_t));
    if (!tree->hashtable) return false;

    uint32_t *data = (uint32_t *)(tree->nodes + tree->size - 1);
    while ((uint8_t *) data < ((uint8_t *) tree->root) + sb.st_size) {
        node_t *node = tree_from_index(tree, *data++);
        if (!node) node = tree->root;
        uint32_t size = *data++;
        if (!tree_lookup_subtree_size(tree, node)) {
            bool success = tree_save_subtree_size(tree, node, size);
            assert(success);
        }
    }

    return true;
}

const node_t *tree_move(const tree_t *tree, move_t move, const node_t *node) {
    if (!node) return NULL;
    if (!node_has_child(node)) return NULL;

    const node_t *child = tree_next(tree, node);

    do {
        if (child->move == move) {
            while (node_is_trans(child)) child = tree_trans(tree, child);
            return child;
        }
    } while ((child = tree_next_sibling(tree, child)));

    return NULL;
}

void move_to_uci(uint16_t move, char *uci) {
    if (move == 0xfedc || move == 0xedc) {
        // d7 to e3
        sprintf(uci, "(none)");
        return;
    }

    int from = (move >> 6) & 077;
    int to = move & 077;

    // hack for unsolved proof
    if ((move >> 12) > 8 || (move >> 12) == 7) move ^= 0xf000;

    sprintf(uci, "%c%c%c%c", 'a' + (from & 7), '1' + (from >> 3),
                             'a' + (to   & 7), '1' + (to   >> 3));

    const char promotions[] = "\0\0nbrqk\0";

    if (move & (7 << 12)) sprintf(uci + 4, "%c", promotions[move >> 12]);
}

void tree_debug(const tree_t *tree, bool dump_hashtable) {
    printf("tree size = %u (%zumb) \n", tree->size, (sizeof(node_t) * tree->size >> 20));

    for (size_t i = 0; i < tree->prolog_len; i++) {
        char uci[8];
        move_to_uci(tree->prolog[i], uci);
        printf("prolog[%zu] = %s\n", i, uci);
    }

    if (dump_hashtable) {
        for (size_t i = 0; i < hashtable_len; i++) {
            if (tree->hashtable[i].index) printf("hashtable[%zu] = <%d, %d>\n", i, tree->hashtable[i].index, tree->hashtable[i].size);
        }
    }
}

static int query_result_cmp(const void *a, const void *b) {
    const query_result_t *ra = a, *rb = b;
    return (ra->size > rb->size) ? -1 : (ra->size < rb->size);
}

static void tree_walk(tree_t *tree, const node_t *node, bool transpositions) {
    uint32_t index = tree_index(tree, node);
    uint16_t k = node->move >> 12;
    assert(node_trans_index(node) != 0x3fffffff);
    assert(node->move == 0xfedc || (k != 7 && k < 9));

    if (transpositions) {
        if (arr_get_bit(tree->arr, index)) return;
        arr_set_bit(tree->arr, index);
    }

    if (node_is_trans(node)) {
        tree_walk(tree, tree_trans(tree, node), transpositions);
        return;
    }

    if (!transpositions) {
        if (arr_get_bit(tree->arr, index)) return;
        arr_set_bit(tree->arr, index);
    }

    if (!node_has_child(node)) return;

    const node_t *child = tree_next(tree, node);
    do {
        tree_walk(tree, child, transpositions);
    } while ((child = tree_next_sibling(tree, child)));
}

static uint32_t tree_subtree_size(tree_t *tree, const node_t *node) {
    if (tree->root == node) return tree->size;

    uint16_t k = node->move >> 12;
    assert(node_trans_index(node) != 0x3fffffff);
    assert(node->move == 0xfedc || (k != 7 && k < 9));

    while (node_is_trans(node)) node = tree_trans(tree, node);

    uint32_t subtree_size = tree_lookup_subtree_size(tree, node);
    if (subtree_size) return subtree_size;

    uint32_t size = (tree->size + 63) / 64;
    memset(tree->arr, 0, sizeof(uint64_t) * size);
    tree_walk(tree, node, false);

    for (uint32_t i = 0; i < size; i++) {
        subtree_size += popcount(tree->arr[i]);
    }

    tree_save_subtree_size(tree, node, subtree_size);
    return subtree_size;
}

size_t tree_query(tree_t *tree, const node_t *node, query_result_t *results, size_t num_children) {
    assert(node);

    if (!node_has_child(node)) return num_children;

    const node_t *child = tree_next(tree, node);

    do {
        for (size_t i = 0; i < max_results; i++) {
            if (results[i].move == 0) {
                results[i].move = child->move;
                results[i].size = tree_subtree_size(tree, child);
                num_children++;
                break;
            } else if (results[i].move == child->move) {
                results[i].size += tree_subtree_size(tree, child);
                break;
            }
        }
    } while ((child = tree_next_sibling(tree, child)));

    qsort(results, num_children, sizeof(query_result_t), query_result_cmp);

    return num_children;
}

int main(int argc, char *argv[]) {
    tree_t tree;
    if (!tree_open(argv[1], &tree)) {
        printf("could not open proof tree: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    tree_debug(&tree, false);

    query_result_t results[max_results] = { { 0 } };
    size_t num_children = tree_query(&tree, tree.root, results, 0);
    if (!num_children) {
        printf("no children!\n");
    }

    for (size_t i = 0; i < num_children; i++) {
        char uci[8];
        move_to_uci(results[i].move, uci);
        printf("  %s -> %d\n", uci, results[i].size);
    }

    return EXIT_SUCCESS;
}
