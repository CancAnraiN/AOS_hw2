GCC=gcc


all: client.c server.c
	$(GCC) -c client.c
	$(GCC) -o client client.o
	$(GCC) -c server.c
	$(GCC) -o server server.o
clean:
	rm -f client, server, *.o
