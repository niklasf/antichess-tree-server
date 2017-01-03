CFLAGS += -O3 -Wall -Wextra -Wpedantic -Wformat-security -Wstack-protector
CFLAGS += -Wno-language-extension-token
CFLAGS += -fstack-protector-strong
CFLAGS += -D_FORTIFY_SOURCE=2 -fPIC

LDFLAGS += -pie -fPIE -Wl,-z,relro,-z,now

all: antichess-tree-server test

antichess-tree-server: san.o tree.o server.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o antichess-tree-server san.o tree.o server.o -levent

test: san.o tree.o test.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o test san.o tree.o test.o

test.o: test.c tree.h san.h
	$(CC) $(CFLAGS) -c test.c

server.o: server.c tree.h san.h
	$(CC) $(CFLAGS) -c server.c

tree.o: tree.c tree.h san.h
	$(CC) $(CFLAGS) -c tree.c

san.o: san.c san.h
	$(CC) $(CFLAGS) -c san.c
