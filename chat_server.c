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
int numRooms = DEFAULT_ROOM;
User *registered_users_list;
User *active_users_list;
Room *room_list;
char *server_MOTD = "Welcome to The Best Damn Chat Server! This is a temporary MOTD without version numbers or anything particularly useful.";

int main(int argc, char **argv) {
   if(argc < 3) {
      printf("%s --- Error:%s Usage: %s IP_ADDRESS PORT.\n", RED, NORMAL, argv[0]);
      exit(0);
   }
   
   // Handle CTRL+C
   signal(SIGINT, sigintHandler);
   
   room_list = NULL;
   registered_users_list = NULL;
   active_users_list = NULL;
   
   createRoom(&room_list, numRooms, "Lobby");
   RprintList(&room_list);
   
   readUserFile(&registered_users_list, "Users.bin");
   printList(&registered_users_list);  
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
   freeaddrinfo(servinfo);   // servinfo structure is no longer needed. free it.
   
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
  
   //Closing client sockets and freeing memory from user lists
   User *temp = active_users_list;
   User *next;

   printf("--------CLOSING ACTIVE USERS--------\n");
   while(temp != NULL) {
      printf("Closing %s's socket\n", temp->username);
      next = temp->next;
      exit_client(temp->sock);
      free(temp);
      temp = next;
   }

   temp = registered_users_list;
   printf("--------EMPTYING REGISTERED USERS LIST--------\n");
   while(temp != NULL) {
      printf("Freeing %s\n", temp->username);
      next = temp->next;
      free(temp);
      temp = next;
   }
      
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
               exit_client(client);
               return NULL;
            }
            else if(in_pkt.options == INVITE) {
               invite(&in_pkt, client);
            }
            else if(in_pkt.options == JOIN) { 
               join(&in_pkt, client);
            }
            else if(in_pkt.options == LEAVE) { 
               //leave(&in_pkt, client);
               printf("%s%s wants to %s %s Unknown message received from client.\n", YELLOW, in_pkt.username, in_pkt.buf, NORMAL);
            }
            else if(in_pkt.options == GETALLUSERS) {
               get_active_users(client);
            }
            else if(in_pkt.options == GETUSERS) {
               get_room_users(&in_pkt, client);
            }
            else if(in_pkt.options == GETUSER) {
               user_lookup(&in_pkt, client);
            }
            else if(in_pkt.options == GETROOMS) {
               get_room_list(client);
            }
            else if(in_pkt.options == GETMOTD) {
               sendMOTD(client);
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
   char *args[5];
   char *tmp;
   tmp = pkt->buf;
   packet ret;
   
   //Pull command
   args[0] = strsep(&tmp, " \t");
   
   //Pull username
   args[1] = strsep(&tmp, " \t");
   //
   if(strcmp(get_real_name(&registered_users_list, args[1]), "ERROR") !=0) {
      ret.timestamp = time(NULL);
      ret.options = REGFAIL;
      strcpy(ret.buf, "Username taken.");
      debugPacket(&ret);
      send(fd, &ret, sizeof(ret), 0);
      printf("sent\n");
      return;
   }
   User *user = (User *)malloc(sizeof(User));
   
   //Pull password
   args[2] = strsep(&tmp, " \t");
   strcpy(user->username, args[1]);
   strcpy(user->real_name, args[1]);
   strcpy(user->password, args[2]);
   user->sock = fd;
   user->next = NULL;
   
   insertUser(&registered_users_list, user);
   user = clone_user(user);
   insertUser(&active_users_list, user);
   Room *defaultRoom = Rget_roomFID(&room_list, DEFAULT_ROOM);
   user = clone_user(user);
   insertUser(&(defaultRoom->user_list), user);
   RprintList(&room_list);  
   
   //Return success message
   ret.timestamp = time(NULL);
   ret.options = REGSUC;
   send(fd, &ret, sizeof(ret), 0);
   
   writeUserFile(&registered_users_list, "Users.bin");
   printf("New User Registered\n");

   memset(&ret, 0, sizeof(packet));
   ret.options = DEFAULT_ROOM;
   strcpy(ret.realname, "SERVER");
   strncpy(ret.buf, user->real_name, sizeof(user->real_name));
   strcat(ret.buf, " has joined the lobby.");
   ret.timestamp = time(NULL);
   send_message(&ret, -1);
   sendMOTD(fd);
}


