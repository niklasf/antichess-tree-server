CFLAGS = -g -O3 -Wall

HEADERS = losing.h LCTB.h

CORE_SOURCES = magic_mult.c gen_moves.c utils.c
CORE_OBJS = $(CORE_SOURCES:.c=.o)

PARSE_SOURCES = parse_win.c parseTB.c TB_util.c TB_io.c TB_cache.c TB_decomp.c TB_index2.c

PARSE_OBJS = $(CORE_OBJS) $(PARSE_SOURCES:.c=.o)

parse: $(PARSE_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -o parse $(PARSE_OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ -c $<
