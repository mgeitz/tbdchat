/* Program:             Simple Chat Server
   Authors:             Matthew Owens, Michale Geitz, Shayne Wierbowski
   Date:                10/23/2014
   File Name:           chat_server.c
   Compile:             gcc -o chat_server chat_server.c -l pthread
   Run:                 ./chat_server IP_ADDRESS PORT

   The server program for a simple two way chat utility

*/
#include "chat_server.h"

int chat_serv_sock_fd; //server socket
User *user_list;
User *active_users;

// Tentative Room Code
Room *room_list;


int main(int argc, char **argv) {
   if(argc < 3) {
      printf("%s --- Error:%s Usage: %s IP_ADDRESS PORT.\n", RED, NORMAL, argv[0]);
      exit(0);
   }
   
   // Handle CTRL+C
   signal(SIGINT, sigintHandler);
   
   room_list = NULL;
   user_list = NULL;
   active_users = NULL;
   
   // Tentative Room Code
   room_list = NULL;
   Room r1;
   r1.ID = DEFAULT_ROOM;
   strcpy(r1.name, "ROOM TEST");
   r1.user_list = NULL;
   Rinsert(&room_list, &r1);
   
   readUserFile(&user_list, "Users.bin");
   printList(&user_list);  
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
         int iret;
         iret  = pthread_create(&new_client_thread, NULL, client_receive, (void *)&new_client);
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
   
   if((status = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
      printf("getaddrinfo: %s\n", gai_strerror(status));
      exit(1);
   }
   
   for(p = servinfo; p != NULL; p = p ->ai_next) {
      // step 1: create a socket
      if((server_socket = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
         printf("socket socket \n");
         continue;
      }
      // if the port is not released yet, reuse it.
      if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
         printf("socket option\n");
         continue;
      }
      
      // step 2: bind socket to an IP addr and port
      if(bind(server_socket, p->ai_addr, p->ai_addrlen) == -1) {
         printf("socket bind \n");
         continue;
      }
      break;
   }
   freeaddrinfo(servinfo);   // servinfo structure is no longer needed. free it.
   
   return server_socket;
}


/* Copied from Dr. Bi's example */
int start_server(int serv_socket, int backlog) {
   int status = 0;
   if((status = listen(serv_socket, backlog)) == -1) {
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
   if((reply_sock_fd = accept(serv_sock,(struct sockaddr *)&client_addr, &sin_size)) == -1) {
      printf("socket accept error\n");
   }
   return reply_sock_fd;
}


/* Dump contents of received packet from client */
void debugPacket(packet *rx_pkt) {
   printf("%s --------------------- TPS REPORT --------------------- %s\n", CYAN, NORMAL);
   printf("%s Timestamp: %s%lu\n", MAGENTA, NORMAL, rx_pkt->timestamp);
   printf("%s Alias: %s%s\n", MAGENTA, NORMAL, rx_pkt->alias);
   printf("%s Option: %s%d\n", MAGENTA, NORMAL, rx_pkt->options);
   printf("%s Buffer: %s%s\n", MAGENTA, NORMAL, rx_pkt->buf);
   printf("%s ------------------------------------------------------- %s\n", CYAN, NORMAL);
}


/*
 *Used to safely end a chat session.  Closes the sockets
 *for each client and sets a flag telling the threads 
 *for that session to exit.
 */
void end(session *ptr) {
   close(ptr->clients[0]);
   close(ptr->clients[1]);
   free(ptr);
}


/* Handle SIGINT (CTRL+C) */
void sigintHandler(int sig_num) {
   printf("\b\b%s --- Error:%s Forced Exit.\n", RED, NORMAL);
   close(chat_serv_sock_fd);
   exit(0);
}


/*
 *Main thread for each client.  Receives all messages
 *and passes the data off to the correct function.  Receives
 *a pointer to the file descriptor for the socket the thread
 *should listen on
 */
void *client_receive(void *ptr) {
   int client = *(int *) ptr;
   int received;
   packet in_pkt, *client_message_ptr = &in_pkt;
   while (1) {
      received = recv(client, &in_pkt, sizeof(packet), 0);
      if (received) { 
         debugPacket(client_message_ptr); 

         // Handle command messages
         if (in_pkt.options < 1000) {
            if(in_pkt.options == REGISTER) { 
               register_user(&in_pkt, client);
            }
            else if(in_pkt.options == SETPASS) {
               set_pass(&in_pkt, client);
            }
            else if(in_pkt.options == SETNAME) {
               set_name(&in_pkt, client);
            }
            else if(in_pkt.options == LOGIN) {
               login(&in_pkt, client);
            }
            else if(in_pkt.options == EXIT) {
               exit_client(&in_pkt);
            }
            else if(in_pkt.options == INVITE) {
               invite(&in_pkt);
            }
            else if(in_pkt.options == JOIN) { 
               join(&in_pkt);
            }
            else if(in_pkt.options == GETUSERS) {
               get_active_users(client);
            }
            else {
               printf("%s --- Error:%s Unknown message received from client.\n", RED, NORMAL);
            }
         }

         // Handle conversation message
         else { 
            send_message(&in_pkt, client);
         }
         memset(&in_pkt, 0, sizeof(packet));
      }
   }
   return NULL;
}


/*
 *Register
 */
void register_user(packet *pkt, int fd) {
   User *temp_user = (User *)malloc(sizeof(User));
   temp_user->next = NULL;
   char *args[5];
   char *tmp;
   tmp = pkt->buf;
   packet ret;
   
   //Pull command
   args[0] = strsep(&tmp, " \t");
   
   //Pull username
   args[1] = strsep(&tmp, " \t");
   if(strcmp(get_real_name(&user_list, args[1]), "ERROR") !=0) {
      ret.timestamp = time(NULL);
      strcpy(ret.alias, "SERVER");
      ret.options = REGFAIL;
      strcpy(ret.buf, "Username taken.");
      debugPacket(&ret);
      send(fd, &ret, sizeof(ret), 0);
      printf("sent\n");
      return;
   }
   
   //Pull password
   args[2] = strsep(&tmp, " \t");
   strcpy(temp_user->username, args[1]);
   strcpy(temp_user->password, args[2]);
   temp_user->sock = fd;
   insert(&user_list, temp_user);
   insert(&active_users, temp_user);
   
   // Tentative Room Code
   temp_user = (User *)malloc(sizeof(User));
   strcpy(temp_user->username, args[1]);
   strcpy(temp_user->password, args[2]);
   temp_user->sock = fd;
   User *aList = (Rget_roomFID(&room_list, DEFAULT_ROOM))->user_list;
   insert(&aList, temp_user);
   
   //Return success message
   ret.timestamp = time(NULL);
   strcpy(ret.alias, "SERVER");
   ret.options = REGSUC;
   send(fd, &ret, sizeof(ret), 0);
   
   writeUserFile(&user_list, "Users.bin");
   printf("New User Registered\n");
}


/*
 *Login
 */
void login(packet *pkt, int fd) {
   // Tentative Room Code
   User *temp_user = (User *)malloc(sizeof(User));
   
   char *args[3];
   packet ret;
   char *tmp;
   tmp = pkt->buf;
   
   //Pull command
   args[0] = strsep(&tmp, " \t");
   
   //Pull username and check valid
   args[1] = strsep(&tmp, " \t");
   if (strcmp(get_real_name(&user_list, args[1]), "ERROR") ==0) {
      ret.options = LOGFAIL;
      ret.timestamp = time(NULL);
      strcpy(ret.alias, "SERVER");
      strcpy(ret.buf, "Username not found.");
      send(fd, &ret, sizeof(ret), 0);
      return;
   }
   //Pull password and check if it is valid
   args[2] = strsep(&tmp, " \t");
   char *password = get_password(&user_list, args[1]);
   
   if (strcmp(args[2], password) != 0) {
     ret.options = LOGFAIL;
     ret.timestamp = time(NULL);
     strcpy(ret.alias, "SERVER");
     strcpy(ret.buf, "Incorrect password.");
     send(fd, &ret, sizeof(ret), 0);
     return;
   }
   //Login successful, send username to client and add to active_users
   User *user = (User *)malloc(sizeof(User));
   strcpy(user->username, args[1]);
   user->sock = fd;
   user->next = NULL;
   user->sock = fd;
   insert(&active_users, user);
   
   // Tentative Room Code
   temp_user = (User *)malloc(sizeof(User));
   strcpy(temp_user->username, args[1]);
   strcpy(temp_user->password, args[2]);
   temp_user->sock = fd;
   User *aList = (Rget_roomFID(&room_list, DEFAULT_ROOM))->user_list;
   insert(&aList, temp_user);
   //insert(&((Rget_roomFID(&room_list, &DEFAULT_ROOM))->user_list), temp_user);
   
   ret.options = LOGSUC;
   ret.timestamp = time(NULL);
   //strcpy(ret.alias, "SERVER");
   strcpy(ret.buf, get_real_name(&user_list, args[1]));
   send(fd, &ret, sizeof(ret), 0);
   printf("User logged in\n");
}


/*
 *Invite
 */
void invite(packet *pkt) {

}


/*
 *Exit
 */
void exit_client(packet *pkt) {

}


/*
 *Send Message
 */
void send_message(packet *pkt, int clientfd) {
    // Tentative Room Code
    Room *room = Rget_roomFID(&room_list, pkt->options);
    printf("Sending message to Room %d, %s\n", room->ID, room->name);
    printf("The Users in Room %d %s are...\n", room->ID, room->name);
    printList(&(room->user_list));
    User *tmp = room->user_list;
    
    //User *tmp = active_users;
    while(tmp != NULL) {
       if (clientfd != tmp->sock) {
          send(tmp->sock, (void *)pkt, sizeof(packet), 0);
       }
       tmp = tmp->next;
    }
}


/*
 *Get active users
 */
void get_active_users(int fd) {
    User *temp = active_users;
    packet pkt;
    pkt.options = GETUSERS;
    strcpy(pkt.alias, "SERVER");
    while(temp != NULL ) {
     pkt.timestamp = time(NULL);
     strcpy(pkt.buf, temp->username);
     send(fd, &pkt, sizeof(pkt), 0);
     temp = temp->next;
    }
}


/*
 *Set user password
 */
void set_pass(packet *pkt, int fd) {
   char *args[3];
   char *tmp = pkt->buf;

   // We should loop this incase a malformed command gets through and segfaults
   //Pull command, old pw, new pw
   args[0] = strsep(&tmp, " \t");
   args[1] = strsep(&tmp, " \t");
   args[2] = strsep(&tmp, " \t");

   User *user = get_user(&user_list, pkt->alias);
   if (user != NULL) {
      if(strcmp(user->password, args[1]) == 0) {
         memset(user->password, 0, 32);
         strcpy(user->password, args[2]);
         writeUserFile(&user_list, "Users.bin");
         pkt->options = PASSSUC;
      }
   }

   else {
      pkt->options = PASSFAIL;
   }

   send(fd, (void *)pkt, sizeof(packet), 0);
}


/*
 *Set user real name
 */
void set_name(packet *pkt, int fd) {
   char name[64];
   packet ret;

   strncpy(name, pkt->buf, sizeof(name));
   strncpy(ret.buf, pkt->buf, sizeof(ret.buf));

   //Submit name change to user list, write list
   User *user = get_user(&user_list, pkt->alias);
   printf("Username: %s, Real name: %s\n", user->username, user->real_name);

   if(user != NULL) {
      memset(user->real_name, 0, sizeof(user->real_name));
      strncpy(user->real_name, name, sizeof(name));
      writeUserFile(&user_list, "Users.bin");
      ret.options = NAMESUC;
   }
   else {
      printf("%s --- Error:%s Trying to modify null user in user_list.\n", RED, NORMAL);
      ret.options = NAMEFAIL;
   }
   
   //Submit name change to active users
   user = get_user(&active_users, pkt->alias);
   if(user != NULL) {
      memset(user->real_name, 0, sizeof(user->real_name));
      strncpy(user->real_name, name, sizeof(name));
   }
   else {
      printf("%s --- Error:%s Trying to modify null user in active_users.\n", RED, NORMAL);
      ret.options = NAMEFAIL;
   }

   printList(&user_list);
   printList(&active_users);
   ret.timestamp = time(NULL);
   send(fd, &ret, sizeof(packet), 0);
}


/*
 *Join a chat room
 */
void join(packet *pkt) {

}