/*
 *Login
 */
void login(packet *pkt, int fd) {
   char *args[3];
   packet ret;
   char *tmp;
   tmp = pkt->buf;
   
   //Pull command
   args[0] = strsep(&tmp, " \t");
   
   //Pull username and check valid
   args[1] = strsep(&tmp, " \t");
   if (strcmp(get_real_name(&registered_users_list, args[1]), "ERROR") ==0) {
      ret.options = LOGFAIL;
      ret.timestamp = time(NULL);
      strcpy(ret.buf, "Username not found.");
      send(fd, &ret, sizeof(packet), 0);
      return;
   }
   //Pull password and check if it is valid
   args[2] = strsep(&tmp, " \t");
   char *password = get_password(&registered_users_list, args[1]);
   
   if (strcmp(args[2], password) != 0) {
     ret.options = LOGFAIL;
     ret.timestamp = time(NULL);
     strcpy(ret.buf, "Incorrect password.");
     send(fd, &ret, sizeof(packet), 0);
     return;
   }
   //Login successful, send username to client and add to active_users
   User *user = get_user(&registered_users_list, args[1]);
   user->sock = fd;

   user = clone_user(user);
   
   if(insertUser(&active_users_list, user) == 1) {
      Room *defaultRoom = Rget_roomFID(&room_list, DEFAULT_ROOM);
      user = clone_user(user);
      insertUser(&(defaultRoom->user_list), user);
      RprintList(&room_list);  
      strcpy(ret.realname, get_real_name(&registered_users_list, args[1]));
      strcpy(ret.username, args[1]);
      ret.options = LOGSUC;
      printf("User logged in\n");
   }
   else {
      ret.options = LOGFAIL;
      printf("User log in failed: alredy logged in");
   }
   ret.timestamp = time(NULL);
   send(fd, &ret, sizeof(packet), 0);
   if (ret.options == LOGSUC) {
      memset(&ret, 0, sizeof(packet));
      ret.options = DEFAULT_ROOM;
      strcpy(ret.realname, "SERVER");
      strncpy(ret.buf, user->real_name, sizeof(user->real_name));
      strcat(ret.buf, " has joined the lobby.");
      ret.timestamp = time(NULL);
      send_message(&ret, -1);

      sendMOTD(fd);
   }
}


/*
 *Invite
 */
void invite(packet *in_pkt, int fd) {
   int i = 0, roomNum;
   char *args[16];
   char *tmp = in_pkt->buf;
   packet ret;

   args[i] = strsep(&tmp, " \t");
   while ((i < sizeof(args) - 1) && (args[i] != '\0')) {
      args[++i] = strsep(&tmp, " \t");
   }
   if (i > 1) {
      roomNum = atoi(args[1]);
      Room *currRoom = Rget_roomFID(&room_list, roomNum);
      if (currRoom != NULL) {
         User *inviteUser = get_user(&active_users_list, args[0]);
         if (inviteUser != NULL) {
            ret.options = INVITE;
            strcpy(ret.username, "SERVER");
            memset(&ret.buf, 0, sizeof(ret.buf));
            sprintf(ret.buf, "%s has invited you to join %s", in_pkt->realname, Rget_name(&room_list, roomNum));
            send(inviteUser->sock, &ret, sizeof(packet), 0);
            memset(&ret, 0, sizeof(packet));
            ret.options = INVITESUC;
            strcpy(ret.username, "SERVER");
            send(fd, &ret, sizeof(packet), 0);
            return;
         }
      }
      else {
         printf("%s --- Error:%s Trying to read user info but room is null.\n", RED, NORMAL);
      }
   }
   else {
      printf("%s --- Error:%s Malformed buffer received, ignoring.\n", RED, NORMAL);
   }
   ret.options = INVITEFAIL;
   strcpy(ret.username, "SERVER");
   send(fd, &ret, sizeof(packet), 0);
}


