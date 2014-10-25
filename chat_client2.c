/* Program:             Simple Chat Client
   Authors:             Matthew Owens, Michale Geitz, Shayne Wierbowski
   Date:                10/23/2014
   File Name:           chat_client.c
   Compile:             gcc -o chat_client chat_client.c -l pthread
   Run:                 ./chat_client

   The client program for a simple two way chat utility

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
#include <time.h>
#include <pthread.h>

#define PORT "32300"
#define BUFFERSIZE 128

struct sendPacket
{
   time_t timestamp;
   char alias[32];
   char buf[BUFFERSIZE];
};
typedef struct sendPacket packet;

// Defined color constants
#define NORMAL "\x1B[0m"
#define BLACK "\x1B[30;1m"
#define RED "\x1B[31;1m"
#define GREEN "\x1B[32;1m"
#define YELLOW "\x1B[33;1m"
#define BLUE "\x1B[34;1m"
#define MAGENTA "\x1B[35;1m"
#define CYAN "\x1B[36;1m"
#define WHITE "\x1B[37;1m"

void sigintHandler(int sig_num);
void print_ip( struct addrinfo *ai);
int get_server_connection(char *hostname, char *port);
void *chatRX(void *ptr);

int main()
{
   pthread_t chat_rx_thread;         // Chat RX thread
   int conn;                         //
   int exit_flag = 1;                // Exit flag for main loop
   int i;                            // Counter
   char name[32];
   
   // Handle CTRL+C
   signal(SIGINT, sigintHandler);
   
   // Initiliaze memory space for send packet
   packet *spkt = (packet *) malloc(sizeof(packet));
   
   // Assign name as *nix username, maybe add ability to assign alias
   strcpy(name, getlogin());
   
   // Establish connection with server1, else exit with error
   printf("Connecting . . .\n");
   if((conn = get_server_connection("134.198.169.2", PORT)) == -1)
   {
      // If connection fails, exit
      close(conn);
      printf("\e[1m\x1b[31m --- Error:\x1b[0m\e[0m Connection failure.\n");
      exit_flag = 0;
   }
   
   // Start chat rx thread
   if(pthread_create(&chat_rx_thread, NULL, chatRX, (void *)&conn))
   {
      // If thread creation fails, exit
      printf("\e[1m\x1b[31m --- Error:\x1b[0m\e[0m chatRX thread not created.\n");
      exit_flag = 0;
   }
   
   // Primary execution loop
   while(exit_flag)
   {
      // Add alias to send packet
      strcpy(spkt->alias, name);
      
      // Read input into buffer until newline or EOF (CTRL+D)
      i = 0;
      spkt->buf[i] = getc(stdin);
      while(spkt->buf[i] != '\n' && spkt->buf[i] != EOF)
      {
         i ++;
         spkt->buf[i] = getc(stdin);
      }
      // If EOF is read, exit?
      if(spkt->buf[i] == EOF)
      {
         exit_flag = 0;
      }
      // Otherwise Null terminate kb_buff
      else
      {
         spkt->buf[i] = '\0';
      }
      
      // Transmit message if it is not empty
      if (i > 0 && spkt->buf[i] != EOF)
      {
         // Timestamp packet
         asctime(localtime(&spkt->timestamp));
         printf("%s%s (%s):%s%s\n", RED, spkt->alias,
                spkt->timestamp, spkt->buf, NORMAL);

         send(conn, spkt, sizeof(spkt), 0);
      }
      
      // Wipe buffers clean
      memset(spkt, 0, sizeof(spkt));
   }
   
   // Close connection
   if(pthread_join(chat_rx_thread, NULL))
   {
      printf("\e[1m\x1b[31m --- Error:\x1b[0m\e[0m chatRX thread not joining.\n");
   }
   printf("Exiting.\n");
   close(conn);
   exit(0);
}


/* Handle SIGINT (CTRL+C) */
void sigintHandler(int sig_num)
{ 
   //printf("\b\b  \b\b"); fflush(stdout); // Ignore CTRL+C
   printf("\b\b\e[1m\x1b[31m --- Error:\x1b[0m\e[0m Forced Exit.\n");
   exit(1); // Exit with error
}

/* Print messages as they are received */
void *chatRX(void *ptr)
{
   packet *chat_rx_buf = (packet *) malloc(sizeof(packet));
   int received;
   time_t ltime;
   int *conn = (int *)ptr;
   
   while(1)
   {
      // Wait for message to arrive..
      received = recv(*conn, chat_rx_buf, sizeof(chat_rx_buf), 0);
      // Null terminate buffer
      //chat_rx_buf[received] = '\0';
      // Print if not empty
      if(received > 0)
      {
         printf("%s%s (%s):%s%s\n", BLUE, chat_rx_buf->alias,
                chat_rx_buf->timestamp, chat_rx_buf->buf, NORMAL);

         //memset(chat_rx_buf, 0, sizeof(chat_rx_buf));
      }
   }
   return NULL;
}

/* Establish server connection */
int get_server_connection(char *hostname, char *port)
{
   int serverfd;
   struct addrinfo hints, *servinfo, *p;
   int status;
   
   /* How much of this do we need? */
   
   memset(&hints, 0, sizeof hints);
   hints.ai_family = PF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   
   if((status = getaddrinfo(hostname, port, &hints, &servinfo)) != 0)
   {
      printf("getaddrinfo: %s\n", gai_strerror(status));
      return -1;
   }
   
   print_ip(servinfo);
   for(p = servinfo; p != NULL; p = p ->ai_next)
   {
      if((serverfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
      {
         printf("\e[1m\x1b[31m --- Error:\x1b[0m\e[0m socket socket \n");
         continue;
      }
      
      if(connect(serverfd, p->ai_addr, p->ai_addrlen) == -1)
      {
         close(serverfd);
         printf("\e[1m\x1b[31m --- Error:\x1b[0m\e[0m socket connect \n");
         continue;
      }
      break;
   }
   
   freeaddrinfo(servinfo);
   send(serverfd, getlogin(), strlen(getlogin()), 0);
   return serverfd;
}

/* Copied wholesale from bi example */
void print_ip( struct addrinfo *ai)
{
   struct addrinfo *p;
   void *addr;
   char *ipver;
   char ipstr[INET6_ADDRSTRLEN];
   struct sockaddr_in *ipv4;
   struct sockaddr_in6 *ipv6;
   short port = 0;
   
   /* How much of this do we need? */
   
   for(p = ai; p !=  NULL; p = p->ai_next)
   {
      if(p->ai_family == AF_INET)
      {
         ipv4 = (struct sockaddr_in *)p->ai_addr;
         addr = &(ipv4->sin_addr);
         port = ipv4->sin_port;
         ipver = "IPV4";
      }
      else
      {
         ipv6= (struct sockaddr_in6 *)p->ai_addr;
         addr = &(ipv6->sin6_addr);
         port = ipv4->sin_port;
         ipver = "IPV6";
      }
      inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
      printf("serv ip info: %s - %s @%d\n", ipstr, ipver, ntohs(port));
   }
}
