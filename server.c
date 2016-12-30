#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define popcnt __builtin_popcountll

#define ABS(x) (((x) >= 0) ? (x) : -(x))
#define SIGN(x) (((x) > 0) ? 1 : -1)

#define GET_BIT(A,x) (A[(x) >> 6] & (1ULL << ((x) & 0x3f)))
#define SET_BIT(A,x) A[(x) >> 6] |= (1ULL << ((x) & 0x3f))

uint32_t compute_hash(uint32_t n) {
    uint32_t k = n * n;
    k += (n >> 10) ^ ((~n) & 0x3ff);
    k += ((n >> 5) ^ ((~n) & 0x7fff)) << 5;
    k += (((~n) >> 15) ^ (n & 0x1f)) << 5;
    k += (n >> 4) & 0x55aa55;
    k += ((~n) >> 8) & 0xaa55aa;
    return k & 0xfffff;
}

typedef uint16_t move_t;

typedef struct position {
} position_t;

typedef struct node {
    uint32_t data;
    uint16_t move;
} __attribute__((packed)) node_t;

_Static_assert(sizeof(node_t) == 6, "node_t packed");

typedef struct counted {
    uint32_t n;
    int32_t size;
} counted_t;

struct tree_info {
    int fd;

    size_t prolog_len;
    move_t *prolog;

    size_t size;
    node_t *root;
    node_t *nodes;

    counted_t *counted;

    uint64_t *arr;
};

node_t *tree_node(const struct tree_info *tree, uint32_t n) {
    if (!n) return tree->root;
    else return tree->nodes + n - 1;
}

bool tree_is_trans(const struct tree_info *tree, uint32_t n) {
    return (tree_node(tree, n)->data & (1U << 31)) == (1U << 31);
}

bool tree_trans_and_sibling(const struct tree_info *tree, uint32_t n) {
    return (tree_node(tree, n)->data & (3U << 30)) == (3U << 30);
}

uint32_t tree_get_node(const struct tree_info *tree, uint32_t n) {
    return tree_node(tree, n)->data & 0x3fffffff;
}

uint32_t tree_get_node_ns(const struct tree_info *tree, uint32_t n) {
    node_t *node = tree_node(tree, n);
    if ((node->data & 0x3fffffff) != 0x3fffffff) return node->data & 0x3fffffff;
    else return ((node->data & (1U << 30)) == (1U << 30)) ? (n + 1) : 0;
}

bool tree_has_child(const struct tree_info *tree, uint32_t n) {
    node_t *node = tree_node(tree, n);
    return ((node->data & (3U << 30)) == (1U << 30)) && ((node->data & 0x3fffffff) != 0x3fffffff);
}

uint32_t tree_next_sibling(const struct tree_info *tree, uint32_t n) {
    if (tree_trans_and_sibling(tree, n)) return n + 1;
    else return tree_is_trans(tree, n) ? 0 : tree_get_node_ns(tree, n);
}

int32_t tree_subtree_count(const struct tree_info *tree, uint32_t n) {
    while (tree_is_trans(tree, n)) n = tree_get_node(tree, n);

    uint32_t bucket = compute_hash(n);
    while (tree->counted[bucket].n) {
        if (n == tree->counted[bucket].n) return tree->counted[bucket].size;
        bucket++;
        if (bucket == 0x100000) bucket = 0;
    }

    return 0;
}

void tree_do_subtree_count(struct tree_info *tree, uint32_t n, int32_t count) {
    while (tree_is_trans(tree, n)) n = tree_get_node(tree, n);

    uint32_t bucket = compute_hash(n);
    while (tree->counted[bucket].n) {
        bucket++;
        if (bucket == 0x100000) bucket = 0;
    }

    tree->counted[bucket].n = n;
    tree->counted[bucket].size = count;

    if (!tree_has_child(tree, n)) return;

    if (!tree_next_sibling(tree, n + 1))
        tree_do_subtree_count(tree, n + 1, (ABS(count) - 1) * SIGN(count));
}

bool tree_open(const char *filename, struct tree_info *tree) {
    tree->fd = open(filename, O_RDONLY);
    if (tree->fd == -1) return false;

    struct stat sb;
    if (fstat(tree->fd, &sb) == -1) return false;

    tree->root = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, tree->fd, 0);
    if (tree->root == MAP_FAILED) return false;

    tree->prolog_len = tree->root->move;
    tree->prolog = (move_t *)(tree->root + 1);
    tree->nodes = (node_t *)(tree->prolog + tree->prolog_len);

    tree->size = tree_get_node(tree, 0);
    if (!tree->size) return false;

    tree->arr = calloc(tree->size + 1, sizeof(uint64_t));
    if (!tree->arr) return false;

    tree->counted = calloc(0x100000, sizeof(counted_t));
    if (!tree->counted) return false;

    uint32_t *data = (uint32_t *)(tree->nodes + tree->size - 1);
    while ((uint8_t *) data < ((uint8_t *) tree->root) + sb.st_size) {
        uint32_t node = *data++;
        int32_t size = *((int32_t *) data++);
        if (!tree_subtree_count(tree, node)) tree_do_subtree_count(tree, node, size);
    }

    return true;
}

