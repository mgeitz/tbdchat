CC=gcc
CPATH=client/
SPATH=server/
VPATH=$(CPATH)
CFLAGS_CLIENT=-Wall -l pthread $(CPATH)client_commands.c
CFLAGS_SERVER=-Wall -l pthread $(SPATH)linked_list.c $(SPATH)server_clients.c

all: chat_client chat_server

chat_client: $(CPATH)chat_client.c

	$(CC) $(CFLAGS_CLIENT) $(CPATH)chat_client.c -o tbdchat 

chat_server: $(SPATH)chat_server.c
	$(CC) $(CFLAGS_SERVER) $(SPATH)chat_server.c -o tbdchat_server


.PHONY: clean all

clean:
	rm -f tbdchat tbdchat_server Users.bin
