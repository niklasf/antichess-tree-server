CFLAGS = -Wall

antichess-tree-server: san.o tree.o server.o
	$(CC) $(LDFLAGS) -o antichess-tree-server san.o tree.o server.o -levent

server.o: server.c tree.h san.h
	$(CC) $(CFLAGS) -c server.c

tree.o: tree.c tree.h san.h
	$(CC) $(CFLAGS) -c tree.c

san.o: san.c san.h
	$(CC) $(CFLAGS) -c san.c
