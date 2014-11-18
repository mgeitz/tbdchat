CC=gcc
CFLAGS_CLIENT=-Wall -l pthread client_commands.c
CFLAGS_SERVER=-Wall -l pthread linked_list.c

all: chat_client chat_server

chat_client: chat_client.c
	$(CC) $(CFLAGS_CLIENT) chat_client.c -o tbdchat 

chat_server: chat_server.c
	$(CC) $(CFLAGS_SERVER) chat_server.c -o tbdchat_server


.PHONY: clean all

clean:
	rm -f chat_client chat_server Users.bin
