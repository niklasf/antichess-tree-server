CFLAGS = -Wall

antichess-tree-server: tree.o server.o
	$(CC) $(LDFLAGS) -o antichess-tree-server tree.o server.o -levent

server.o: server.c tree.h
	$(CC) $(CFLAGS) -c server.c

tree.o: tree.c tree.h
	$(CC) $(CFLAGS) -c tree.c
