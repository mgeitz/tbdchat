/*
#    Chat client program.
#    Authors: Matthew Owens, Michael Geitz, Shayne Wierbowski
#
#    gcc ./chat_client.c -Wall -o chat_client -l pthread
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

void sigintHandler(int sig_num);
void print_ip( struct addrinfo *ai);
int get_server_connection(char *hostname, char *port);
void *chatRX(void *ptr);


int main() {
    pthread_t chat_rx_thread;         // Chat RX thread
    int conn;                         // Connection fd
    int exit_flag = 1;                // Exit flag for main loop
    int i;                            // Counter
    char kb_buf[BUFFERSIZE];          // Keyboard input buffer

    // Handle CTRL+C
    signal(SIGINT, sigintHandler);

    // Establish connection with server1
    printf("Connecting . . .\n");
    if ((conn = get_server_connection("134.198.169.2", PORT)) == -1) { 
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

        // Read input into buffer until newline or EOF (CTRL+D)
        i = 0;
        kb_buf[i] = getc(stdin);
        while (kb_buf[i] != '\n' && kb_buf[i] != EOF) { kb_buf[++i] = getc(stdin); }
        // If EOF is read, exit?
        if (kb_buf[i] == EOF) { exit_flag = 0; }
        // Otherwise Null terminate keyboard buffer
        else kb_buf[i] = '\0';

        // Transmit message if it is not empty
        if (i > 0 && kb_buf[i] != EOF) { send(conn, kb_buf, strlen(kb_buf), 0); }
        
        // Wipe keyboard buffer clean
        memset(kb_buf, 0, sizeof(kb_buf));
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
    exit(1); // Exit with error
}


/* Print messages as they are received */
void *chatRX(void *ptr) {
    char chat_rx_buf[BUFFERSIZE];    
    int received;                    
    int *conn = (int *)ptr;          
    while (1) { // Better condition here?
        // Wait for message to arrive..
        received = recv(*conn, &chat_rx_buf, sizeof(chat_rx_buf), 0);
        // Null terminate buffer
        chat_rx_buf[received] = '\0';
        // Print if not empty 
        if (received > 0) { 
            printf("%s\n", chat_rx_buf); 
            memset(chat_rx_buf, 0, sizeof(chat_rx_buf)); 
        }
    }
    return NULL;
}


/* Establish server connection */
int get_server_connection(char *hostname, char *port) {
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
