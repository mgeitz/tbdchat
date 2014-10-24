CC=gcc
CFLAGS=-Wall -l pthread

all: chat_client chat_server

clean:
	rm -f chat_client chat_server
