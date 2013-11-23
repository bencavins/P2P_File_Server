CC = gcc
FLAGS = -g -Wall -Wextra

all: client_PFS server_PFS

client_PFS: client_PFS.o
	$(CC) $(FLAGS) $^ -o $@

server_PFS: server_PFS.o
	$(CC) $(FLAGS) $^ -o $@

client_PFS.o: client_PFS.c
	$(CC) $(FLAGS) -c $<

server_PFS.o: server_PFS.c
	$(CC) $(FLAGS) -c $<

clean:
	rm -f client_PFS
	rm -f server_PFS
	rm -f *.o
	rm -f *~