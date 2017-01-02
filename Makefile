CFLAGS += -O3 -Wall -Wextra -Wformat-security -pedantic
CFLAGS += -fstack-protector-all --param ssp-buffer-size=4
CFLAGS += -pie -fPIE
CFLAGS += -Wl,-z,relro,-z,now
CFLAGS += -D_FORTIFY_SOURCE=2

antichess-tree-server: san.o tree.o server.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o antichess-tree-server san.o tree.o server.o -levent

.PHONY: test
test: san.o tree.o test.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o test san.o tree.o test.o
	./test

test.o: test.c tree.h san.h
	$(CC) $(CFLAGS) -c test.c

server.o: server.c tree.h san.h
	$(CC) $(CFLAGS) -c server.c

tree.o: tree.c tree.h san.h
	$(CC) $(CFLAGS) -c tree.c

san.o: san.c san.h
	$(CC) $(CFLAGS) -c san.c
