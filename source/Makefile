CC= gcc
APP=server
CFLAGS = -D_REENTRANT 
LDFLAGS = -lpthread 
OBJS=server.o

all: $(APP)

$(APP): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *~ $(OBJS) $(APP)