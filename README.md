Antichess solution server
=========================

HTTP API and C library to query [Watkins antichess proof tables](http://magma.maths.usyd.edu.au/~watkins/LOSING_CHESS/index.html).
The decompressed proof trees (2.9 GB total) are memory
mapped allowing the server to run on systems with as little as 128 MB RAM.
Extra RAM may be used by the operating systems page cache to speed up lookups.

Building
--------

Install libevent2:

    apt-get install build-essential libevent-dev

Then:

    make

Usage
-----

[Download](http://magma.maths.usyd.edu.au/~watkins/LOSING_CHESS/index.html)
and `bunzip2` the proof trees:

    wget -i MIRROR.txt
    bunzip *.bz2

Then start the server:

    ./antichess-tree-server
        [--verbose] [--cors] [--port 5004]
        e3b5.rev e3b6.noe6.rev e3c5.rev e3c6.rev e3e6.rev e3g5.rev e3Nc6.rev e3Nh6.rev easy12.rev

HTTP API
--------

CORS enabled if `--cors` was given. Provide `moves` as a GET parameter or as
plain text in the POST body. Space or comma seperated.

### `GET|POST /`

```
> curl https://tablebase.lichess.org/watkins --data e2e3,c7c5,f1b5
```

```javascript
{
  "game_over": false,
  "moves": [
    {"uci": "c5c4", "san": "c4", "nodes": 103184905},
    {"uci": "g8h6", "san": "Nh6", "nodes": 43560428},
    {"uci": "d8c7", "san": "Qc7", "nodes": 36740518},
    {"uci": "g7g5", "san": "g5", "nodes": 17378970},
    {"uci": "b7b6", "san": "b6", "nodes": 6948918},
    {"uci": "a7a5", "san": "a5", "nodes": 2632138},
    {"uci": "e7e6", "san": "e6", "nodes": 1682781},
    {"uci": "d8a5", "san": "Qa5", "nodes": 1584849},
    {"uci": "f7f6", "san": "f6", "nodes": 1095995},
    {"uci": "h7h5", "san": "h5", "nodes": 850730},
    {"uci": "b8c6", "san": "Nc6", "nodes": 687137},
    {"uci": "g8f6", "san": "Nf6", "nodes": 330309},
    {"uci": "e7e5", "san": "e5", "nodes": 294172},
    {"uci": "b8a6", "san": "Na6", "nodes": 262343},
    {"uci": "g7g6", "san": "g6", "nodes": 251022},
    {"uci": "f7f5", "san": "f5", "nodes": 153818},
    {"uci": "h7h6", "san": "h6", "nodes": 124791},
    {"uci": "a7a6", "san": "a6", "nodes": 21336},
    {"uci": "d8b6", "san": "Qb6", "nodes": 3078},
    {"uci": "d7d5", "san": "d5", "nodes": 33},
    {"uci": "d7d6", "san": "d6", "nodes": 31}
  ]
}
```

C API
-----

```c
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "tree.h"

int main() {
    tree_t tree;

    if (!tree_open(&tree, "easy18.done")) {
        printf("could not open tree: %s\n", strerror(errno));
        return 1;
    }

    query_result_t result;
    query_result_clear(&result);

    move_t moves[] = { move_parse("e2e3") };
    size_t moves_len = 1;

    if (!tree_query(&tree, moves, moves_len, &result)) {
        printf("query failed\n");
        return 1;
    }

    // Could also query other trees now. Results are merged.

    query_result_sort(&result);

    for (size_t i = 0; i < result.num_children; i++) {
        char uci[MAX_UCI];
        move_uci(result.moves[i], uci);

        printf("%s %d\n", uci, result.sizes[i]);
    }

    return 0;
}
```

Acknowledgements
----------------

Thanks to [Mark Watkins](http://magma.maths.usyd.edu.au/~watkins/)
for solving antichess and publishing the proof trees.

License
-------

Licensed under the AGPLv3+. See COPYING for the full license text.
