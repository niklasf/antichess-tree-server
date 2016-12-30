#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/mman.h>

typedef uint16_t move_t;

typedef struct position {
} position_t;

typedef struct win_node {
    uint32_t data;
    uint16_t move;
} __attribute__((packed)) win_node_t;

_Static_assert(sizeof(win_node_t) == 6, "win_node_t packed");

bool win_node_is_trans(const win_node_t *node) {
    return !!(node->data & (1U << 31));
}

typedef struct counted {
    uint32_t node;
    int32_t size;
} __attribute__((packed)) counted_t;

_Static_assert(sizeof(counted_t) == 8, "counted_t packed");

struct tree_info {
    int fd;

    size_t prolog_len;
    move_t *prolog;

    size_t size;
    win_node_t *root;
    win_node_t *nodes;

    counted_t *counted;
};

bool tree_open(const char *filename, struct tree_info *tree) {
    tree->fd = open(filename, O_RDONLY);
    if (tree->fd == -1) return false;

    struct stat sb;
    if (fstat(tree->fd, &sb) == -1) return false;

    tree->root = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, tree->fd, 0);
    if (tree->root == MAP_FAILED) return false;

    tree->prolog_len = tree->root->move;
    tree->prolog = (move_t *)(tree->root + 1);

    tree->size = tree->root->data & 0x3fffffff;
    if (!tree->size) return false;
    tree->nodes = (win_node_t *)(tree->prolog + tree->prolog_len);

    tree->counted = calloc(0x100000, sizeof(counted_t));
    if (!tree->counted) return false;

    int i = 0;
    uint8_t *data = (uint8_t *)(tree->nodes + tree->size - 1);
    while (data < ((uint8_t *) tree->root) + sb.st_size) {
        uint32_t node = *((uint32_t *) data);
        data += 4;
        int32_t size = *((int32_t *) data);
        data += 4;
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
    printf("tree size = %zu (%zumb) \n", tree->size, (sizeof(win_node_t) * tree->size >> 20));

    for (size_t i = 0; i < tree->prolog_len; i++) {
        char uci[8];
        move_to_uci(tree->prolog[i], uci);
        printf("prolog[%zu] = %s\n", i, uci);
    }
}

/*int32_t tree_subtree_count(const struct tree_info *tree) {
    while (win_node_is_trans(tree->root[
}*/

int main(int argc, char *argv[]) {
    struct tree_info tree;
    if (!tree_open(argv[1], &tree)) {
        printf("could not open proof tree: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    tree_debug(&tree);

    return EXIT_SUCCESS;
}
