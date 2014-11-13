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

   readUserFile(&user_list, "Users.txt");
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


/*
 * 
 *Receives all messages from client. Needs to check
 *if the message is a command or goes to all connected
 *users.
 */
/*
void *client_thread(void *ptr)
{
   session *current_session = (session *)ptr;
   char *timestamp;
   int received;
   int client = current_session->this_client;
   
   //Send username to other client
   strcpy(client_message.buf, current_session->aliases[client]);
   send(current_session->clients[1-client], (void *)&client_message, 
       sizeof(packet), 0); 
   while(1) {
      received = recv(current_session->clients[client], (void *)&client_message,
                             sizeof(packet), 0);
      //Check if other user has ended conversation
      if(received <= 0)
      {
         current_session->running = 0;
         break;
      }
      
      client_message.options = 0;
      //Format timestamp and remove \n
      timestamp = asctime(localtime(&(client_message.timestamp)));
      timestamp[strlen(timestamp) -1] = '\0';
      
      //Print messages as they are recieved, each client in a different color
      if(current_session->running)
      {
         if(client == 0)
         {
            printf("%s%s [%s]:%s%s\n", RED, timestamp, current_session->aliases[client],
                   NORMAL, client_message.buf);
         }
         else if(client == 1)
         {
            printf("%s%s [%s]:%s%s\n", BLUE, timestamp, current_session->aliases[client],
                   NORMAL, client_message.buf);
         }
      }
      
      //Handle "EXIT" message
      if(strcmp(client_message.buf, "EXIT") == 0)
      {
         client_message.options = -1;
         send(current_session->clients[1-client], (void *)&client_message, 
              sizeof(packet), 0);
         printf("Session between %d and %d ended.\n", current_session->clients[0],
            current_session->clients[1]);
         
         current_session->running = 0;
         break;
      }
      
      //Send message to both clients, changing options to display
      //correct username client-side
      else
      {
         client_message.options = 0;
         send(current_session->clients[client], (void *)&client_message,
              sizeof(packet), 0);
         client_message.options = 1;
         send(current_session->clients[1 - client], (void *)&client_message,
              sizeof(packet), 0);
      }
   }

   end(current_session);
   return NULL;
}*/


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
 *Used in the login/registration process for each new connection. 
 *Allows registration of a new user and login of an existing user.
 *Registration: prompts user to enter their desired username, real name,
 *and password.
 *Login: prompts user to enter their username and password
 */
void establish_identity(int fd, char *ID, char *name, User **user_list) {
   packet pck;
   int loop = 1;
   while(loop)
   {
      // Receive command and username
      recv(fd, &pck, sizeof(pck), 0);
      
      // Register
      if(pck.options == 0)
      {
         User *temp_user = (User *)malloc(sizeof(User));
         temp_user->next = NULL;
         strcpy(temp_user->username, pck.buf);
         
         if(strcmp(get_real_name(user_list, temp_user->username), "ERROR") != 0)
         {
            printf("%sERROR - %sUsername %s is already taken\n", RED, NORMAL,
                   temp_user->username);
            strcpy(pck.buf, "ERROR");
            send(fd, &pck, sizeof(pck), 0);
         }
         else
         {
            // Send valid username message
            send(fd, &pck, sizeof(pck), 0);
            
            // Receive and save password
            recv(fd, &pck, sizeof(pck), 0);
            strcpy(temp_user->password, pck.buf);
            
            // Receive and save name
            recv(fd, &pck, sizeof(pck), 0);
            strcpy(temp_user->real_name, pck.buf);
            
            strcpy(ID, temp_user->username);
            insert(user_list, temp_user);
            strcpy(name, pck.buf);
            printf("Client %d successfully registered with username %s and real name %s\n",
                   fd, ID, name);
            loop = 0;
         }
      }
      // Login
      else if(pck.options == 1)
      {
         strcpy(ID, pck.buf);
         
         strcpy(name, get_real_name(user_list, ID));
         if(strcmp(name, "ERROR") != 0) loop = 0;
         strcpy(pck.buf, name);
         send(fd, &pck, sizeof(pck), 0);

         //Ensure correct password
         recv(fd, &pck, sizeof(pck), 0);
  
         while(strcmp(pck.buf, get_password(user_list, ID)) != 0)
         {
            strcpy(pck.buf, "INVALID");
            send(fd, &pck, sizeof(pck), 0);
            recv(fd, &pck, sizeof(pck), 0);
         }
         strcpy(pck.buf, "VALID");
         send(fd, &pck, sizeof(pck), 0);
         
         strcpy(name, get_real_name(user_list, ID));
         if(strcmp(name, "ERROR") != 0) loop = 0;
         strcpy(pck.buf, name);
         send(fd, &pck, sizeof(pck), 0);
         printf("Client %d successfully logged in as %s\n",fd , ID);
      }
      else
      {
         printf("%sERROR - %sInvalid input received", RED, NORMAL);         
      }
   }
   writeUserFile(user_list, "Users.txt");
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
   printf("Got to register\n");
   User *temp_user = (User *)malloc(sizeof(User));
   temp_user->next = NULL;
   char *args[5];
   char *tmp;
   tmp = pkt->buf;

   printf("Attempting string separation\n");
   //Pull command
   args[0] = strsep(&tmp, " \t");

   printf("pulling username\n");
   //Pull username
   args[1] = strsep(&tmp, " \t");
   if(strcmp(get_real_name(&user_list, args[1]), "ERROR") ==0) {
      packet ret;
      ret.options = REGFAIL;
      strcpy(ret.buf, "Username taken.");
      send(fd, &ret, sizeof(ret), 0);
      return;
   }
  
   printf("Pulling password\n");
   //Pull password
   args[2] = strsep(&tmp, " \t");
   strcpy(temp_user->username, args[1]);
   strcpy(temp_user->password, args[2]);
   printf("Going to insert\n");
   temp_user->sock = fd;
   insert(&user_list, temp_user);
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
   if(strcmp(get_real_name(&user_list, args[1]), "ERROR" ==0)) {
      ret.options = LOGFAIL;
      strcpy(ret.buf, "Username not found.");
      send(fd, &ret, sizeof(ret), 0);
      //return;
   }

   //Pull password and check if it is valid
   args[2] = strsep(&tmp, " \t");
   char *password = get_password(&user_list, args[1]);
   if(strcmp(args[2], password) != 0) {
     ret.options = LOGFAIL;
     strcpy(ret.buf, "Incorrect password.");
     send(fd, &ret, sizeof(ret), 0);
     //return;
   }

   //Login successful, send username to client and add to active_users
   User *user = get_user(&user_list, args[1]);
   user->sock = fd;
   insert(&active_users, user);

   ret.options = LOGSUC;
   strcpy(ret.buf, get_real_name(&user_list, args[1]));
   send(fd, &ret, sizeof(ret), 0);
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

}


/*
 *Get active users
 */
void get_active_users(int fd) {
    User *temp = active_users;
    packet pkt;

    while(temp != NULL ) {
     strcpy(pkt.buf, temp->username);
     send(fd, &pkt, sizeof(pkt), 0);
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


}

/*
 *Join a chat room
 */
void join(packet *pkt) {


}
