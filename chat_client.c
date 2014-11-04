/* Program:             Simple Chat Client
   Authors:             Matthew Owens, Michale Geitz, Shayne Wierbowski
   Date:                10/23/2014
   File Name:           chat_client.c
   Compile:             gcc -o chat_client chat_client.c -l pthread
   Run:                 ./chat_client IP_ADDRESS PORT

   The client program for a simple two way chat utility

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>
#include <pthread.h>

#define BUFFERSIZE 128

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

// Structure to be sent to server containing the send time, username, and message
struct Packet {
   time_t timestamp;
   char buf[BUFFERSIZE];
   int options;
};
// Declare structure type
typedef struct Packet packet;

// Function prototypes
void sigintHandler(int sig_num);
void print_ip( struct addrinfo *ai);
int get_server_connection(char *hostname, char *port);
void *chatRX(void *ptr);
void userInput(int conn);

// Declare exit_flag as global volatile int
int exit_flag = 1;

int main(int argc, char **argv)
{
   pthread_t chat_rx_thread;         // Chat RX thread
   int conn;                         // Connection fd
   char fname[32];                    // User alias
   char lname[32];
   int selection;		     // User selection on startup
   int loop_control = 1;
 
   // Confirm valid program usage
   if (argc < 3)
   {
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
         scanf("%32s", fname); 
         printf("Your username is: %s\n", fname);

         strcpy(tx_pkt.buf, fname);
         tx_pkt.timestamp = time(NULL);
         tx_pkt.options = 1;
         send(conn, (void *)&tx_pkt, sizeof(packet), 0);
         loop_control = 0;
      }

      else if(selection == 0)
      {
         printf("Enter your desired username: ");
         scanf("%32s", fname);
         printf("Your username is: %s\n", fname);
         
         strcpy(tx_pkt.buf, fname);
         tx_pkt.timestamp = time(NULL);
         tx_pkt.options = 0;
         send(conn, (void *)&tx_pkt, sizeof(packet), 0);

         printf("Enter your first name: ");
         scanf("%32s", fname);
         printf("Enter your last name: ");
         scanf("%32s", lname);

         strcpy(tx_pkt.buf, fname);
         strcat(tx_pkt.buf, " ");
         strcat(tx_pkt.buf, lname); 
         printf("your name is: %s\n", tx_pkt.buf);
         send(conn, (void *)&tx_pkt, sizeof(packet), 0);


         loop_control = 0;
      }

      else
      {
         printf("Invalid input. Please try again\n");
      }
   }
     
   // Start chat rx thread
   if(pthread_create(&chat_rx_thread, NULL, chatRX, (void *)&conn))
   {
      // If thread creation fails, exit
      //printf("\e[1m\x1b[31m --- Error:\x1b[0m\e[0m chatRX thread not created.\n");
      printf("%s --- Error: %s chatRX thread not created.\n", RED, NORMAL);
      exit_flag = 0;
   }
   
   userInput(conn);  
 
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


void userCommand(int conn, packet tx_pkt) {
    if (strncmp(tx_pkt.buf, "exit", sizeof("exit")) == 0) {
        exit_flag = 0;
    }
    else if (strncmp(tx_pkt.buf, "register", sizeof("register")) == 0) {
        // handle register here
    }
    else if (strncmp(tx_pkt.buf, "login", sizeof("login")) == 0) {
        // handle register here
    }
    else {
      printf("%s --- Error:%s Invalid command.\n", RED, NORMAL);
    }
}

void userInput(int conn) {
   int i;                            // Counter
   // Initiliaze memory space for send packet
   packet tx_pkt;

   // Primary execution loop
   while(exit_flag) {
   
      // Read up to 126 input chars into packet buffer until newline or EOF (CTRL+D)
      i = 0;
      tx_pkt.buf[i] = getc(stdin);
      while(tx_pkt.buf[i] != '\n' && tx_pkt.buf[i] != EOF) {
         tx_pkt.buf[++i] = getc(stdin);
         // Automatically send message once it reaches 127 characters
         if(i >= 126 || exit_flag == 0) {
            tx_pkt.buf[++i] = '\n';
            break;
         }
      }
      // If EOF is read, exit?
      if(tx_pkt.buf[i] == EOF) {
         exit_flag = 0;
      }
      else { // Otherwise Null terminate keyboard buffer
         tx_pkt.buf[i] = '\0';
      }
   
      // Transmit packet if input buffer is not empty
      if(i > 0 && tx_pkt.buf[i] != EOF) {

         // Timestamp and send packet
         tx_pkt.timestamp = time(NULL);
  
         //Handle user command message
         if(strncmp("/", &(tx_pkt.buf), 1) == 0) {
             memmove(tx_pkt.buf, tx_pkt.buf + 1, sizeof(tx_pkt.buf) - 1);
             userCommand(conn, tx_pkt);
         }
         else {
             send(conn, (void *)&tx_pkt, sizeof(packet), 0);
         }
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
      if(strcmp("EXIT", &(rx_pkt.buf)) == 0)
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
         //printf("\a\x1b[31m\e[1m%s\x1b[0m | %s\e[0m: %s\n", timestamp,
         //       rx_pkt.alias, rx_pkt.buf);
         printf("\a%s%s [%s]:%s %s\n", RED, timestamp, partner_name,
                 NORMAL, rx_pkt.buf);
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
