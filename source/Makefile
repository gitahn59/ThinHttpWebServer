CC= gcc
APP=server
CFLAGS = -D_REENTRANT 
LDFLAGS = -lpthread 

all: server.o utility.o
	$(CC) -o $(APP) server.o utility.o $(LDFLAGS)

server.o: server.c utility.h
	$(CC) $(CFLAGS) -c server.c $(LDFLAGS)

utility.o: utility.c utility.h
	$(CC) $(CFLAGS) -c utility.c $(LDFLAGS)

clean:
	rm -f *~ $(APP).o $(APP) utility.o log.txt