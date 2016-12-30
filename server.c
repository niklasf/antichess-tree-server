#include <getopt.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>

#include "tree.h"

static int verbose = 0;  // --verbose
static int cors = 0;     // --cors

static int num_trees = 0;
static tree_t *forest;

void http_api(struct evhttp_request *req, void *data) {
}

int serve(int port) {
    struct event_base *base = event_base_new();
    if (!base) {
        printf("could not initialize event_base\n");
        abort();
    }

    struct evhttp *http = evhttp_new(base);
    if (!http) {
        printf("could not initialize evhttp\n");
        abort();
    }

    evhttp_set_gencb(http, http_api, NULL);

    struct evhttp_bound_socket *socket = evhttp_bind_socket_with_handle(http, "127.0.0.1", port);
    if (!socket) {
        printf("could not bind socket to http://127.0.0.1:%d\n", port);
        return 1;
    }

    printf("antichess solution server listening on http://127.0.0.1:%d ...\n", port);

    return event_base_dispatch(base);
}

int main(int argc, char *argv[]) {
    int port = 5004;

    static struct option long_options[] = {
        {"verbose", no_argument,       &verbose, 1},
        {"cors",    no_argument,       &cors, 1},
        {"port",    required_argument, 0, 'p'},
        {NULL, 0, 0, 0},
    };

    while (true) {
        int option_index;
        int opt = getopt_long(argc, argv, "p:", long_options, &option_index);
        if (opt < 0) break;

        switch (opt) {
        case 0:
            break;

        case 'p':
            port = atoi(optarg);
            if (!port) {
                printf("invalid port: %d\n", port);
                return 78;
            }
            break;

        case '?':
            return 78;

        default:
            printf("getopt error: %d\n", opt);
            abort();
        }
    }

    if (optind == argc) {
        printf("open at least one proof tree\n");
        return 78;
    }

    num_trees = argc - optind;
    forest = calloc(num_trees, sizeof(tree_t));

    for (int i = 0; i < num_trees; i++) {
        if (!tree_open(argv[optind], forest + i)) {
            printf("could not open %s: %s\n", argv[optind], strerror(errno));
            return 78;
        }

        if (verbose) {
            printf("%s:\n", argv[optind]);
            tree_debug(forest + i, false);
            printf("---\n");
        }

        optind++;
    }

    return serve(port);
}
