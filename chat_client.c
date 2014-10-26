/* 
#
#   Program:             Simple Chat Client
#   File Name:           chat_client.c
#
#   Authors:             Matthew Owens, Michale Geitz, Shayne Wierbowski
#   Date:                10/23/2014
#
#   Compile:             gcc chat_client.c -o chat_client -l pthread
#   Run:                 ./chat_client
#
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>
#include <pthread.h>

#define PORT "32300"
#define BUFFERSIZE 128

struct Packet {
    time_t timestamp;
    char alias[32];
    char buf[BUFFERSIZE];
};

typedef struct Packet packet;

void sigintHandler(int sig_num);
void print_ip( struct addrinfo *ai);
int get_server_connection(char *hostname, char *port, char *user);
void *chatRX(void *ptr);

// Declare exit_flag as global volatile int
int exit_flag = 1;

int main() {
    pthread_t chat_rx_thread;         // Chat RX thread
    int conn;                         // Connection fd
    int i;                            // Counter
    char name[32];                    // User alias

    // Handle CTRL+C
    signal(SIGINT, sigintHandler);

    // Initiliaze memory space for send packet
    packet tx_pkt;

    // Assign name as *nix username, maybe add ability to assign alias
    strcpy(name, getlogin());

    // Establish connection with server1
    printf("Connecting . . .\n");
    if ((conn = get_server_connection("134.198.169.2", PORT, name)) == -1) { 
        // If connection fails, exit
        printf("\e[1m\x1b[31m --- Error:\x1b[0m\e[0m Connection failure.\n");
        exit_flag = 0; 
    }
    // Start chat rx thread
    if (pthread_create(&chat_rx_thread, NULL, chatRX, (void *)&conn)) { 
        // If thread creation fails, exit
        printf("\e[1m\x1b[31m --- Error:\x1b[0m\e[0m chatRX thread not created.\n");
        exit_flag = 0; 
    }

    // Primary execution loop
    while(exit_flag) {

        // Add alias to send packet
        strcpy(tx_pkt.alias, name);

        // Read input into packet buffer until newline or EOF (CTRL+D)
        i = 0;
        tx_pkt.buf[i] = getc(stdin);
        while (tx_pkt.buf[i] != '\n' && tx_pkt.buf[i] != EOF) { 
            tx_pkt.buf[++i] = getc(stdin); 
        }
        // If EOF is read, exit?
        if (tx_pkt.buf[i] == EOF) { 
            exit_flag = 0; 
        }
        // Otherwise Null terminate keyboard buffer
        else { tx_pkt.buf[i] = '\0'; }

        // Transmit packet if input buffer is not empty
        if (i > 0 && tx_pkt.buf[i] != EOF) { 
            // Timestamp packet
            tx_pkt.timestamp = time(NULL);
            send(conn, (void *)&tx_pkt, sizeof(packet), 0); 
        }
        
        // Wipe packet buffer clean
        memset(&tx_pkt, 0, sizeof(packet));
    }

    // Close connection
    if (pthread_join(chat_rx_thread, NULL)) {
        printf("\e[1m\x1b[31m --- Error:\x1b[0m\e[0m chatRX thread not joining.\n");
    }
    printf("Exiting.\n");
    close(conn);
    exit(0);
}


/* Handle SIGINT (CTRL+C) */
void sigintHandler(int sig_num) { 
    //printf("\b\b  \b\b"); fflush(stdout); // Ignore CTRL+C
    printf("\b\b\e[1m\x1b[31m --- Error:\x1b[0m\e[0m Forced Exit.\n");
    exit_flag = 0;
}


/* Print messages as they are received */
void *chatRX(void *ptr) {
    //char chat_rx_buf[BUFFERSIZE];    
    //packet *rpkt = (packet *) malloc(sizeof(packet));
    packet rx_pkt;
    int received;                    
    int *conn = (int *)ptr;          
    char *timestamp;

    while (exit_flag) { 
        // Wait for message to arrive..
        received = recv(*conn, (void *)&rx_pkt, sizeof(packet), 0);
        // Print if not empty 
        if (received > 0) { 
            // Format timestamp
            timestamp = asctime(localtime(&(rx_pkt.timestamp)));
            timestamp[strlen(timestamp) -1] = '\0';
            printf("\x1b[31m\e[1m%s\x1b[0m | %s\e[0m: %s\n", timestamp, rx_pkt.alias, rx_pkt.buf); 
            memset(&rx_pkt, 0, sizeof(packet)); 
        }
    }
    return NULL;
}


/* Establish server connection */
int get_server_connection(char *hostname, char *port, char *user) {
    int serverfd;
    struct addrinfo hints, *servinfo, *p;
    int status;

   /* How much of this do we need? */

    memset(&hints, 0, sizeof hints);
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

   if ((status = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
       printf("getaddrinfo: %s\n", gai_strerror(status));
       return -1;
    }

    print_ip(servinfo);
    for (p = servinfo; p != NULL; p = p ->ai_next) {
       if ((serverfd = socket(p->ai_family, p->ai_socktype,
                           p->ai_protocol)) == -1) {
           printf("\e[1m\x1b[31m --- Error:\x1b[0m\e[0m socket socket \n");
           continue; // What is this error?
       }

       if (connect(serverfd, p->ai_addr, p->ai_addrlen) == -1) {
           close(serverfd);
           printf("\e[1m\x1b[31m --- Error:\x1b[0m\e[0m socket connect \n");
           return -1;
       }
       break;
    }

    freeaddrinfo(servinfo);
    send(serverfd, user, strlen(user), 0);   
    return serverfd;
}


/* Copied wholesale from bi example */
void print_ip( struct addrinfo *ai) {
   struct addrinfo *p;
   void *addr;
   char *ipver;
   char ipstr[INET6_ADDRSTRLEN];
   struct sockaddr_in *ipv4;
   struct sockaddr_in6 *ipv6;
   short port = 0;

   /* How much of this do we need? */

   for (p = ai; p !=  NULL; p = p->ai_next) {
      if (p->ai_family == AF_INET) {
         ipv4 = (struct sockaddr_in *)p->ai_addr;
         addr = &(ipv4->sin_addr);
         port = ipv4->sin_port;
         ipver = "IPV4";
      }
      else {
         ipv6= (struct sockaddr_in6 *)p->ai_addr;
         addr = &(ipv6->sin6_addr);
         port = ipv4->sin_port;
         ipver = "IPV6";
      }
      inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
      printf("serv ip info: %s - %s @%d\n", ipstr, ipver, ntohs(port));
   }
}
