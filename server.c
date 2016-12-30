#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define ABS(x) (((x) >= 0) ? (x) : -(x))
#define SIGN(x) (((x) > 0) ? 1 : -1)

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
} __attribute__((packed)) counted_t;

_Static_assert(sizeof(counted_t) == 8, "counted_t packed");

struct tree_info {
    int fd;

    size_t prolog_len;
    move_t *prolog;

    size_t size;
    node_t *root;
    node_t *nodes;

    counted_t *counted;
};

node_t *tree_node(const struct tree_info *tree, uint32_t n) {
    if (!n) return tree->root;
    else return tree->nodes + n;
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
    else return ((node->data & (1U << 30)) == (1U << 30)) ? n + 1 : 0;
}

bool tree_has_child(const struct tree_info *tree, uint32_t n) {
    node_t *node = tree_node(tree, n);
    return ((node->data & (3U << 30)) == (1U << 30)) && ((node->data & 0x3fffffff) != 0x3fffffff);
}

uint32_t tree_next_sibling(const struct tree_info *tree, uint32_t n) {
    if (tree_trans_and_sibling(tree, n)) return n + 1;
    else return tree_is_trans(tree, n) ? 0 : tree_get_node_ns(tree, n);
}

int32_t tree_subtree_count(struct tree_info *tree, uint32_t n) {
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
    if (!tree_next_sibling(tree, n + 1)) tree_do_subtree_count(tree, n + 1, (ABS(count) - 1) * SIGN(count));
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

    tree->counted = calloc(0x100000, sizeof(counted_t));
    if (!tree->counted) return false;

    int i = 0;
    uint8_t *data = (uint8_t *)(tree->nodes + tree->size - 1);
    while (data < ((uint8_t *) tree->root) + sb.st_size) {
        uint32_t node = *((uint32_t *) data);
        data += 4;
        int32_t size = *((int32_t *) data);
        data += 4;

        if (!tree_subtree_count(tree, node)) tree_do_subtree_count(tree, node, size);
        printf("%d %d\n", node, size);
        i++;
    }

    printf("i = %d\n", i);

    return true;
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
}


int main(int argc, char *argv[]) {
    struct tree_info tree;
    if (!tree_open(argv[1], &tree)) {
        printf("could not open proof tree: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    tree_debug(&tree);

    return EXIT_SUCCESS;
}
