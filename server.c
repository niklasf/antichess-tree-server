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

static const size_t MAX_MOVES = 512;

static int verbose = 0;  // --verbose
static int cors = 0;     // --cors

static int num_trees = 0;
static tree_t *forest;

void http_api(struct evhttp_request *req, void *data) {
    const char *uri = evhttp_request_get_uri(req);
    if (!uri) {
        printf("evhttp_request_get_uri failed\n");
        return;
    }

    struct evkeyvalq *headers = evhttp_request_get_output_headers(req);
    if (cors) {
        // TODO: Preflight
        evhttp_add_header(headers, "Access-Control-Allow-Origin", "*");
    }

    struct evkeyvalq query;
    char *move_buf = NULL;
    if (0 == evhttp_parse_query(uri, &query)) {
        if (evhttp_find_header(&query, "moves")) {
            move_buf = strdup(evhttp_find_header(&query, "moves"));
        }
    }

    if (!move_buf) {
        struct evbuffer *buf = evhttp_request_get_input_buffer(req);
        size_t body_len = evbuffer_get_length(buf);
        if (body_len > MAX_MOVES * (MAX_UCI + 1)) body_len = MAX_MOVES * (MAX_UCI + 1);
        move_buf = malloc(body_len + 1);
        move_buf[body_len] = 0;
        evbuffer_copyout(buf, move_buf, body_len);
    }

    if (!move_buf) {
        move_buf = strdup("");
    }

    move_t moves[MAX_MOVES];
    size_t num_moves = 0;

    char *token = strtok(move_buf, " ");
    while (token && num_moves < MAX_MOVES) {
        move_t move = move_parse(token);
        if (!move) {
            free(move_buf);
            evhttp_send_error(req, HTTP_BADREQUEST, "Invalid move");
            return;
        }

        moves[num_moves++] = move;

        token = strtok(NULL, " ");
    }

    free(move_buf);

    if (num_moves >= MAX_MOVES) {
        evhttp_send_error(req, HTTP_BADREQUEST, "Too many moves");
        return;
    }

    query_result_t results[MAX_RESULTS] = { { 0 } };
    size_t num_children = 0;

    for (int i = 0; i < num_trees; i++) {
        tree_t *tree = forest + i;

        bool prolog_matches = true;
        if (tree->prolog_len > num_moves) prolog_matches = false;
        for (int j = 0; j < tree->prolog_len && prolog_matches; j++) {
            if (tree->prolog[j] != moves[j]) prolog_matches = false;
        }

        if (!prolog_matches) continue;

        const node_t *node = tree->root;
        for (int j = tree->prolog_len; j < num_moves; j++) {
            node = tree_move(tree, moves[j], node);
            if (!node) break;
        }

        if (node) {
            num_children = tree_query(tree, node, results, num_children);
        }
    }

    evhttp_add_header(headers, "Content-Type", "application/json");

    struct evbuffer *res = evbuffer_new();
    if (!res) {
        printf("could not allocate response buffer\n");
        abort();
    }

    evbuffer_add_printf(res, "{\n");
    evbuffer_add_printf(res, "  moves: [\n");
    for (size_t i = 0; i < num_children; i++) {
        char uci[MAX_UCI];
        move_to_uci(results[i].move, uci);
        evbuffer_add_printf(res, "    {\"uci\": \"%s\", \"nodes\": %d}%s\n", uci, results[i].size, (i < num_children - 1) ? "," : "");
    }
    evbuffer_add_printf(res, "  ]\n");
    evbuffer_add_printf(res, "}\n");

    evhttp_send_reply(req, HTTP_OK, "OK", res);

    evbuffer_free(res);
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
