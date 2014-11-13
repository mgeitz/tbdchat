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
 
int main(int argc, char **argv) {
   if(argc < 3) {
      printf("%s --- Error:%s Usage: %s IP_ADDRESS PORT.\n", RED, NORMAL, argv[0]);
      exit(0);
   }
   
   // Handle CTRL+C
   signal(SIGINT, sigintHandler);
   
   
   user_list = NULL;
   active_users = NULL;

   readUserFile(&user_list, "Users.bin");
   printList(&user_list);  
   // Open server socket
   chat_serv_sock_fd = get_server_socket(argv[1], argv[2]);
   
   // step 3: get ready to accept connections
   if(start_server(chat_serv_sock_fd, BACKLOG) == -1) {
      printf("start server error\n");
      exit(1);
   }	
   /*while (1) {
      // Initialize memory space for new clientInfo node
      clientInfo newClient;
      // Store client sockfd in clientInfo  node
      newClient.sockfd = accept_client(chat_serv_sock_fd);
      if(newClient.sockfd != -1) {
         int iret;
         // Set newClient default logged in value
         newClient.logged_in = 0;
         // Set newClient default room
         newClient.currentRoom = 0;
         // Probably should insert newClient node into list of clients (in default room maybe)

         // Launch client recaive thread
         iret  = pthread_create(&newClient.thread_ID, NULL, client_receive, (void *)&newClient);
     }
      
   }*/
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
void end(session *ptr)
{
   close(ptr->clients[0]);
   close(ptr->clients[1]);
   free(ptr);
}

/* Handle SIGINT (CTRL+C) */
void sigintHandler(int sig_num)
{
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
   // Just for testing
   if (received) { 
      debugPacket(client_message_ptr); 
      //printf("Client Info: %lu %d %d %d", (unsigned long)client.thread_ID, client.sockfd, client.logged_in, client.currentRoom);
   }

   //Handle command messages
   if(in_pkt.options == REGISTER) { 
      printf("register\n");
      register_user(&in_pkt, client);
   }
   else if(in_pkt.options == SETPASS) {
      printf("setpass\n");
      set_pass(&in_pkt, client);
   }
   else if(in_pkt.options == SETNAME) {
      printf("setname\n");
      set_name(&in_pkt, client);
   }
   else if(in_pkt.options == LOGIN) {
      printf("login\n");
      login(&in_pkt, client);
   }
   else if(in_pkt.options == EXIT) {
      printf("exit\n");
      exit_client(&in_pkt);
   }
   else if(in_pkt.options == INVITE) {
      printf("invite\n");
      invite(&in_pkt);
   }
   else if(in_pkt.options == JOIN) { 
      printf("join\n");
      join(&in_pkt);
   }
   else if(in_pkt.options == GETUSERS) {
      printf("getusers\n");
      get_active_users(client);
   }
   //Handle conversation message
   else if(in_pkt.options >= 1000) { 
      printf("message\n");
      send_message(&in_pkt);
   }
   //Unrecognized command, send client a failure message
   //else {
   //   packet ret;
   //   ret.options = RECFAIL;
   //   strcpy(ret.buf, "Invalid Command");
   //   send(client, &ret, sizeof(ret), 0);
   //}
   memset(&in_pkt, 0, sizeof(packet));
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

   //Pull command
   args[0] = strsep(&tmp, " \t");

   //Pull username
   args[1] = strsep(&tmp, " \t");
   if(strcmp(get_real_name(&user_list, args[1]), "ERROR") !=0) {
      packet ret;
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
 
   writeUserFile(&user_list, "Users.bin");
   printf("New User Registered\n");
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

   //printf("-------BEFORE FIRST LOOKUP-------\n");
   //printList(&user_list);
   //Pull username and check valid
   args[1] = strsep(&tmp, " \t");
   if(strcmp(get_real_name(&user_list, args[1]), "ERROR") ==0) {
      ret.options = LOGFAIL;
      ret.timestamp = time(NULL);
      strcpy(ret.alias, "SERVER");
      strcpy(ret.buf, "Username not found.");
      send(fd, &ret, sizeof(ret), 0);
      return;
   }
   //printf("-------AFTER FIRST LOOKU---------\n");
   //printList(&user_list);
   //Pull password and check if it is valid
   args[2] = strsep(&tmp, " \t");
   //printf("--------BEFORE GET PASS--------\n");
   //printList(&user_list);

   char *password = get_password(&user_list, args[1]);
   //printf("---------AFTER GETPASS--------\n");
   //printList(&user_list);
   if(strcmp(args[2], password) != 0) {
     ret.options = LOGFAIL;
     ret.timestamp = time(NULL);
     strcpy(ret.alias, "SERVER");
     strcpy(ret.buf, "Incorrect password.");
     send(fd, &ret, sizeof(ret), 0);
     return;
   }
   //printf("--------BEFORE GET USER--------\n");
   //printList(&user_list);
   //Login successful, send username to client and add to active_users
   User *user = (User *)malloc(sizeof(User));
   strcpy(user->username, args[1]);
   user->sock = fd;
   user->next = NULL;
   //printf("----------AFTER GET USER--------\n");
   //printList(&user_list);
   user->sock = fd;
   insert(&active_users, user);
   //printf("--------ACTIVE USERS--------\n");
   //printList(&active_users);
   ret.options = LOGSUC;
   ret.timestamp = time(NULL);
   strcpy(ret.buf, "SERVER");
   //printf("---------BEFORE LAST GET REAL NAME--------\n");
   //printList(&user_list);
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
void send_message(packet *pkt) {
    User *tmp = active_users;
    pkt->options = 1001;
    while(tmp != NULL) {
       send(tmp->sock, pkt, sizeof(pkt), 0);
       printf("Sent to %s\n", tmp->username);
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

    strcpy(pkt.buf, "END");
    send(fd, &pkt, sizeof(pkt), 0);

}

/*
 *Set user password
 */
void set_pass(packet *pkt, int fd) {


}

/*
 *Set user real name
 */
void set_name(packet *pkt, int fd) {
   //Submit name change to user list, write list
   User *user = get_user(&user_list, pkt->alias);
   if(user != NULL) {
      strcpy(user->real_name, pkt->buf);
      writeUserFile(&user_list, "Users.bin");
   }
   
   //Submit name change to active users
   user = get_user(&active_users, pkt->alias);
   if(user != NULL) {
      strcpy(user->real_name, pkt->buf);
   }
}

/*
 *Join a chat room
 */
void join(packet *pkt) {


}