/* Send the server MOTD to the socket passed in */
void sendMOTD(int fd) {
   packet ret;
   strcpy(ret.realname, "SERVER");
   ret.options = MOTD;
   strcpy(ret.buf, server_MOTD);
   ret.timestamp = time(NULL);
   send(fd, &ret, sizeof(ret), 0);
}


/*
 *Exit
 */
void exit_client(int fd) {
   packet ret;
   strcpy(ret.realname, "SERVER");
   ret.options = EXIT;
   strcat(ret.buf, "Closing connection.");
   ret.timestamp = time(NULL);
   printf("Sending close message to %d\n", fd);
   send(fd, &ret, sizeof(ret), 0);
   close(fd);
}


/*
 *Send Message
 */
void send_message(packet *pkt, int clientfd) {
   Room *currentRoom = Rget_roomFID(&room_list, pkt->options);
   printList(&(currentRoom->user_list));
   User *tmp = currentRoom->user_list;
   
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
   User *temp = active_users_list;
   packet ret;
   ret.options = GETALLUSERS;
   strcpy(ret.username, "SERVER");
   while(temp != NULL ) {
      ret.timestamp = time(NULL);
      strcpy(ret.buf, temp->username);
      send(fd, &ret, sizeof(packet), 0);
      memset(&ret.buf, 0, sizeof(ret.buf));
      temp = temp->next;
   }
}


/*
 *Get real name of user requested
 */
void user_lookup(packet *in_pkt, int fd) {
   int i = 0;
   char *args[16];
   char *tmp = in_pkt->buf;

   args[i] = strsep(&tmp, " \t");
   while ((i < sizeof(args) - 1) && (args[i] != '\0')) {
      args[++i] = strsep(&tmp, " \t");
   }
   if (i > 1) {
      packet ret;
      ret.options = GETUSER;
      strcpy(ret.username, "SERVER");
      char *realname = get_real_name(&active_users_list, args[1]);
      if (strcmp(realname, "ERROR") == 0) {
         ret.options = WHOFAIL;
      }
      strcpy(ret.buf, realname);
      ret.timestamp = time(NULL);
      send(fd, &ret, sizeof(packet), 0);
   }
   else {
      printf("%s --- Error:%s Malformed buffer received, ignoring.\n", RED, NORMAL);
   }
}


/*
 *Get users from specific room
 */
void get_room_users(packet *in_pkt, int fd) {
   int i = 0, roomNum;
   char *args[16];
   char *tmp = in_pkt->buf;

   // Split command args
   args[i] = strsep(&tmp, " \t");
   while ((i < sizeof(args) - 1) && (args[i] != '\0')) {
      args[++i] = strsep(&tmp, " \t");
   }
   if (i > 1) {
      roomNum = atoi(args[1]);
      Room *currRoom = Rget_roomFID(&room_list, roomNum);
      if (currRoom != NULL) {
         User *temp = currRoom->user_list;
         packet ret;
         ret.options = GETUSERS;
         strcpy(ret.username, "SERVER");
         while(temp != NULL ) {
            ret.timestamp = time(NULL);
            strcpy(ret.buf, temp->username);
            send(fd, &ret, sizeof(packet), 0);
            memset(&ret.buf, 0, sizeof(ret.buf));
            temp = temp->next;
         }
      }
      else {
         printf("%s --- Error:%s Trying to read user info but room is null.\n", RED, NORMAL);
      }
   }
   else {
      printf("%s --- Error:%s Malformed buffer received, ignoring.\n", RED, NORMAL);
   }
}


