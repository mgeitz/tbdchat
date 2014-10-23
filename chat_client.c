/*
#    Chat client program.
#    Authors: Matthew Owens, Michael Geitz, Shayne Wierbowski
#
#    gcc ./chat_client.c -Wall -o chat_client
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

#define PORT "32300"
#define BUFFERSIZE 256

//void sigintHandler(int sig_num);
void print_ip( struct addrinfo *ai);
int get_server_connection(char *hostname, char *port);


int main() {
    int conn;
    int exit_flag = 1;

    // Trap CTRL+C
    //signal(SIGINT, sigintHandler);

    // Establish connection with server1, else print halp
    if ((conn = get_server_connection("134.198.169.255", PORT)) == -1) { printf("halp"); }

    // Input and tx/rx loop here?
    while(exit_flag) {
        
    }

    // Close connection
    close(conn);
}

/* Handle SIGINT (CTRL+C) [basically ignores it]*/
//void sigintHandler(int sig_num) { printf("\b\b  \b\b"); fflush(stdout); }

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
