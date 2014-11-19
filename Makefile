CC=gcc
CLIENT_NAME=tbdchat
SERVER_NAME=tbdchat_server
SERVER_USERS_FILE=Users.bin
CPATH=client/
SPATH=server/
CFLAGS_CLIENT=-Wall -l pthread $(CPATH)client_commands.c
CFLAGS_SERVER=-Wall -l pthread $(SPATH)linked_list.c $(SPATH)server_clients.c

all: chat_client chat_server

chat_client: $(CPATH)chat_client.c

	$(CC) $(CFLAGS_CLIENT) $(CPATH)chat_client.c -o $(CLIENT_NAME) 

chat_server: $(SPATH)chat_server.c
	$(CC) $(CFLAGS_SERVER) $(SPATH)chat_server.c -o $(SERVER_NAME)

.PHONY: clean all

clean:
	rm -f $(CLIENT_NAME) $(SERVER_NAME) $(SERVER_USERS_FILE)
