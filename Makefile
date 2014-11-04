#CC=gcc
CFLAGS_CLIENT=-Wall -l pthread
CFLAGS_SERVER=-Wall -l pthread linked_list.c

all: chat_client chat_server

chat_client: chat_client.c
	gcc chat_client.c -o chat_client $(CFLAGS_CLIENT)

chat_server: chat_server.c
	gcc chat_server.c -o chat_server $(CFLAGS_SERVER)

clean:
	rm -f chat_client chat_server
