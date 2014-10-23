/*
 *Chat server program
 *Authors: Matthew Owens, Michael Geitz, Shayne Wierbowski
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>
#include <netdb.h>


#define PORT "32300" //Port clients will connect to
#define HOSTNAME "server1.cs.scranton.edu" //hostname of the chat server
#define BACKLOG 2 //How many pending connections the queue will hold

int get_server_socket(char *hostname, char *port);
int start_server(int serv_socket, int backlog);


int main() {
	int chat_serv_sock_fd;
	int clientA_sock_fd;
	int clientB_sock_fd;

	char clientA_message[128];
	char clientB_message[128];

	chat_serv_sock_fd = get_server_socket(HOSTNAME, PORT);

        // step 3: get ready to accept connections
        if (start_server(chat_serv_sock_fd, BACKLOG) == -1) {
             printf("start server error\n");
             exit(1);
        }	

	while(strcmp(clientA_message, "EXIT") != 0 && strcmp(clientB_message, "EXIT") != 0) {
	
	}
int get_server_socket(char *hostname, char *port) {
    struct addrinfo hints, *servinfo, *p;
    int status;
    int server_socket;
    int yes = 1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
       printf("getaddrinfo: %s\n", gai_strerror(status));
       exit(1);
    }

    for (p = servinfo; p != NULL; p = p ->ai_next) {
       // step 1: create a socket
       if ((server_socket = socket(p->ai_family, p->ai_socktype,
                           p->ai_protocol)) == -1) {
           printf("socket socket \n");
           continue;
       }
       // if the port is not released yet, reuse it.
       if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
         printf("socket option\n");
         continue;
       }

       // step 2: bind socket to an IP addr and port
       if (bind(server_socket, p->ai_addr, p->ai_addrlen) == -1) {
           printf("socket bind \n");
           continue;
       }
       break;
    }
    print_ip(servinfo);
    freeaddrinfo(servinfo);   // servinfo structure is no longer needed. free it.

    return server_socket;
}

int start_server(int serv_socket, int backlog) {
    int status = 0;
    if ((status = listen(serv_socket, backlog)) == -1) {
        printf("socket listen error\n");
    }
    return status;
}}
