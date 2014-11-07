/* Program:             Simple Chat Client
   Authors:             Matthew Owens, Michale Geitz, Shayne Wierbowski
   Date:                10/23/2014
   File Name:           chat_client.c
   Compile:             gcc -o chat_client chat_client.c -l pthread
   Run:                 ./chat_client IP_ADDRESS PORT

   The client program for a simple two way chat utility

*/
#include "chat_client.h"

// Declare exit_flag as global volatile int
int exit_flag = 1;

int main(int argc, char **argv)
{
   pthread_t chat_rx_thread;         // Chat RX thread
   int conn;                         // Connection fd
   char fname[32];                    // User alias
   char lname[32];
   char full_name[50];
   char username[32];
   int selection = -1;		     // User selection on startup
   int loop_control = 1;
   
   // Confirm valid program usage
   if (argc < 3)
   {
      //printf("\e[1m\x1b[31m --- Error:\x1b[0m\e[0m Usage: %s IP_ADDRESS PORT.\n",
      //       argv[0]);
      
      
      printf("%s --- Error:%s Usage: %s IP_ADDRESS PORT.\n",
             RED, argv[0], NORMAL);
      exit(0);
   }
   
   // Handle CTRL+C
   signal(SIGINT, sigintHandler);
   
   // Initiliaze memory space for send packet
   packet tx_pkt;
   
   // Assign name as *nix username, maybe add ability to assign alias
   //strcpy(name, getlogin());
   
   // Establish connection with server1
   if((conn = get_server_connection(argv[1], argv[2])) == -1)
   {
      // If connection fails, exit
      //printf("\e[1m\x1b[31m --- Error:\x1b[0m\e[0m Connection failure.\n");
      printf("%s --- Error:%s Connection failure.\n", RED, NORMAL);
      exit_flag = 0;
   }
   
   printf("Connected.\n");
   
   while(loop_control) 
   {  
      //Register or log in
      printf("Enter 0 to register or 1 to login: ");
      scanf("%d", &selection); 
      
      if(selection == 1)
      {
         printf("Enter your username: ");
         scanf("%32s", username); 
         printf("Your username is: %s\n", username);
         
         strcpy(tx_pkt.buf, username);
         tx_pkt.timestamp = time(NULL);
         tx_pkt.options = 1;
         send(conn, (void *)&tx_pkt, sizeof(packet), 0);
         recv(conn, (void *)&tx_pkt, sizeof(packet),0);
         if(strcmp(tx_pkt.buf, "ERROR") == 0)
         {
            printf("Invalid username. Please try again.\n");
         }
         else
         {
            printf("Your name is: %s\n", tx_pkt.buf);
            strcpy(full_name, tx_pkt.buf);
            loop_control = 0;
         }
      }
      
      else if(selection == 0)
      {
         printf("Enter your desired username: ");
         scanf("%32s", username);
         printf("Your username is: %s\n", username);
         
         strcpy(tx_pkt.buf, username);
         tx_pkt.timestamp = time(NULL);
         tx_pkt.options = 0;
         send(conn, (void *)&tx_pkt, sizeof(packet), 0);
         
         recv(conn, (void *)&tx_pkt, sizeof(packet),0);
         if(strcmp(tx_pkt.buf, "ERROR") == 0)
         {
            printf("%sERROR - %sUsername %s is already taken\n", RED, NORMAL, username);
         }
         else
         {
            printf("Enter your first name: ");
            scanf("%32s", fname);
            printf("Enter your last name: ");
            scanf("%32s", lname);
            
            strcpy(tx_pkt.buf, fname);
            strcat(tx_pkt.buf, " ");
            strcat(tx_pkt.buf, lname); 
            printf("Your name is: %s\n", tx_pkt.buf);
            strcpy(full_name, tx_pkt.buf);
            send(conn, (void *)&tx_pkt, sizeof(packet), 0);
            loop_control = 0;
         }
      }
      
      else
      {
         printf("Invalid input. Please try again\n");
         //memset(&selection, 0, sizeof(selection));
      }
      fseek(stdin, 0, SEEK_END);
      selection = -1;
   }
     
   // Start chat rx thread
   if(pthread_create(&chat_rx_thread, NULL, chatRX, (void *)&conn))
   {
      // If thread creation fails, exit
      //printf("\e[1m\x1b[31m --- Error:\x1b[0m\e[0m chatRX thread not created.\n");
      printf("%s --- Error: %s chatRX thread not created.\n", RED, NORMAL);
      exit_flag = 0;
   }
   
   // Primary execution loop
   userInput(conn, full_name);
   
   // Send EXIT message (ensure clean exit on CRTL+C)
   //strcpy(tx_pkt.alias, name);
   tx_pkt.timestamp = time(NULL);
   strcpy(tx_pkt.buf, "EXIT\0");
   send(conn, (void *)&tx_pkt, sizeof(packet), 0);
   
   // Close connection
   if(pthread_join(chat_rx_thread, NULL))
   {
      //printf("\e[1m\x1b[31m --- Error:\x1b[0m\e[0m chatRX thread not joining.\n");
      printf("%s --- Error:%s chatRX thread not joining.\n", RED, NORMAL);
   }
   printf("Exiting.\n");
   close(conn);
   exit(0);
}


/* Primary execution loop */
void userInput(int conn, char *username) {
    int i;
   // Initiliaze memory space for send packet
   packet tx_pkt;
   
   // Primary execution loop
   while(exit_flag)
   {
      // Add alias to send packet
      strcpy(tx_pkt.alias, username);
      
      // Read up to 126 input chars into packet buffer until newline or EOF (CTRL+D)
      i = 0;
      tx_pkt.buf[i] = getc(stdin);
      while(tx_pkt.buf[i] != '\n' && tx_pkt.buf[i] != EOF)
      {
         tx_pkt.buf[++i] = getc(stdin);
         // Automatically send message once it reaches 127 characters
         if(i >= 126 || exit_flag == 0)
         {
            tx_pkt.buf[++i] = '\n';
            break;
         }
      }
      // If EOF is read, exit?
      if(tx_pkt.buf[i] == EOF)
      {
         exit_flag = 0;
      }
      else // Otherwise Null terminate keyboard buffer
      {
         tx_pkt.buf[i] = '\0';
      }
      
      // Transmit packet if input buffer is not empty
      if(i > 0 && tx_pkt.buf[i] != EOF)
      {
         // Timestamp and send packet
         tx_pkt.timestamp = time(NULL);
         send(conn, (void *)&tx_pkt, sizeof(packet), 0);
      }
      
      //Handle 'EXIT' message
      if(strcmp("EXIT", (void *)&(tx_pkt.buf)) == 0)
      {
         exit_flag = 0;
      }
      
      // Wipe packet buffer clean
      memset(&tx_pkt, 0, sizeof(packet));
   }
}

/* Handle SIGINT (CTRL+C) */
void sigintHandler(int sig_num)
{
   //printf("\b\b\e[1m\x1b[31m --- Error:\x1b[0m\e[0m Forced Exit.\n");
   printf("\b\b%s --- Error:%s Forced Exit.\n", RED, NORMAL);
   exit_flag = 0;
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
         //printf("\e[1m\x1b[31m --- Error:\x1b[0m\e[0m socket socket \n");
         printf("%s --- Error:%s socket socket \n", RED, NORMAL);
         continue; // What is this error?
      }
      
      if(connect(serverfd, p->ai_addr, p->ai_addrlen) == -1)
      {
         close(serverfd);
         //printf("\e[1m\x1b[31m --- Error:\x1b[0m\e[0m socket connect \n");
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
