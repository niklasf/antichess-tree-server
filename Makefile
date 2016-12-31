CFLAGS = -Wall

antichess-tree-server: tree.o server.o
	$(CC) $(LDFLAGS) -levent -o antichess-tree-server tree.o server.o

server.o: server.c tree.h
	$(CC) $(CFLAGS) -c server.c

tree.o: tree.c tree.h
	$(CC) $(CFLAGS) -c tree.c
