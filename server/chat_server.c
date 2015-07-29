/* 
//   Program:             TBD Chat Server
//   File Name:           chat_server.c
//   Authors:             Matthew Owens, Michael Geitz, Shayne Wierbowski
//   Date Started:        10/23/2014
//   Compile:             gcc -Wall -l pthread linked_list.c server_clients.c chat_server.c -o chat_server
//   Run:                 ./chat_server IP_ADDRESS PORT
//
//   The server for a simple chat utility
//   TBDChat is a simple chat client and server using BSD sockets
//   Copyright (C) 2014 Michael Geitz Matthew Owens Shayne Wierbowski
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License along
//   with this program; if not, write to the Free Software Foundation, Inc.,
//   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "chat_server.h"

int chat_serv_sock_fd; //server socket
int numRooms = DEFAULT_ROOM;
pthread_mutex_t registered_users_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t active_users_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rooms_mutex = PTHREAD_MUTEX_INITIALIZER;
Node *registered_users_list;
Node *active_users_list;
Node *room_list;
char const *server_MOTD = "Thanks for connecting to the TBDChat Demo Server."
                          " It's demo day!";


int main(int argc, char **argv) {
   if(argc < 3) {
      printf("%s --- Error:%s Usage: %s IP_ADDRESS PORT.\n", RED, NORMAL, argv[0]);
      exit(0);
   }
  
   signal(SIGINT, sigintHandler);
   
   room_list = NULL;
   registered_users_list = NULL;
   active_users_list = NULL;

   createRoom(&room_list, numRooms, DEFAULT_ROOM_NAME, rooms_mutex);
   RprintList(&room_list, rooms_mutex);

   readUserFile(&registered_users_list, USERS_FILE, registered_users_mutex);
   printList(&registered_users_list, registered_users_mutex);  
   // Open server socket
   chat_serv_sock_fd = get_server_socket(argv[1], argv[2]);
   
   // step 3: get ready to accept connections
   if(start_server(chat_serv_sock_fd, BACKLOG) == -1) {
      printf("start server error\n");
      exit(1);
   }	
   //Main execution loop   
   while(1) {
      //Accept a connection, start a thread
      int new_client = accept_client(chat_serv_sock_fd);
      if(new_client != -1) {
         pthread_t new_client_thread;
         pthread_create(&new_client_thread, NULL, client_receive, (void *)&new_client);
      }
   }
   
   close(chat_serv_sock_fd);
}


//Copied from Dr. Bi's example
int get_server_socket(char *hostname, char *port) {
   struct addrinfo hints, *servinfo, *p;
   int status;
   int server_socket;
   int yes = 1;
   
   memset(&hints, 0, sizeof hints);
   hints.ai_family = PF_UNSPEC;      // either ipv4 or ipv6
   hints.ai_socktype = SOCK_STREAM;  // TCP
   hints.ai_flags = AI_PASSIVE;      // Flag for returning bindable socket addr for either ipv4/6
   
   if ((status = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
      printf("getaddrinfo: %s\n", gai_strerror(status));
      exit(1);
   }
   
   for (p = servinfo; p != NULL; p = p ->ai_next) {
      // step 1: create a socket
      if((server_socket = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
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
   freeaddrinfo(servinfo);
   
   return server_socket;
}


/* Copied from Dr. Bi's example */
int start_server(int serv_socket, int backlog) {
   int status = 0;
   if ((status = listen(serv_socket, backlog)) == -1) {
      printf("socket listen error\n");
   }
   return status;
}


/* Copied from Dr. Bi's example */
int accept_client(int serv_sock) {
   int reply_sock_fd = -1;
   socklen_t sin_size = sizeof(struct sockaddr_storage);
   struct sockaddr_storage client_addr;
   //char client_printable_addr[INET6_ADDRSTRLEN];
   
   // accept a connection request from a client
   // the returned file descriptor from accept will be used
   // to communicate with this client.
   if ((reply_sock_fd = accept(serv_sock,(struct sockaddr *)&client_addr, &sin_size)) == -1) {
      printf("socket accept error\n");
   }
   return reply_sock_fd;
}


/* Dump contents of received packet from client */
void debugPacket(packet *rx_pkt) {
   printf("%s --------------------- TPS REPORT --------------------- %s\n", CYAN, NORMAL);
   printf("%s Timestamp: %s%lu\n", MAGENTA, NORMAL, rx_pkt->timestamp);
   printf("%s User Name: %s%s\n", MAGENTA, NORMAL, rx_pkt->username);
   printf("%s Real Name: %s%s\n", MAGENTA, NORMAL, rx_pkt->realname);
   printf("%s Option: %s%d\n", MAGENTA, NORMAL, rx_pkt->options);
   printf("%s Buffer: %s%s\n", MAGENTA, NORMAL, rx_pkt->buf);
   printf("%s ------------------------------------------------------- %s\n", CYAN, NORMAL);
}


/* Handle SIGINT (CTRL+C) */
void sigintHandler(int sig_num) {
   printf("\b\b%s --- Error:%s Forced Exit.\n", RED, NORMAL);
  
   //Closing client sockets and freeing memory from user lists
   Node *temp = active_users_list;
   Node *next;
   User *current;
   packet ret;
   strcpy(ret.username, SERVER_NAME);
   strcpy(ret.realname, SERVER_NAME);
   ret.timestamp = time(NULL);

   printf("--------CLOSING ACTIVE USERS--------\n");
   while(temp != NULL) {
      current = (User *) temp->data;
      printf("Closing %s's socket\n", current->username);
      next = temp->next;
      exit_client(&ret, current->sock);
      free(current);
      free(temp);
      temp = next;
   }

   temp = registered_users_list;
   printf("--------EMPTYING REGISTERED USERS LIST--------\n");
   while(temp != NULL) {
      next = temp->next;
      free(temp);
      temp = next;
   }
      
   close(chat_serv_sock_fd);
   exit(0);
}
