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
#include <pthread.h>

#define PORT "32300" //Port clients will connect to
#define HOSTNAME "server1.cs.scranton.edu" //hostname of the chat server
#define BACKLOG 2 //How many pending connections the queue will hold

int get_server_socket(char *hostname, char *port);
int start_server(int serv_socket, int backlog);
int accept_client(int serv_sock);
void *clientA_thread(void *ptr);
void *clientB_thread(void *ptr);
void end();

int clientA_sock_fd;   //socket for first client
int clientB_sock_fd;   //socket for second client
int chat_serv_sock_fd; //server socket

int main() {

	//Threads
	pthread_t client_A_thread, client_B_thread;

	int iret1, iret2;

	//Open server socket
	chat_serv_sock_fd = get_server_socket(HOSTNAME, PORT);

        // step 3: get ready to accept connections
        if (start_server(chat_serv_sock_fd, BACKLOG) == -1) {
             printf("start server error\n");
             exit(1);
        }	

	//Accept client connections
	clientA_sock_fd = accept_client(chat_serv_sock_fd); 
	
	if(clientA_sock_fd != -1) printf("Client A connected\n");

	clientB_sock_fd = accept_client(chat_serv_sock_fd);

	if(clientB_sock_fd != -1) printf("Client B connected\n");

	iret1 = pthread_create(&client_A_thread, NULL, clientA_thread, NULL);
        iret2 = pthread_create(&client_B_thread, NULL, clientB_thread, NULL);

	pthread_join(client_A_thread, NULL);
	pthread_join(client_B_thread, NULL);

}

//Copied from Dr. Bi's example
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
    //print_ip(servinfo);
    freeaddrinfo(servinfo);   // servinfo structure is no longer needed. free it.

    return server_socket;
}

//Copied from Dr. Bi's example
int start_server(int serv_socket, int backlog) {
    int status = 0;
    if ((status = listen(serv_socket, backlog)) == -1) {
        printf("socket listen error\n");
    }
    return status;
}

//Copied from Dr. Bi's example
int accept_client(int serv_sock) {
    int reply_sock_fd = -1;
    socklen_t sin_size = sizeof(struct sockaddr_storage);
    struct sockaddr_storage client_addr;
    char client_printable_addr[INET6_ADDRSTRLEN];

    // accept a connection request from a client
    // the returned file descriptor from accept will be used
    // to communicate with this client.
    if ((reply_sock_fd = accept(serv_sock, 
       (struct sockaddr *)&client_addr, &sin_size)) == -1) {
            printf("socket accept error\n");
    }
    return reply_sock_fd;
}

void *clientA_thread(void *ptr) {
        char clientA_message[128];

	while (1) {
	        //Send client A message to client B
                int read_countA = recv(clientA_sock_fd, clientA_message, 128, 0);
                clientA_message[read_countA] = '\0';
                printf("%s\n", clientA_message);
		if(strcmp(clientA_message, "EXIT") == 0) {
			send(clientB_sock_fd, "Other user disconnected.", 128, 0);
			break;
		}

		else {
                	if(send(clientB_sock_fd, clientA_message, 128, 0) != -1) printf("Client A message sent\n");
		}
	}
	end();
}

void *clientB_thread(void *ptr) {
        char clientB_message[128];

	while(1) {
		//Send client B message to client A
                int read_countB = recv(clientB_sock_fd, clientB_message, 128, 0);
                clientB_message[read_countB] = '\0';
                printf("%s\n", clientB_message);
                
		if(strcmp(clientB_message, "EXIT") == 0) {
			send(clientA_sock_fd, "Other user disconnected.", 128, 0);
			break;
		}
		else {
			if(send(clientA_sock_fd, clientB_message, 128, 0) != -1) printf("Client B message sent\n");
		}
	}
	end();
}

void end() {
        close(clientA_sock_fd);
        close(clientB_sock_fd);
        close(chat_serv_sock_fd);

	printf("Session ended.\n");
	exit(0);
}
