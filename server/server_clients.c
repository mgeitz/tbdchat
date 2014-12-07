/* 
//   Program:             TBD Chat Server
//   File Name:           server_clients.c
//   Authors:             Matthew Owens, Michael Geitz, Shayne Wierbowski
*/
#include "chat_server.h"

extern int numRooms;
extern pthread_mutex_t registered_users_mutex;
extern pthread_mutex_t active_users_mutex;
extern pthread_mutex_t rooms_mutex;
extern Node *registered_users_list;
extern Node *active_users_list;
extern Node *room_list;
extern char *server_MOTD;


/*
 *Main thread for each client.  Receives all messages
 *and passes the data off to the correct function.  Receives
 *a pointer to the file descriptor for the socket the thread
 *should listen on
 */
void *client_receive(void *ptr) {
   int client = *(int *) ptr;
   int received;
   int logged_in = 0;
   packet in_pkt, *client_message_ptr = &in_pkt;

   while (1) {
      received = recv(client, &in_pkt, sizeof(packet), 0);
      if (received) {
         debugPacket(client_message_ptr);

         // Responses to not logged in clients
         if (!logged_in) {
            if(in_pkt.options == REGISTER) {
               logged_in = register_user(&in_pkt, client);
            }
            else if(in_pkt.options == LOGIN) {
               logged_in = login(&in_pkt, client);
            }
            else if(in_pkt.options == EXIT) {
               close(client);
               return NULL;
            }
            else {
               sendError("Not logged in.", client);
            }
         }

         // Responses to logged in clients
         else if (logged_in) {
            // Handle option messages for logged in client
            if (in_pkt.options < 1000) {
               if(in_pkt.options == REGISTER) { 
                  sendError("You may not register while logged in.", client);
               }
               else if(in_pkt.options == SETPASS) {
                  set_pass(&in_pkt, client);
               }
               else if(in_pkt.options == SETNAME) {
                  set_name(&in_pkt, client);
               }
               else if(in_pkt.options == LOGIN) {
                  sendError("Already logged in.", client);
               }
               else if(in_pkt.options == EXIT) {
                  exit_client(&in_pkt, client);
                  return NULL;
               }
               else if(in_pkt.options == INVITE) {
                  invite(&in_pkt, client);
               }
               else if(in_pkt.options == JOIN) {
                  join(&in_pkt, client);
               }
               else if(in_pkt.options == LEAVE) {
                  leave(&in_pkt, client);
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
               else if(in_pkt.options == 0) {
                  printf("%s --- Error:%s Abrupt disconnect on logged in client.\n", RED, NORMAL);
                  exit_client(&in_pkt, client);
                  return NULL;
               }
               else {
                  printf("%s --- Error:%s Unknown message received from client.\n", RED, NORMAL);
	       }
            }
            // Handle conversation message for logged in client
            else {
               // Will be treated as a message packet, safe to santize entire buffer
               sanitizeInput((void *)&in_pkt.buf, 0);
               send_message(&in_pkt, client);
            }
         }

         memset(&in_pkt, 0, sizeof(packet));
      }
   }
   return NULL;
}


/* Send an error message to a client */
void sendError(char *error, int clientfd) {
   packet ret;
   ret.timestamp = time(NULL);
   ret.options = SERV_ERR;
   strcpy(ret.username, SERVER_NAME);
   strcpy(ret.realname, SERVER_NAME);
   strcpy(ret.buf, error);
   send(clientfd, &ret, sizeof(packet), MSG_NOSIGNAL);
}


/* Replace any char in buffer not in safe_chars, return number of unsafe chars changed */
int sanitizeInput(char *buf, int type) {
   int i = 0;
  // Allowable chars for message buffer
   char const safe_buf_chars[] = "abcdefghijklmnopqrstuvwxyz"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 " _,.-/@()*~`&^%$#!?<>'\";:+=[]{}|"
                                 "1234567890";
  // Allowable chars for realname, username*, roomname*.. 
  // *[provided they have been strsep'ed first]
   char const safe_name_chars[] = "abcdefghijklmnopqrstuvwxyz"
                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                  " _-"
                                  "1234567890";
   char *end = buf + strlen(buf);
   if (type == 0) {
      for (buf += strspn(buf, safe_buf_chars); buf != end; buf += strspn(buf, safe_buf_chars)) {
         *buf = '_';
         i++;
      }
   }
   else {
      for (buf += strspn(buf, safe_name_chars); buf != end; buf += strspn(buf, safe_name_chars)) {
         *buf = '_';
         i++;
      }
   }
   return i;
}


/*
 *Register
 */
int register_user(packet *in_pkt, int fd) {
   int i = 0;
   char *args[16];
   char cpy[BUFFERSIZE];
   char *tmp = cpy;
   strcpy(tmp, in_pkt->buf);

   args[i] = strsep(&tmp, " \t");
   while ((i < sizeof(args) - 1) && (args[i] != '\0')) {
       args[++i] = strsep(&tmp, " \t");
   }
   // Check there are enough arguements to safely inspect them
   if (i > 3) {
      // Ensure requested username is valid
      if (!validUsername(args[1], fd)) { return 0; }
      // Check if the requested username is unique
      if(strcmp(get_real_name(&registered_users_list, args[1], registered_users_mutex), "ERROR") !=0 || \
                              !(strcmp(SERVER_NAME, args[1])) || \
                              strcmp(args[2], args[3]) != 0) {
         sendError("Username unavailable.", fd);
         return 0;
      }
      // Ensure password requested is valid
      if (!validPassword(args[2], args[3], fd)) { return 0; }

      // Allocate memory space for new user node, populate node with new user data
      User *user = (User *)malloc(sizeof(User));
      strcpy(user->username, args[1]);
      strcpy(user->real_name, args[1]);
      // Hash password
      SHA256_CTX sha256;
      SHA256_Init(&sha256);
      SHA256_Update(&sha256, args[2], strlen(args[2]));
      SHA256_Final(user->password, &sha256);
      user->sock = fd;
      user->next = NULL;
      
      // Insert user as registered user, write new user data to file
      insertUser(&registered_users_list, user, registered_users_mutex);
      writeUserFile(&registered_users_list, USERS_FILE, registered_users_mutex);

      // Reform packet as valid login, pass new user data to login
      memset(&in_pkt->buf, 0, sizeof(in_pkt->buf));
      sprintf(in_pkt->buf, "/login %s %s", args[1], args[2]);
      return login(in_pkt, fd);
   }
   // There were not enough arguements received to correctly read them
   else {
      printf("%s --- %sError:%s Malformed reg packet received from %s on %d, ignoring.\n", \
             WHITE, RED, NORMAL, args[1], fd);
   }
   return 0;
}


/*
 *Login
 */
int login(packet *pkt, int fd) {
   int i = 0;
   char *args[16];
   char cpy[BUFFERSIZE];
   char *tmp = cpy;
   unsigned char *arg_pass_hash = (unsigned char *)malloc(SHA256_DIGEST);
   strcpy(tmp, pkt->buf);

   args[i] = strsep(&tmp, " \t");
   while ((i < sizeof(args) - 1) && (args[i] != '\0')) {
       args[++i] = strsep(&tmp, " \t");
   }
   // Check there are enough arguements to safely inspect them
   if (i > 2) {
      packet ret;
      // Check if user exists as registered user
      if (strcmp(get_real_name(&registered_users_list, args[1], registered_users_mutex), "ERROR") == 0) {
         sendError("Username not found.", fd);
         return 0;
      }
      // Retreive password for requested user
      unsigned char *password = get_password(&registered_users_list, args[1], registered_users_mutex);
      // Hash login password arg
      SHA256_CTX sha256;
      SHA256_Init(&sha256);
      SHA256_Update(&sha256, args[2], strlen(args[2]));
      SHA256_Final(arg_pass_hash, &sha256);

      // Compare pass arg and stored pass
      if (comparePasswords(password, arg_pass_hash, 32) != 0) {
         sendError("Incorrect password.", fd);
         free(arg_pass_hash);
         return 0;
      }
      free(arg_pass_hash);

      // Login input is valid, read user data from registered users
      User *user = get_user(&registered_users_list, args[1], registered_users_mutex);
      user->sock = fd;
      user->roomID = 1000;

      //Create node for active users list
      Node *new_usr_act = (Node *)malloc(sizeof(Node));
      new_usr_act->data = (void *)user;
      new_usr_act->next = NULL;

      //Create node for room list
      Node *new_usr_rm = (Node *)malloc(sizeof(Node));
      new_usr_rm->data = (void *)user;
      new_usr_rm->next = NULL;

      // Check if the user is already logged in
      if(insertUser(&active_users_list, user, active_users_mutex) == 1) {
         // Login successful, add user to default room
         Room *defaultRoom = Rget_roomFID(&room_list, DEFAULT_ROOM, rooms_mutex);
         insertNode(&(defaultRoom->user_list), new_usr_rm, defaultRoom->user_list_mutex);

         // Inform client of successful login
         strcpy(ret.realname, get_real_name(&registered_users_list, args[1], registered_users_mutex));
         strcpy(ret.username, args[1]);
         ret.options = LOGSUC;
         //printf("%s logged in\n", ret.username);
         ret.timestamp = time(NULL);
         send(fd, &ret, sizeof(packet), MSG_NOSIGNAL);

         // Inform lobby of successful login
         memset(&ret, 0, sizeof(packet));
         ret.options = DEFAULT_ROOM;
         strcpy(ret.realname, SERVER_NAME);
         strcpy(ret.username, SERVER_NAME);
         sprintf(ret.buf, "%s has joined the lobby.", user->real_name);
         ret.timestamp = time(NULL);
         send_message(&ret, -1);

         // Send MOTD to client
         sendMOTD(fd);
         return 1;
      }
      // Valid login data received, but user is already in active users
      else {
         sendError("User already logged in.", fd);
         printf("%s log in failed: already logged in", args[1]);
         free(user);
         return 0;
      }
   }
   // Not enough arguements received to properly parse input, ignore it
   else {
      printf("%s --- %sError:%s Malformed login packet received from %s on %d, ignoring.\n", \
             WHITE, RED, NORMAL, args[1], fd);
   }
   return 0;
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
      Room *currRoom = Rget_roomFID(&room_list, roomNum, rooms_mutex);
      if (currRoom != NULL) {
         User *inviteUser = get_user(&active_users_list, args[0], active_users_mutex);
         if (inviteUser != NULL) {
            ret.options = INVITE;
            ret.timestamp = time(NULL);
            strcpy(ret.username, SERVER_NAME);
            strcpy(ret.realname, in_pkt->realname);
            memset(&ret.buf, 0, sizeof(ret.buf));
            sprintf(ret.buf, "%s has invited you to join %s", \
                    in_pkt->realname, Rget_name(&room_list, roomNum, rooms_mutex));
            send(inviteUser->sock, &ret, sizeof(packet), MSG_NOSIGNAL);
            memset(&ret, 0, sizeof(packet));
            ret.options = INVITESUC;
            strcpy(ret.username, SERVER_NAME);
            strcpy(ret.realname, SERVER_NAME);
            ret.timestamp = time(NULL);
            send(fd, &ret, sizeof(packet), MSG_NOSIGNAL);
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
   ret.options = SERV_ERR;
   strcpy(ret.username, SERVER_NAME);
   strcpy(ret.realname, SERVER_NAME);
   ret.timestamp = time(NULL);
   sprintf(ret.buf, "An invitation could not be sent to %s.", args[0]);
   send(fd, &ret, sizeof(packet), MSG_NOSIGNAL);
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
   if (i > 1 && validRoomname(args[0], fd)) {
      // check if room exists
      printf("Checking if room exists . . .\n");
      if (Rget_ID(&room_list, args[0], rooms_mutex) == -1) {
         // create if it does not exist
         createRoom(&room_list, numRooms, args[0], rooms_mutex);
      }
      RprintList(&room_list, rooms_mutex);
      printf("Receiving room node for requested room.\n");
      Room *newRoom = Rget_roomFNAME(&room_list, args[0], rooms_mutex);

      int currRoomNum = atoi(args[1]);
      // Should check if current room exists
      printf("Receiving room node for users current room.\n");
      Room *currentRoom = Rget_roomFID(&room_list, currRoomNum, rooms_mutex);//pkt->options);
      printf("Getting user node from current room user list.\n");
      if(currentRoom == NULL) {
         printf("Could not remove user: current room is NULL\n");
      }
      else {
         User *currUser = get_user(&(currentRoom->user_list), pkt->username, currentRoom->user_list_mutex);
         printf("Removing user from his current rooms user list\n");
         removeUser(&(currentRoom->user_list), currUser, currentRoom->user_list_mutex);
         printf("User removed from current room\n");
         
         //Create node to add user to other room list.
         Node *new_node = (Node *)malloc(sizeof(Node));
         new_node->data = currUser;
         currUser->roomID = newRoom->ID;
         printf("Inserting user into new rooms user list\n");
         insertUser(&(newRoom->user_list), currUser, newRoom->user_list_mutex);

         RprintList(&room_list, rooms_mutex);

         ret.options = JOINSUC;
         strcpy(ret.realname, SERVER_NAME);
         strcpy(ret.username, SERVER_NAME);
         ret.timestamp = time(NULL);
         sprintf(ret.buf, "%s %d", args[0], newRoom->ID);
         send(fd, (void *)&ret, sizeof(packet), MSG_NOSIGNAL);
         memset(&ret, 0, sizeof(ret));

         ret.options = currRoomNum;
         strcpy(ret.realname, SERVER_NAME);
         strcpy(ret.username, SERVER_NAME);
         strncpy(ret.buf, currUser->real_name, sizeof(currUser->real_name));
         strcat(ret.buf, " has left the room.");
         ret.timestamp = time(NULL);
         send_message(&ret, -1);
         memset(&ret, 0, sizeof(ret));

         ret.options = newRoom->ID;
         strcpy(ret.realname, SERVER_NAME);
         strcpy(ret.username, SERVER_NAME);
         strncpy(ret.buf, currUser->real_name, sizeof(currUser->real_name));
         strcat(ret.buf, " has joined the room.");
         ret.timestamp = time(NULL);
         send_message(&ret, -1);
      }
   }
   else {
      printf("Problem in join.\n");
      sendError("We were unable to put you in that room, sorry.", fd);
   }
}


/* Remove a user from their current room and move them to lobby */
void leave(packet *pkt, int fd) {
   int i = 0, roomNum;
   char *args[16];
   char *tmp = pkt->buf;
   packet ret;

   // Split command args
   args[i] = strsep(&tmp, " \t");
   while ((i < sizeof(args) - 1) && (args[i] != '\0')) {
      args[++i] = strsep(&tmp, " \t");
   }
   if (i > 1) {
      roomNum = atoi(args[1]);
      // If user is not in the lobby
      if (roomNum != DEFAULT_ROOM) {
         // Get current room information
         Room *currRoom = Rget_roomFID(&room_list, roomNum, rooms_mutex);
         if (currRoom != NULL) {
            // Find users node in room
            User *currUser = get_user(&(currRoom->user_list), pkt->username, currRoom->user_list_mutex);
            if (currUser != NULL) {
               // Remove user from their current room
               removeUser(&(currRoom->user_list), currUser, currRoom->user_list_mutex);

               //Create node to add user back to lobby
               Node *new_node = (Node *)malloc(sizeof(Node));
               new_node->data = currUser;

               // Place user in lobby room
               Room *defaultRoom = Rget_roomFID(&room_list, DEFAULT_ROOM, rooms_mutex);
               currUser->roomID = 1000;
               insertUser(&(defaultRoom->user_list), currUser, defaultRoom->user_list_mutex);

               // Send user leave message to room
               ret.options = roomNum;
               strcpy(ret.realname, SERVER_NAME);
               strcpy(ret.username, SERVER_NAME);
               strncpy(ret.buf, currUser->real_name, sizeof(currUser->real_name));
               strcat(ret.buf, " has left the room.");
               ret.timestamp = time(NULL);
               send_message(&ret, -1);
               memset(&ret, 0, sizeof(ret));

               // Send join success to client
               ret.options = JOINSUC;
               strcpy(ret.realname, SERVER_NAME);
               strcpy(ret.username, SERVER_NAME);
               sprintf(ret.buf, "%s %d", defaultRoom->name, defaultRoom->ID);
               strcat(ret.buf, " has joined the room.");
               ret.timestamp = time(NULL);
               send(fd, (void *)&ret, sizeof(packet), MSG_NOSIGNAL);
               memset(&ret, 0, sizeof(ret));

               // Send join notification to lobby room
               ret.options = defaultRoom->ID;
               strcpy(ret.realname, SERVER_NAME);
               strcpy(ret.username, SERVER_NAME);
               strncpy(ret.buf, currUser->real_name, sizeof(currUser->real_name));
               strcat(ret.buf, " has joined the room.");
               ret.timestamp = time(NULL);
               send_message(&ret, -1);
            }
         }
      }
   }
}


/*
 *Set user real name
 */
void set_name(packet *pkt, int fd) {
   char name[64];
   packet ret;
   
      strncpy(name, pkt->buf, sizeof(name)); 
      name[strlen(pkt->buf)] = '\0';
      if (!validRealname(name, fd)) { return; }

      //Submit name change to user list, write list
      User *user = get_user(&registered_users_list, pkt->username, registered_users_mutex);

      if(user != NULL) {
         strncpy(ret.buf, user->real_name, sizeof(user->real_name));
         memset(user->real_name, 0, sizeof(user->real_name));
         strncpy(user->real_name, name, sizeof(name));
         writeUserFile(&registered_users_list, USERS_FILE, registered_users_mutex);
      
         //printf("RIGHT BEFORE ATOI %s\n", args[i - 1]);
         ret.options = user->roomID;
         strcpy(ret.realname, SERVER_NAME);
         strcpy(ret.username, SERVER_NAME);
         strcat(ret.buf, " is now known as ");
         strcat(ret.buf, user->real_name);
         ret.timestamp = time(NULL);
         //printf("HERE%s %dy\n", ret.buf, ret.options);
         send_message(&ret, fd);
         memset(&ret, 0, sizeof(ret));
      
         strncpy(ret.buf, name, sizeof(ret.buf));
         ret.options = NAMESUC;
      }
      else {
         printf("%s --- Error:%s Trying to modify null user in user_list.\n", RED, NORMAL);
         strcpy(ret.buf, "Name change failed, for some reason we couldn't find you.");
         ret.options = SERV_ERR;
      }

      strcpy(ret.realname, SERVER_NAME);
      strcpy(ret.username, SERVER_NAME);
      ret.timestamp = time(NULL);
      send(fd, &ret, sizeof(packet), MSG_NOSIGNAL);
}


/*
 *Set user password
 */
void set_pass(packet *pkt, int fd) {
   int i = 0;
   char *args[16];
   char cpy[BUFFERSIZE];
   char *tmp = cpy;
   unsigned char *curr_pass_hash = (unsigned char *)malloc(SHA256_DIGEST);
   strcpy(tmp, pkt->buf);

   args[i] = strsep(&tmp, " \t");
   while ((i < sizeof(args) - 1) && (args[i] != '\0')) {
       args[++i] = strsep(&tmp, " \t");
   }
   if (i > 3) {
      if (!validPassword(args[2], args[3], fd)) { 
         free(curr_pass_hash); 
         return; 
      }
      User *user = get_user(&registered_users_list, pkt->username, registered_users_mutex);
      if (user != NULL) {
         // Hash for pw compare
         SHA256_CTX sha256;
         SHA256_Init(&sha256);
         SHA256_Update(&sha256, args[1], strlen(args[1]));
         SHA256_Final(curr_pass_hash, &sha256);
         if (comparePasswords(user->password, curr_pass_hash, 32) == 0) {
            memset(user->password, 0, 32);
            // Hash new password
            SHA256_CTX sha256;
            SHA256_Init(&sha256);
            SHA256_Update(&sha256, args[2], strlen(args[2]));
            SHA256_Final(user->password, &sha256);
            //pthread_mutex_lock(&registered_users_mutex);
            writeUserFile(&registered_users_list, USERS_FILE, registered_users_mutex);
            //pthread_mutex_unlock(&registered_users_mutex);
            pkt->options = PASSSUC;
         }
         else {
            pkt->options = SERV_ERR;
            strcpy(pkt->buf, "Password change failed, password mismatch.");
         }
      }
      else {
         pkt->options = SERV_ERR;
         strcpy(pkt->buf, "Password change failed, for some reason we couldn't find you.");
      }
   }
   else {
      pkt->options = SERV_ERR;
      strcpy(pkt->buf, "Password change failed, malformed request.");
   }
   free(curr_pass_hash); 
   strcpy(pkt->username, SERVER_NAME);
   strcpy(pkt->realname, SERVER_NAME);
   pkt->timestamp = time(NULL);
   send(fd, (void *)pkt, sizeof(packet), MSG_NOSIGNAL);
}


/* Check requested username */
int validUsername (char *username, int client) {
   if (sanitizeInput(username, 1)) {
      sendError("Invalid characters in username.", client);
      return 0;
   }
   if (strlen(username) < 3) {
      sendError("Username is too short.", client);
      return 0;
   }
   if (strlen(username) > USERNAME_LENGTH) {
      sendError("Username is too long.", client);
      return 0;
   }
   return 1;
}


/* Check requested realname */
int validRealname (char *realname, int client) {
   if (sanitizeInput(realname, 1)) {
      sendError("Invalid characters in requested name.", client);
      return 0;
   }
   if (strlen(realname) < 3) {
      sendError("Requested name is too short.", client);
      return 0;
   }
   if (strlen(realname) > REALNAME_LENGTH) {
      sendError("Requested name is too long.", client);
      return 0;
   }
   return 1;
}


/* Check password input */
int validPassword (char *pass1, char *pass2, int client) {
   if (strcmp(pass1, pass2)) {
      sendError("Password input does not match.", client);
      return 0;
   }

   if (strlen(pass1) < 3) {
      sendError("Requested password is too short.", client);
      return 0;
   }
   if (strlen(pass1) > 32) {
      sendError("Requested password is too long.", client);
      return 0;
   }
   return 1;
}


/* Check requested room name */
int validRoomname (char *roomname, int client) {
   if (sanitizeInput(roomname, 1)) {
      sendError("Invalid characters in room name.", client);
      return 0;
   }
   if (strlen(roomname) < 3) {
      sendError("Requested room name is too short.", client);
      return 0;
   }
   if (strlen(roomname) > ROOMNAME_LENGTH) {
      sendError("Requested room name is too long.", client);
      return 0;
   }
   return 1;
}


/*
 *Exit
 */
void exit_client(packet *pkt, int fd) {
   packet ret;
   User *current;

   //Remove user from their current room and the active user's list
   current = get_user(&active_users_list, pkt->username, active_users_mutex);
   if(current != NULL) {
      //Send disconnect message to user room
      ret.options = current->roomID;
      strcpy(ret.realname, SERVER_NAME);
      strcpy(ret.username, SERVER_NAME);
      sprintf(ret.buf, "User %s has disconnected.", pkt->realname);
      ret.timestamp = time(NULL);
      send_message(&ret, -1);

      memset(&ret, 0, sizeof(packet));
      strcpy(ret.realname, SERVER_NAME);
      strcpy(ret.username, SERVER_NAME);
      ret.options = EXIT;
      strcat(ret.buf, "Goodbye!");
      ret.timestamp = time(NULL);
      printf("Sending close message to %d\n", fd);
      send(fd, &ret, sizeof(packet), MSG_NOSIGNAL);
      close(fd);

      Room *room = Rget_roomFID(&room_list, current->roomID, rooms_mutex);
      printf("got room\n");
      removeUser(&(room->user_list), current, room->user_list_mutex); 
      printf("removed user from current room\n");
      removeUser(&active_users_list, current, active_users_mutex);
      printf("removed user from active users\n");
   }
}


/*
 *Send Message
 */
void send_message(packet *pkt, int clientfd) {
   Room *currentRoom = Rget_roomFID(&room_list, pkt->options, rooms_mutex);
   log_message(pkt, currentRoom->fd);
   printList(&(currentRoom->user_list), currentRoom->user_list_mutex);
   Node *tmp = currentRoom->user_list;
   User *current;
   while(tmp != NULL) {
      current = (User *)tmp->data;
      if (clientfd != current->sock) {
         send(current->sock, (void *)pkt, sizeof(packet), MSG_NOSIGNAL);
      }
      tmp = tmp->next;
   }
   // extra unlock????
   pthread_mutex_unlock(&currentRoom->user_list_mutex);
}


/* Send the server MOTD to the socket passed in */
void sendMOTD(int fd) {
   packet ret;
   strcpy(ret.realname, SERVER_NAME);
   ret.options = MOTD;
   strcpy(ret.buf, server_MOTD);
   ret.timestamp = time(NULL);
   send(fd, &ret, sizeof(packet), MSG_NOSIGNAL);
}


/*
 *Get active users
 */
void get_active_users(int fd) {
   packet ret;

   ret.options = GETALLUSERS;
   strcpy(ret.username, SERVER_NAME);
   strcpy(ret.realname, SERVER_NAME);
   
   int num_users = listLength(&active_users_list, active_users_mutex);
   sprintf(ret.buf, "%d users online", num_users);
   ret.timestamp = time(NULL);
   send(fd, &ret, sizeof(packet), MSG_NOSIGNAL);
   memset(&ret.buf, 0, sizeof(ret.buf));

   pthread_mutex_lock(&active_users_mutex);
   Node *temp = active_users_list;
   User *current;
   while(temp != NULL ) {
      current = (User *) temp->data;
      ret.timestamp = time(NULL);
      sprintf(ret.buf, "%s-%s", current->username, current->real_name);
      send(fd, &ret, sizeof(packet), MSG_NOSIGNAL);
      memset(&ret.buf, 0, sizeof(ret.buf));
      temp = temp->next;
   }
   pthread_mutex_unlock(&active_users_mutex);
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
      strcpy(ret.username, SERVER_NAME);
      strcpy(ret.realname, SERVER_NAME);
      char *realname = get_real_name(&active_users_list, args[1], active_users_mutex);
      if (strcmp(realname, "ERROR") == 0) {
         ret.options = SERV_ERR;
         sprintf(ret.buf, "%s not found.", args[1]);
         ret.timestamp = time(NULL);
         send(fd, &ret, sizeof(packet), MSG_NOSIGNAL);
      }
      else {
         sprintf(ret.buf, "User Lookup");
         ret.timestamp = time(NULL);
         send(fd, &ret, sizeof(packet), MSG_NOSIGNAL);
         memset(&ret.buf, 0, sizeof(ret.buf));

         strcpy(ret.buf, realname);
         sprintf(ret.buf, "%s-%s", args[1], realname);
         send(fd, &ret, sizeof(packet), MSG_NOSIGNAL);
      }
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
      Room *currRoom = Rget_roomFID(&room_list, roomNum, rooms_mutex);
      if (currRoom != NULL) {
         packet ret;
         int num_users = listLength(&currRoom->user_list, currRoom->user_list_mutex);

         ret.options = GETUSERS;
         strcpy(ret.username, SERVER_NAME);
         strcpy(ret.realname, SERVER_NAME);

         sprintf(ret.buf, "%d users in %s", num_users, currRoom->name);
         ret.timestamp = time(NULL);
         send(fd, &ret, sizeof(packet), MSG_NOSIGNAL);
         memset(&ret.buf, 0, sizeof(ret.buf));
  
         pthread_mutex_lock(&currRoom->user_list_mutex);
         Node *temp = currRoom->user_list;
         User *current;
         while(temp != NULL ) {
            current = (User *)temp->data;
            ret.timestamp = time(NULL);
            sprintf(ret.buf, "%s-%s", current->username, current->real_name);
            send(fd, &ret, sizeof(packet), MSG_NOSIGNAL);
            memset(&ret.buf, 0, sizeof(ret.buf));
            temp = temp->next;
         }
         pthread_mutex_unlock(&currRoom->user_list_mutex);
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
   packet pkt;

   pkt.options = GETROOMS;
   strcpy(pkt.username, SERVER_NAME);
   strcpy(pkt.realname, SERVER_NAME);
   pkt.timestamp = time(NULL);
   int num_rooms = listLength(&room_list, rooms_mutex);
   sprintf(pkt.buf, "%d Rooms Found", num_rooms);
   send(fd, &pkt, sizeof(pkt), MSG_NOSIGNAL);
   memset(&pkt.buf, 0, sizeof(pkt.buf));

   pthread_mutex_lock(&rooms_mutex);
   Node  *temp = room_list;
   Room *current;
   while(temp != NULL ) {
      current = (Room *)temp->data;
      pkt.timestamp = time(NULL);
      strcpy(pkt.buf, current->name);
      send(fd, &pkt, sizeof(pkt), MSG_NOSIGNAL);
      memset(&pkt.buf, 0, sizeof(pkt.buf));
      temp = temp->next;
   }
   pthread_mutex_unlock(&rooms_mutex);
}


/* Reliably compare unsigned char arrays (debug printfs output a lot) */
int comparePasswords(unsigned char *pass1, unsigned char *pass2, int size) {
   int i = 0;
   //printf("-----------------------------------------------------------\n");
   //printf("PREPARE PASS COMPARE OF %d. %d of what I don't know\n\n", size, size); 

   for (i = 0; i < size; i++) {
       //printf("PASS1: %02x\n", pass1[i]);
       //printf("PASS2: %02x\n\n", pass2[i]);
      if (pass1[i] != pass2[i]) { return 1; }
   }
   //printf("-----------------------------------------------------------\n\n");
   return 0;
}


/*
 *Logs the given message packet to the given fd
 */
void log_message(packet *pkt, int fd) {
   char *temp = (char*)malloc(256 * sizeof(char));
   strcpy(temp, asctime(localtime(&(pkt->timestamp))));
   temp[strlen(temp) - 1] = ' ';
   strncat(temp, "| [", 3);
   strncat(temp, pkt->realname, strlen(pkt->username));
   strncat(temp, "] ", 2);
   strncat(temp, pkt->buf, strlen(pkt->buf));
   strncat(temp, "\n", 2);
   write(fd, temp, strlen(temp) * sizeof(char));
   printf("%s", temp);
}


