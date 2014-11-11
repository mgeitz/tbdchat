/* Program:             Simple Chat Client
   Authors:             Matthew Owens, Michael Geitz, Shayne Wierbowski
   Date:                10/23/2014
   File Name:           chat_client.c
   Compile:             gcc -o chat_client chat_client.c -l pthread
   Run:                 ./chat_client IP_ADDRESS PORT

   The client program for a simple two way chat utility

*/
#include "chat_client.h"
int serverfd;

int main() {
   pthread_t chat_rx_thread;
   int bufSize, send;
   char username[64];
   char alias[32];
   
   // Handle CTRL+C
   signal(SIGINT, sigintHandler);
   
   packet tx_pkt;
   
   while (1) {
      strcpy(tx_pkt.alias, username);
      bufSize = userInput(&tx_pkt.buf);
      send = 1;
      if(bufSize > 0 && tx_pkt.buf[bufSize] != EOF) {
         if(strncmp("/", (void *)tx_pkt.buf, 1) == 0) {
             send = userCommand(&tx_pkt);
         }
      if (send && serverfd)
            tx_pkt.timestamp = time(NULL);
            send(serverfd, (void *)&tx_pkt, sizeof(packet), 0);
      }
      // Wipe packet
      memset(&tx_pkt, 0, sizeof(packet));
   }


   
   // Close connection
   //if(pthread_join(chat_rx_thread, NULL))
   //{
   //   printf("%s --- Error:%s chatRX thread not joining.\n", RED, NORMAL);
   //}
   //printf("Exiting.\n");
   //close(conn);
   //exit(0);
}

/* Process user commands and mutate buffer accordingly */
void userCommand(packet *tx_pkt) {
   int i = 0;
   char *argv[16];
   char *tmp;

   tmp = tx_pkt.buf; // Required for strsep

   // Handle exit command
   if (strncmp((void *)tx_pkt.buf, "/exit", strlen("/exit")) == 0) {
       exit(1);
   }
   // Handle help command
   else if (strncmp((void *)tx_pkt.buf, "/help", strlen("/help")) == 0) {
       showHelp();
       return 0;
   }
   // Handle connect command
   else if (strncmp((void *)tx_pkt.buf, "/connect", strlen("/connect")) == 0) {
      if (!newServerConnection(&tx_pkt.buf)) {
          printf("%s --- Error:%s Server connect failed.\n", RED, NORMAL);
      }
      return 0;
   }
   // Handle register command
   else if (strncmp((void *)tx_pkt.buf, "/register", strlen("/register")) == 0) {
      if (!serverRegistration(&tx_pkt)) {
          printf("%s --- Error:%s Server registration failed.\n", RED, NORMAL);
         return 0;
      }
      else {
         return 1;
      }
   }
   // Handle login command
   else if (strncmp((void *)tx_pkt.buf, "/login", strlen("/login")) == 0) {
      // change option and remove /login from buffer
      return 1;
   }
  // If it wasn't any of that, invalid command
  else {
      printf("%s --- Error:%s Invalid command.\n", RED, NORMAL);
      return 0;
  }
}


/* Read keyboard input into buffer */
int userInput(char *buf) {
   int i;
      
   // Read up to 126 input chars into packet buffer until newline or EOF (CTRL+D)
   i = 0;
   *buf[i] = getc(stdin);
   while(*buf[i] != '\n' && *buf[i] != EOF) {
      *buf[++i] = getc(stdin);
      // Automatically send message once it reaches 127 characters
      if (i >= 126) {
         *buf[++i] = '\n';
         break;
      }
   }
   //printf("\33[1A\33[J");
   // If EOF is read, exit?
   if(*buf[i] == EOF) {
      exit(1);
   }
   else { 
      buf[i] = '\0';
   }
   return i;
}


/* Connect to a new server */
int newServerConnection(char *buf) {
   int i = 0;
   char *argv[16];
   char *tmp;

   tmp = *buf;

   argv[i] = strsep(&tmp, " \t");
   while ((i < sizeof(argv) - 1) && (argv[i] != '\0')) {
       argv[++i] = strsep(&tmp, " \t");
   }
   if (i == 3) {
      if((serverfd = get_server_connection(argv[1], argv[2])) == -1) {
         // If connection fails, exit
         printf("%s --- Error:%s Could not connect to server.\n", RED, NORMAL);
         return 0;
      }
      if(pthread_create(&chat_rx_thread, NULL, chatRX, (void *)&conn)) {
         // If thread creation fails, exit
         printf("%s --- Error: %s chatRX thread not created.\n", RED, NORMAL);
         return 0;
      }
      printf("Connected.\n");
      return 1;
   }
   else {
       printf("%s --- Error:%s Usage: /connect address port\n", RED, NORMAL);
       return 0;
   }
}


