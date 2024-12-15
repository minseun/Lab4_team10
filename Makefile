CC = gcc
CFLAGS = -lpthread `pkg-config --cflags gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0`

SRC = client.c gui.c network.c server.c
OBJ = client.o gui.o network.o server.o
EXEC = client gui server

all: $(EXEC)

client: client.o network.o
	$(CC) -o client client.o network.o $(CFLAGS) $(LIBS)

gui: gui.o network.o
	$(CC) -o gui gui.o network.o $(CFLAGS) $(LIBS)

server: server.o network.o
	$(CC) -o server server.o network.o $(CFLAGS) $(LIBS)

client.o: client.c network.h
	$(CC) -c client.c $(CFLAGS)

gui.o: gui.c network.h
	$(CC) -c gui.c $(CFLAGS)

server.o: server.c network.h
	$(CC) -c server.c $(CFLAGS)

network.o: network.c network.h
	$(CC) -c network.c $(CFLAGS)

clean:
	rm -f *.o $(EXEC)
