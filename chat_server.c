/* Program:             Simple Chat Server
   Authors:             Matthew Owens, Michale Geitz, Shayne Wierbowski
   Date:                10/23/2014
   File Name:           chat_server.c
   Compile:             gcc -o chat_server chat_server.c -l pthread
   Run:                 ./chat_server IP_ADDRESS PORT

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
#include "linked_list.h"
#define BACKLOG 2               // how many pending connections the queue will hold
#define BUFFERSIZE 128

struct Packet
{
   time_t timestamp;
   char buf[BUFFERSIZE];
   int options;
};
typedef struct Packet packet;

struct chatSession
{
   int clientA_fd;
   int clientB_fd;
   char aliasA[32];
   char aliasB[32];
   int running;
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
int accept_client(int serv_sock);
void *clientA_thread(void *ptr);
void *clientB_thread(void *ptr);
void *subserver(void *ptr);
void end(session *ptr);
void start_subserver(int A_fd, int B_fd, char* clientA_usrID, char* clientB_usrID);
void sigintHandler(int sig_num);

int chat_serv_sock_fd; //server socket

int main(int argc, char **argv)
{
   
   if(argc < 3)
   {
      printf("%s --- Error:%s Usage: %s IP_ADDRESS PORT.\n", RED, NORMAL, argv[0]);
      exit(0);
   }
   
   // Handle CTRL+C
   signal(SIGINT, sigintHandler);
   
   //Client Sockets
   int clientA_sock_fd, clientB_sock_fd;
   
   // User IDs
   char clientA_usrID[32];
   char clientB_usrID[32];
   char clientA_name[32];
   char clientB_name[32];
   
   // Packet
   packet pck;
   packet pck2; 
   User *user_list = NULL;
  
   // Open server socket
   chat_serv_sock_fd = get_server_socket(argv[1], argv[2]);
   
   // step 3: get ready to accept connections
   if(start_server(chat_serv_sock_fd, BACKLOG) == -1)
   {
      printf("start server error\n");
      exit(1);
   }	
   
   //Main execution loop   
   while(1)
   {
      //Accept client connections
      clientA_sock_fd = accept_client(chat_serv_sock_fd);
      if(clientA_sock_fd != -1)
      {
         //while(!valid login)
         //{
            // Receive command and username
            recv(clientA_sock_fd, &pck, sizeof(pck), 0);
            strcpy(clientA_usrID, pck.buf);
            printf("User ID is: %s\n", clientA_usrID);
            
            // Register
            if(pck.options == 0)
            {
               recv(clientA_sock_fd, &pck, sizeof(pck), 0);
               //if(usrname valid)
               //{
                  strcpy(clientA_name, pck.buf);
                  printf("Name is: %s\n", clientA_name);
                  
                 User temp_user;
                 strcpy(temp_user.username, clientA_usrID);
                 strcpy(temp_user.real_name, clientA_name);
                 insert(&user_list, &temp_user);
               //}
            }
            
            // Login
            //if(username valid)
            //{
                  strcpy(clientA_name, get_real_name(&user_list, &clientA_usrID));
            //}
         //}
         printf("%s connected as Client A at socket_fd %d\n", clientA_name, clientA_sock_fd);
      }
      
      clientB_sock_fd = accept_client(chat_serv_sock_fd);
      if(clientB_sock_fd != -1)
      {
         //while(!valid login)
         //{
            // Receive command and username
            recv(clientB_sock_fd, &pck2, sizeof(pck2), 0);
            strcpy(clientB_usrID, pck2.buf);

            printf("User ID is: %s\n", clientB_usrID);
 
            // Register
            if(pck2.options == 0)
            {
               recv(clientB_sock_fd, &pck2, sizeof(pck), 0);
               //if(usrname valid)
               //{
                  strcpy(clientB_name, pck2.buf);
                  printf("Name is: %s\n", clientB_name);
                  User temp2;  
                  strcpy(temp2.username, clientB_usrID);
                  strcpy(temp2.real_name, clientB_name);
                  insert(&user_list, &temp2);
               //}
            }
            
            // Login
            //if(username valid)
            //{
                 strcpy(clientB_name, get_real_name(&user_list, &clientB_usrID));
               
            //}
         //}
         printf("%s connected as Client B at socket_fd %d\n", clientB_name, clientB_sock_fd);
      }
      
      start_subserver(clientA_sock_fd, clientB_sock_fd, &clientA_name, &clientB_name); 
   
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
   hints.ai_family = PF_UNSPEC;      // either ipv4 or ipv6
   hints.ai_socktype = SOCK_STREAM;  // TCP
   hints.ai_flags = AI_PASSIVE;      // Flag for returning bindable socket addr for either ipv4/6
   
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
int accept_client(int serv_sock)
{
   int reply_sock_fd = -1;
   socklen_t sin_size = sizeof(struct sockaddr_storage);
   struct sockaddr_storage client_addr;
   //char client_printable_addr[INET6_ADDRSTRLEN];
   
   // accept a connection request from a client
   // the returned file descriptor from accept will be used
   // to communicate with this client.
   if((reply_sock_fd = accept(serv_sock,
      (struct sockaddr *)&client_addr, &sin_size)) == -1)
   {
      printf("socket accept error\n");
   }
   return reply_sock_fd;
}


/*
 *Main thread for client A.  Tracks both incoming 
 *and outgoing messages.
 */