/* Handle registration for server  */
int serverRegistration(packet *tx_pkt) {
   int i = 0;
   char *argv[16];
   char *tmp;
   int pwdMatch = 0;

   tmp = *buf;

   argv[i] = strsep(&tmp, " \t");
   while ((i < sizeof(argv) - 1) && (argv[i] != '\0')) {
       argv[++i] = strsep(&tmp, " \t");
   }
   if (i == 4) {
      pwdMatch = setPassword(argv[2], argv[3]);
      if (pwdMatch) {
         //change buffer
         *tx.pkt.options = REGISTER;
      } 
   }
   else {
       printf("%s --- Error:%s Usage: /register username password password\n", RED, NORMAL);
   }
}


/* Set user password */
int setPassword(char *pwd1, char *pwd2) {
   //compare pwd strings, maybe hash them if valid
}


/* Set user real name */
void setName(char *name) {
  //stub
}


/* Handle SIGINT (CTRL+C) */
void sigintHandler(int sig_num) {
   printf("\b\b%s --- Error:%s Forced Exit.\n", RED, NORMAL);
   exit(1);
}


/* Print messages as they are received */
void *chatRX(void *ptr)
{
   packet rx_pkt;
   int received;
   int *conn = (int *)ptr;
   char *timestamp;
   char partner_name[32];
   
   recv(*conn, (void *)&rx_pkt, sizeof(packet), 0);
   strcpy(partner_name, rx_pkt.buf);
   printf("Conversation started with %s.\n", partner_name);
   
   while(exit_flag)
   {
      // Wait for message to arrive..
      received = recv(*conn, (void *)&rx_pkt, sizeof(packet), 0);
      
      //Handle 'EXIT' message
      if(strcmp("EXIT", (void *)&(rx_pkt.buf)) == 0)
      {
          printf("Other user disconnected.\n");
          close(*conn);
          exit(0);
      }
      
      // Print if not empty
      if(received > 0)
      {
         // Format timestamp
         timestamp = asctime(localtime(&(rx_pkt.timestamp)));
         timestamp[strlen(timestamp) - 1] = '\0';
         if(rx_pkt.options == 0)
         {
            printf("%s%s [%s]:%s %s\n", RED, timestamp, rx_pkt.alias,
                   NORMAL, rx_pkt.buf);
         }
         else
         {
            printf("%s%s [%s]:%s %s\n", BLUE, timestamp, rx_pkt.alias,
                   NORMAL, rx_pkt.buf);
         }
         memset(&rx_pkt, 0, sizeof(packet));
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
   for (p = servinfo; p != NULL; p = p ->ai_next)
   {
      if((serverfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
      {
         printf("%s --- Error:%s socket socket \n", RED, NORMAL);
         continue; // What is this error?
      }
      
      if(connect(serverfd, p->ai_addr, p->ai_addrlen) == -1)
      {
         close(serverfd);
         printf("%s --- Error:%s socket connect \n", RED, NORMAL);
         return -1;
      }
      break;
   }
   
   freeaddrinfo(servinfo);
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
   
   for (p = ai; p !=  NULL; p = p->ai_next)
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
      // Write readable form of IP to ipstr
      inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
      // Print connection information
      printf("Connecting to %s: %s:%d . . .\n", ipver, ipstr, ntohs(port));
   }
}


/* Print helpful and unhelpful things */
void showHelp() {
   printf("%s\t/help%s\t\t | Display a list of commands.\n", YELLOW, NORMAL);
   printf("%s\t/exit%s\t\t | Exit the client.\n", YELLOW, NORMAL);
   printf("%s\t/register%s\t | Usage: /register username firstname lastname\n", YELLOW, NORMAL);
   printf("%s\t/login%s\t\t | Usage: /login username.\n", YELLOW, NORMAL);
}

