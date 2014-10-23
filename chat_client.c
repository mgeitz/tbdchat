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
//#include <pthread.h>

#define PORT "32300"
#define BUFFERSIZE 256

//void sigintHandler(int sig_num);
void print_ip( struct addrinfo *ai);
int get_server_connection(char *hostname, char *port);
void receivePrint(void *ptr);


int main() {
    //pthread_t thread1;
    int conn;             //
    int exit_flag = 1;    // Exit flag var
    int i;
    char buffer[128];     // Input buffer

    // Trap CTRL+C
    //signal(SIGINT, sigintHandler);

    // Establish connection with server1, else exit with error
    if ((conn = get_server_connection("134.198.169.2", PORT)) == -1) { close(conn); exit(EXIT_FAILURE); }

    //readThread = pthread_create( &thread1, NULL, receivePrint, (void*) &conn);

    // Input and tx/rx loop here?
    while(exit_flag) {
        // Read input into buffer until newline or EOF (CTRL+D)
        i = 0;
        buffer[i] = getc(stdin);
        while (buffer[i] != '\n' && buffer[i] != EOF) { buffer[++i] = getc(stdin); }
        buffer[i] = '\0';

        // Transmit message
        send(conn, buffer, strlen(buffer), 0);
        
        // Receive message
        memset(buffer, 0, sizeof(buffer));
        //i = 0;
        i = recv(conn, buffer, sizeof(buffer), 0);
        buffer[i] = '\0';
        printf("%s\n", buffer);

        // Wipe buffer clean
        memset(buffer, 0, sizeof(buffer));
    }

    // Close connection
    //pthread_join(thread1, NULL);
    close(conn);
    exit(0);
}


/* Handle SIGINT (CTRL+C) [basically ignores it]*/
//void sigintHandler(int sig_num) { printf("\b\b  \b\b"); fflush(stdout); }


/* Print messages as they are received */
void receivePrint(void *ptr) {
    char buf[128];
    int i;
    int *conn = (int*)ptr;
    while (1) {
        i = recv(conn, buf, sizeof(buf), 0);
        buf[i] = '\0';
        if (i > 0) { printf("%s\n", buf); memset(buf, 0, sizeof(buf)); }
    }
}

/* Copied wholesale from bi example */
int get_server_connection(char *hostname, char *port) {
    int serverfd;
    struct addrinfo hints, *servinfo, *p;
    int status;

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
           printf("socket socket \n");
           continue;
       }

       if (connect(serverfd, p->ai_addr, p->ai_addrlen) == -1) {
           close(serverfd);
           printf("socket connect \n");
           continue;
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
