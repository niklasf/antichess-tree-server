CFLAGS = -g -O3 -Wall -Wno-misleading-indentation

HEADERS = losing.h

CORE_SOURCES = magic_mult.c gen_moves.c utils.c
CORE_OBJS = $(CORE_SOURCES:.c=.o)

PARSE_SOURCES = parse_win.c

PARSE_OBJS = $(CORE_OBJS) $(PARSE_SOURCES:.c=.o)

parse: $(PARSE_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -o parse $(PARSE_OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ -c $<
