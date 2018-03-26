CC=gcc
CFLAGS=-o

all:tcp_server tcp_client udp_server udp_client

tcp_server: tcp_server.c
	$(CC) $(CFLAGS) $@ tcp_server.c

tcp_client: tcp_client.c
	$(CC) $(CFLAGS) $@ tcp_client.c

udp_server: udp_server.c
	$(CC) $(CFLAGS) $@ udp_server.c

udp_client: udp_client.c
	$(CC) $(CFLAGS) $@ udp_client.c

clean:
	rm -f tcp_server tcp_client udp_server udp_client

