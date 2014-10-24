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
//#include <pthread.h>

#define PORT "32300"
#define BUFFERSIZE 128

void sigintHandler(int sig_num);
void print_ip( struct addrinfo *ai);
int get_server_connection(char *hostname, char *port);
//void receivePrint(void *ptr);


int main()
{
   //pthread_t thread1;
   int conn;                         //
   int exit_flag = 1;                // Exit flag for main loop
   int i;                            // Counter
   char kb_buf[BUFFERSIZE];          // Keyboard buffer
   char chat_rx_buf[BUFFERSIZE];     // Received chat buffer
   char myUsrID[32];
   char hisUsrID[32];
   time_t ltime;
   
   // Trap CTRL+C
   signal(SIGINT, sigintHandler);
   
   // Establish connection with server1, else exit with error
   if((conn = get_server_connection("134.198.169.2", PORT)) == -1)
   {
      close(conn);
      printf("ERROR: Could not connect to server, now terminating\n");
      exit(EXIT_FAILURE);
   }
   recv(conn, &hisUsrID, 128, 0);
   printf("Beginning conversation with %s\n", hisUsrID);
   
   //readThread = pthread_create( &thread1, NULL, receivePrint, (void*) &conn);
   
   // Input and tx/rx loop here?
   while(exit_flag)
   {
      // Read input into buffer until newline or EOF (CTRL+D)
      i = 0;
      kb_buf[i] = getc(stdin);
      while(kb_buf[i] != '\n' && kb_buf[i] != EOF)
      {
         i ++;
         kb_buf[i] = getc(stdin);
      }
      // If EOF is read, exit?
      if(kb_buf[i] == EOF)
      {
         exit_flag = 0;
      }
      // Otherwise Null terminate kb_buff
      kb_buf[i] = '\0';
      
      // Transmit message if it is not empty
      if(i > 0)
      {
         send(conn, kb_buf, strlen(kb_buf), 0);
      }
      
      // Receive message, print if not empty
      i = recv(conn, chat_rx_buf, sizeof(chat_rx_buf), 0);
      chat_rx_buf[i] = '\0';
      if(i > 0)
      {
         ltime=time(NULL); /* get current cal time */
         printf("%s [%s\b]: ", hisUsrID, asctime(localtime(&ltime)));
         printf("%s\n", chat_rx_buf);
      }
      
      // Wipe buffers clean
      memset(kb_buf, 0, sizeof(kb_buf));
      memset(chat_rx_buf, 0, sizeof(chat_rx_buf));
   }
   
   // Close connection
   //pthread_join(thread1, NULL);
   close(conn);
   exit(0);
}


/* Handle SIGINT (CTRL+C) [basically ignores it]*/
void sigintHandler(int sig_num)
{ 
   //printf("\b\b  \b\b"); fflush(stdout); // Ignore CTRL+C
   printf("\b\b\e[1m\x1b[31m --- Error:\x1b[0m\e[0m Forced Exit.\n");
   exit(1); // Exit with error
}

/* Print messages as they are received */
//void receivePrint(void *ptr)
//{
//   char buf[128];
//   int i;
//   int *conn = (int*)ptr;
//   while (1)
//   {
//      i = recv(conn, buf, sizeof(buf), 0);
//      buf[i] = '\0';
//      if (i > 0)
//      {
//         printf("%s\n", buf);
//         memset(buf, 0, sizeof(buf));
//      }
//   }
//}

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
