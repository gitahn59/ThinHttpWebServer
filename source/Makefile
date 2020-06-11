CC= gcc
APP=server
CFLAGS = -D_REENTRANT
LDFLAGS = -lpthread 

all: server.o utility.o trie.o
	$(CC) -o $(APP) server.o utility.o trie.o $(LDFLAGS)

server.o: server.c utility.h trie.h
	$(CC) $(CFLAGS) -c server.c $(LDFLAGS)

utility.o: utility.c utility.h
	$(CC) $(CFLAGS) -c utility.c $(LDFLAGS)

trie.o: trie.c trie.h
	$(CC) $(CFLAGS) -c trie.c

clean:
	rm -f *~ $(APP).o $(APP) utility.o trie.o log.txt 