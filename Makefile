CC=gcc
CFLAGS=-Wall -lssl


all: ftp_server ftp_client

ftp_server:	ftp_server.c
	$(CC) $(CFLAGS) -o $@ $^

ftp_client:	ftp_client.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f ftp_server ftp_client
