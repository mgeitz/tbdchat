/* Program:             Simple Chat Server
   Authors:             Matthew Owens, Michale Geitz, Shayne Wierbowski
   Date:                10/23/2014
   File Name:           chat_server.c
   Compile:             gcc -o chat_server chat_server.c -l pthread
   Run:                 ./chat_server

   The server program for a simple two way chat utility

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

#define PORT "32300" // port clients will connect to
#define HOSTNAME "server1.cs.scranton.edu" // hostname of the chat server
#define BACKLOG 2 // how many pending connections the queue will hold
#define BUFFERSIZE 128

struct sendPacket
{
   time_t timestamp;
   char alias[32];
   char buf[BUFFERSIZE];
};
typedef struct sendPacket packet;

struct chatSession
{
   int clientA_fd;
   int clientB_fd;
};
typedef struct chatSession session;
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

int get_server_socket(char *hostname, char *port);
int start_server(int serv_socket, int backlog);
int accept_client(int serv_sock, char* usrID);
void *clientA_thread(void *ptr);
void *clientB_thread(void *ptr);
void *subserver(void *ptr);
void end(session *ptr);
void start_subserver(int A_fd, int B_fd);

int chat_serv_sock_fd; //server socket

int main()
{
   //Client Sockets
   int clientA_sock_fd, clientB_sock_fd;

   // User IDs
   char clientA_usrID[32];
   char clientB_usrID[32];
   
   
   // Open server socket
   chat_serv_sock_fd = get_server_socket(HOSTNAME, PORT);
   
   // step 3: get ready to accept connections
   if(start_server(chat_serv_sock_fd, BACKLOG) == -1)
   {
      printf("start server error\n");
      exit(1);
   }	
  
   while(1)
   { 
      //Accept client connections
      clientA_sock_fd = accept_client(chat_serv_sock_fd, &clientA_usrID);
      if(clientA_sock_fd != -1)
      {
         printf("%s connected as Client A\n", clientA_usrID);
      }
   
      clientB_sock_fd = accept_client(chat_serv_sock_fd, &clientB_usrID);
      if(clientB_sock_fd != -1)
      {
         printf("%s connected as Client B\n", clientB_usrID);
      }
   
      start_subserver(clientA_sock_fd, clientB_sock_fd); 
   
   }

   
   close(chat_serv_sock_fd);

}

//Copied from Dr. Bi's example
int get_server_socket(char *hostname, char *port)
{
   struct addrinfo hints, *servinfo, *p;
   int status;
   int server_socket;
   int yes = 1;
   
   memset(&hints, 0, sizeof hints);
   hints.ai_family = PF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;
   
   if((status = getaddrinfo(hostname, port, &hints, &servinfo)) != 0)
   {
      printf("getaddrinfo: %s\n", gai_strerror(status));
      exit(1);
   }
   
   for(p = servinfo; p != NULL; p = p ->ai_next)
   {
      // step 1: create a socket
      if((server_socket = socket(p->ai_family, p->ai_socktype,
                                 p->ai_protocol)) == -1)
      {
         printf("socket socket \n");
         continue;
      }
      // if the port is not released yet, reuse it.
      if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
      {
         printf("socket option\n");
         continue;
      }
      
      // step 2: bind socket to an IP addr and port
      if(bind(server_socket, p->ai_addr, p->ai_addrlen) == -1)
      {
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
int start_server(int serv_socket, int backlog)
{
   int status = 0;
   if((status = listen(serv_socket, backlog)) == -1)
   {
      printf("socket listen error\n");
   }
   return status;
}

//Copied from Dr. Bi's example
int accept_client(int serv_sock, char* usrID)
{
   int reply_sock_fd = -1;
   socklen_t sin_size = sizeof(struct sockaddr_storage);
   struct sockaddr_storage client_addr;
   char client_printable_addr[INET6_ADDRSTRLEN];
   
   // accept a connection request from a client
   // the returned file descriptor from accept will be used
   // to communicate with this client.
   if((reply_sock_fd = accept(serv_sock,
      (struct sockaddr *)&client_addr, &sin_size)) == -1)
   {
      printf("socket accept error\n");
   }
   int i = recv(reply_sock_fd, usrID, 128, 0);
   //*usrID[i] = '\0';
   return reply_sock_fd;
}

void *clientA_thread(void *ptr)
{
   session *current_session = (session *)ptr;
   packet *clientA_message = (packet *) malloc(sizeof(packet));
   char *timestamp;
 
   while(1)
   {
      //Send client A message to client B
      int read_countA = recv(current_session->clientA_fd, clientA_message,
                             sizeof(clientA_message), 0);
      //clientA_message[read_countA] = '\0';
     
      //Format timestamp and remove \n
      timestamp = asctime(localtime(&(clientA_message->timestamp)));
      timestamp[strlen(timestamp) -1] = '\0';
     printf("%s%s (%s):%s%s\n", RED, clientA_message->alias,
             timestamp, clientA_message->buf, NORMAL);
      if(strcmp(clientA_message->buf, "EXIT") == 0)
      {
         //send(clientB_sock_fd, "Other user disconnected.", 128, 0);
         break;
      }
      else if(send(current_session->clientB_fd, clientA_message,
                     sizeof(clientA_message), 0) != -1)
      {
         //printf("Client A message sent successfully\n");
      }
   }
   free(clientA_message);
   end(current_session);
}

void *clientB_thread(void *ptr)
{
   session *current_session = (session *)ptr;
   packet *clientB_message = (packet *) malloc(sizeof(packet));
   char *timestamp;
 
   while(1)
   {
      //Send client B message to client A
      int read_countB = recv(current_session->clientB_fd, clientB_message,
                             sizeof(clientB_message), 0);
      //clientB_message[read_countB] = '\0';
      timestamp = asctime(localtime(&(clientB_message->timestamp)));
      timestamp[strlen(timestamp) -1]  = '\0';

      printf("%s%s (%s):%s%s\n", BLUE, clientB_message->alias,
             timestamp, clientB_message->buf, NORMAL);
     if(strcmp(clientB_message->buf, "EXIT") == 0)
      {
          //send(clientA_sock_fd, "Other user disconnected.", 128, 0);
          break;
      }
      else if(send(current_session->clientA_fd, clientB_message,
                     sizeof(clientB_message), 0) != -1)
      {
         //printf("Client B message sent successfully\n");
      }
   }
   free(clientB_message);
   end(current_session);
}

void end(session *ptr)
{
   close(ptr->clientA_fd);
   close(ptr->clientB_fd);
   
   printf("Session between %d and %d ended.\n", ptr->clientA_fd, ptr->clientB_fd);
   
   free(ptr);
   exit(0);
}

void start_subserver(int A_fd, int B_fd) 
{
   //Set up struct to pass to subserver
   session *newSession = (session *)malloc(sizeof(session));
   newSession->clientA_fd = A_fd;
   newSession->clientB_fd = B_fd;

   pthread_t new_session_thread;
   int iret;

   iret = pthread_create(&new_session_thread, NULL, subserver, (void *)newSession);

}

void *subserver(void *ptr)
{
   // Threads
   pthread_t client_A_thread, client_B_thread;

   int iret1, iret2;

   iret1 = pthread_create(&client_A_thread, NULL, clientA_thread, ptr);
   iret2 = pthread_create(&client_B_thread, NULL, clientB_thread, ptr);

   pthread_join(client_A_thread, NULL);
   pthread_join(client_B_thread, NULL);

}