/*
 *Get list of rooms
 */
void get_room_list(int fd) {
   Room *temp = room_list;
   packet pkt;
   pkt.options = GETROOMS;
   strcpy(pkt.username, "SERVER");
   while(temp != NULL ) {
      pkt.timestamp = time(NULL);
      strcpy(pkt.buf, temp->name);
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
   
   User *user = get_user(&registered_users_list, pkt->username);
   if (user != NULL) {
      if(strcmp(user->password, args[1]) == 0) {
         memset(user->password, 0, 32);
         strcpy(user->password, args[2]);
         writeUserFile(&registered_users_list, "Users.bin");
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
   User *user = get_user(&registered_users_list, pkt->username);
   //printf("Username: %s, Real name: %s\n", user->username, user->real_name);
   
   if(user != NULL) {
      memset(user->real_name, 0, sizeof(user->real_name));
      strncpy(user->real_name, name, sizeof(name));
      writeUserFile(&registered_users_list, "Users.bin");
      ret.options = NAMESUC;
   }
   else {
      printf("%s --- Error:%s Trying to modify null user in user_list.\n", RED, NORMAL);
      ret.options = NAMEFAIL;
   }
   
   // Submit name change to active users
   user = get_user(&active_users_list, pkt->username);
   if(user != NULL) {
      memset(user->real_name, 0, sizeof(user->real_name));
      strncpy(user->real_name, name, sizeof(name));
   }
   else {
      printf("%s --- Error:%s Trying to modify null user in active_users.\n", RED, NORMAL);
      ret.options = NAMEFAIL;
   }
   
   printList(&registered_users_list);
   printList(&active_users_list);
   ret.timestamp = time(NULL);
   send(fd, &ret, sizeof(packet), 0);
}


/*
 *Join a chat room
 */
void join(packet *pkt, int fd) {
   int i = 0;
   char *args[16];
   char *tmp = pkt->buf;
   packet ret;
   
   // Split command args
   args[i] = strsep(&tmp, " \t");
   while ((i < sizeof(args) - 1) && (args[i] != '\0')) {
      args[++i] = strsep(&tmp, " \t");
   }
   if (i > 1) {
      // check if room exists
      printf("Checking if room exists . . .\n");
      if (Rget_ID(&room_list, args[0]) == -1) {
         // create if it does not exist
         createRoom(&room_list, numRooms, args[0]);
      }
      RprintList(&room_list);  
      printf("Receiving room node for requested room.\n");
      Room *newRoom = Rget_roomFNAME(&room_list, args[0]);
      
      int currRoomNum = atoi(args[1]);
      // Should check if current room exists
      printf("Receiving room node for users current room.\n");
      Room *currentRoom = Rget_roomFID(&room_list, currRoomNum);//pkt->options);
      printf("Getting user node from current room user list.\n");
      User *currUser = get_user(&active_users_list, pkt->username);
      if(currentRoom != NULL) {
         printf("Removing user from his current rooms user list\n");
         removeUser(&(currentRoom->user_list), currUser);
      }
      else {
         printf("Could not remove user: current room is NULL\n");
      }
      currUser = clone_user(currUser);
      printf("Inserting user into new rooms user list\n");
      insertUser(&(newRoom->user_list), currUser);
      
      RprintList(&room_list);  

      ret.options = JOINSUC;
      strcpy(ret.realname, "SERVER");
      sprintf(ret.buf, "%s %d", args[0], newRoom->ID);
      send(fd, (void *)&ret, sizeof(packet), 0);
      memset(&ret, 0, sizeof(ret));
      
      ret.options = newRoom->ID;
      strcpy(ret.realname, "SERVER");
      strncpy(ret.buf, currUser->real_name, sizeof(currUser->real_name));
      strcat(ret.buf, " has joined the room.");
      ret.timestamp = time(NULL);
      send_message(&ret, -1);
   }
   else {
      printf("Problem in join.\n");
   }
}