uint32_t tree_move(const struct tree_info *tree, move_t move, uint32_t n) {
    if (n == -1) return -1;
    if (!tree_has_child(tree, n)) return -1;

    uint32_t child = n + 1;

    do {
        if (tree_node(tree, child)->move == move) {
            while (tree_is_trans(tree, child)) child = tree_get_node(tree, child);
            return child;
        }
    } while ((child = tree_next_sibling(tree, child)));

    return -1;
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

    const char promotions[] = "__nbrqk_";

    if (move & (7 << 12)) sprintf(uci + 4, "%c", promotions[move >> 12]);
}

void tree_debug(const struct tree_info *tree) {
    printf("tree size = %zu (%zumb) \n", tree->size, (sizeof(node_t) * tree->size >> 20));

    for (size_t i = 0; i < tree->prolog_len; i++) {
        char uci[8];
        move_to_uci(tree->prolog[i], uci);
        printf("prolog[%zu] = %s\n", i, uci);
    }

    for (size_t i = 0; i < 0x100000; i++) {
        if (tree->counted[i].n) printf("counted[%zu] = <%d, %d>\n", i, tree->counted[i].n, tree->counted[i].size);
    }
}

struct query_result {
    move_t move;
    uint32_t n;
    int32_t size;
};

bool VV(move_t move) {
    uint16_t k = move >> 12;
    return move == 0xfedc || (k != 7 && k < 9);
}

bool tree_is_unsolved(const struct tree_info *tree, uint32_t n) {
    if (tree_get_node(tree, n) == 0x3fffffff) return true;
    return !VV(tree_node(tree, n)->move);
}

void tree_walk(struct tree_info *tree, uint32_t n, bool Transpositions, bool *unwon) {
    if (tree_is_unsolved(tree, n)) {
        *unwon = true;
        return;
    }

    if (Transpositions) {
        if (GET_BIT(tree->arr, n)) return;
        SET_BIT(tree->arr, n);
    }

    if (tree_is_trans(tree, n)) {
        tree_walk(tree, tree_get_node(tree, n), Transpositions, unwon);
        return;
    }

    if (!Transpositions) {
        if (GET_BIT(tree->arr, n)) return;
        SET_BIT(tree->arr, n);
    }

    if (!tree_has_child(tree, n)) return;

    uint32_t child = n + 1;
    do {
        tree_walk(tree, child, Transpositions, unwon);
    } while ((child = tree_next_sibling(tree, child)));
}

uint32_t tree_subtree_size(const struct tree_info *tree, uint32_t n) {
    uint32_t sz = tree->size;
    if (!n) return sz;

    sz = (sz + 63) / 64;
    if (tree_is_unsolved(tree, n)) return -1;

    while (tree_is_trans(tree, n)) n = tree_get_node(tree, n);

    uint32_t s = tree_subtree_count(tree, n);
    if (s) return s;

    bool unwon = false;
    memset(tree->arr, 0, sizeof(uint64_t) * sz);

    tree_walk(tree, n, false, &unwon);

    uint32_t sum = 0;
    for (size_t i = 0; i < sz; i++) {
        sum += popcnt(tree->arr[i]);
    }

    if (unwon) sum = -sum;

    tree_do_subtree_count(tree, n, sum);
    return sum;
}

size_t tree_dump_children(const struct tree_info *tree, uint32_t n) {
    size_t num_children = 0;
    struct query_result result[256];

    uint32_t child = n + 1;

    do {
        node_t *node = tree_node(tree, child);
        result[num_children].move = node->move;
        result[num_children].n = child;
        result[num_children].size = tree_subtree_size(tree, child);
        num_children++;
    } while ((child = tree_next_sibling(tree, child)) && num_children < 256);

    for (int i = 0; i < num_children; i++) {
        char uci[8];
        move_to_uci(result[i].move, uci);
        printf("  %s -> %d\n", uci, result[i].size);
    }

    return num_children;
}

void tree_query(const struct tree_info *tree) {
    uint32_t n = tree_move(tree, 3104, 0);
    //while (tree_is_trans(tree, n)) n = tree_get_node(tree, n);

    if (n != -1 && tree_has_child(tree, n)) {
        printf("output children\n");
        tree_dump_children(tree, n);
    } else {
        printf("no children\n");
    }
}


int main(int argc, char *argv[]) {
    struct tree_info tree;
    if (!tree_open(argv[1], &tree)) {
        printf("could not open proof tree: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    tree_debug(&tree);

    //align_node(tree, NULL);
    tree_query(&tree);

    return EXIT_SUCCESS;
}