void *clientA_thread(void *ptr)
{
   session *current_session = (session *)ptr;
   packet clientA_message;
   char *timestamp;
   int received;
   
   strcpy(clientA_message.buf, current_session->aliasA);
   send(current_session->clientB_fd, (void *)&clientA_message, 
                     sizeof(packet), 0); 
   
   while(current_session->running)
   {
      //Send client A message to client B
      received = recv(current_session->clientA_fd, (void *)&clientA_message,
                             sizeof(packet), 0);
      
      //Format timestamp and remove \n
      timestamp = asctime(localtime(&(clientA_message.timestamp)));
      timestamp[strlen(timestamp) -1] = '\0';
      
      if(current_session->running)
      {
         printf("%s%s [%s]:%s%s\n", RED, timestamp, current_session->aliasA,
                NORMAL, clientA_message.buf);
      }
      
      //Handle "EXIT" message
      if(strcmp(clientA_message.buf, "EXIT") == 0)
      {
         send(current_session->clientB_fd, (void *)&clientA_message, sizeof(packet), 0);
         printf("Session between %d and %d ended.\n", current_session->clientA_fd,
            current_session->clientB_fd);
         
         current_session->running = 0;
         break;
      }
      
      //Send message to client B
      else if(send(current_session->clientB_fd, (void *)&clientA_message,
                     sizeof(packet), 0) != -1)
      {
         printf("Client A message sent successfully\n");
      }
   }

   end(current_session);
   return NULL;
}

/*
 *Main thread for client B. Tracks both incoming and 
 *outgoing messages.
 */
void *clientB_thread(void *ptr)
{
   session *current_session = (session *)ptr;
   packet clientB_message;
   char *timestamp;
   int received; 
   
   strcpy(clientB_message.buf, current_session->aliasB);
   send(current_session->clientA_fd, (void *)&clientB_message,
                     sizeof(packet), 0);
   while(current_session->running)
   {
      //Send client B message to client A
      received = recv(current_session->clientB_fd, (void *)&clientB_message,
                             sizeof(packet), 0);
      
      //Format timestamp and remove \n
      timestamp = asctime(localtime(&(clientB_message.timestamp)));
      timestamp[strlen(timestamp) -1]  = '\0';
      
      if(current_session->running)
      {
         printf("%s%s [%s]:%s%s\n", BLUE, timestamp, current_session->aliasB,
                NORMAL, clientB_message.buf);
      }
      
      if(strcmp(clientB_message.buf, "EXIT") == 0)
      {
          send(current_session->clientA_fd, (void *)&clientB_message, sizeof(packet), 0);
          printf("Session between %d and %d ended.\n", current_session->clientA_fd,
                 current_session->clientB_fd);
          
          current_session->running = 0;
          break;
      }
      else if(send(current_session->clientA_fd, (void *)&clientB_message,
                     sizeof(packet), 0) != -1)
      {
         printf("Client B message sent successfully\n");
      }
   }

   end(current_session);
   return NULL;
}

/*
 *Used to safely end a chat session.  Closes the sockets
 *for each client and sets a flag telling the threads 
 *for that session to exit.
 */
void end(session *ptr)
{
   close(ptr->clientA_fd);
   close(ptr->clientB_fd);
   free(ptr);
}

/*
 *Sets up necessary information to start a subserver
 *for a chat session.
 */
void start_subserver(int A_fd, int B_fd, char* clientA_usrID, char* clientB_usrID) 
{
   //Set up struct to pass to subserver
   session *newSession = (session *)malloc(sizeof(session));
   newSession->clientA_fd = A_fd;
   newSession->clientB_fd = B_fd;
   strcpy(newSession->aliasA, clientA_usrID);
   strcpy(newSession->aliasB, clientB_usrID);
   newSession->running = 1;
   
   //Start subserver thread
   pthread_t new_session_thread;
   int iret;
   
   iret = pthread_create(&new_session_thread, NULL, subserver, (void *)newSession);
}

/*
 *Subserver thread for a conversation.  Starts the main
 *threads for each client.
 */
void *subserver(void *ptr)
{
   // Threads
   pthread_t client_A_thread, client_B_thread;
   
   int iret1, iret2;
   
   iret1 = pthread_create(&client_A_thread, NULL, clientA_thread, ptr);
   iret2 = pthread_create(&client_B_thread, NULL, clientB_thread, ptr);
   
   pthread_join(client_A_thread, NULL);
   pthread_join(client_B_thread, NULL);
   
   return NULL;
}

/* Handle SIGINT (CTRL+C) */
void sigintHandler(int sig_num)
{
   printf("\b\b%s --- Error:%s Forced Exit.\n", RED, NORMAL);
   close(chat_serv_sock_fd);
   exit(0);
}
