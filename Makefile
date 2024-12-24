CC = gcc
CFLAGS = -lpthread `pkg-config --cflags gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0`

SRC = gui.c network.c server.c
OBJ = gui.o network.o server.o
EXEC = gui server

all: $(EXEC)

gui: gui.o network.o
	$(CC) -o gui gui.o network.o $(CFLAGS) $(LIBS)

server: server.o network.o
	$(CC) -o server server.o network.o $(CFLAGS) $(LIBS)

gui.o: gui.c network.h
	$(CC) -c gui.c $(CFLAGS)

server.o: server.c network.h
	$(CC) -c server.c $(CFLAGS)

network.o: network.c network.h
	$(CC) -c network.c $(CFLAGS)

clean:
	rm -f *.o $(EXEC)
