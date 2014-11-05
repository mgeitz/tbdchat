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
void showHelp();

// Declare exit_flag as global int
int exit_flag = 1;

int main(int argc, char **argv) {
   pthread_t chat_rx_thread;         // Chat RX thread
   int conn;                         // Connection fd
 
   // Confirm valid program usage
   if (argc < 3) {
      printf("%s --- Error:%s Usage: %s IP_ADDRESS PORT.\n",
             RED, argv[0], NORMAL);
      exit(0);
   }
   
   // Handle CTRL+C
   signal(SIGINT, sigintHandler);
   
   // Initiliaze memory space for send packet
   packet tx_pkt;
 
   // Establish connection with server1
   if((conn = get_server_connection(argv[1], argv[2])) == -1) {
      printf("%s --- Error:%s Connection failure.\n", RED, NORMAL);
      exit_flag = 0;
   }

   printf("Connected.\nType \'/help\' to view a list of available commands.\n");
     
   // Start chat rx thread
   if(pthread_create(&chat_rx_thread, NULL, chatRX, (void *)&conn)) {
      // If thread creation fails, exit
      printf("%s --- Error: %s chatRX thread not created.\n", RED, NORMAL);
      exit_flag = 0;
   }

   // Primary execution loop   
   userInput(conn);  
 
   // Send EXIT message (ensure clean exit on CRTL+C)
   tx_pkt.timestamp = time(NULL);
   strcpy(tx_pkt.buf, "EXIT\0");
   send(conn, (void *)&tx_pkt, sizeof(packet), 0);
   
   // Close connection
   if(pthread_join(chat_rx_thread, NULL)) {
      printf("%s --- Error:%s chatRX thread not joining.\n", RED, NORMAL);
   }
   printf("Exiting.\n");
   close(conn);
   exit(0);
}


void userCommand(int conn, packet tx_pkt) {
    int i = 0;
    char *argv[16];
    char *tmp;

    tmp = tx_pkt.buf; // Required for strsep

    // Handle exit command
    if (strncmp((void *)tx_pkt.buf, "exit", strlen("exit")) == 0) {
        exit_flag = 0;
    }
    // Handle help command
    else if (strncmp((void *)tx_pkt.buf, "help", strlen("help")) == 0) {
        showHelp();
    }
    // Handle register command
    else if (strncmp((void *)tx_pkt.buf, "register", strlen("register")) == 0) {
        argv[i] = strsep(&tmp, " \t");
        while ((i < sizeof(argv) - 1) && (argv[i] != '\0')) { 
            argv[++i] = strsep(&tmp, " \t"); 
        }
        if (i == 4) {
            strcpy(tx_pkt.buf, argv[1]);
            tx_pkt.timestamp = time(NULL);
            tx_pkt.options = 0;
            send(conn, (void *)&tx_pkt, sizeof(packet), 0);

            strcpy(tx_pkt.buf, argv[2]);
            strcat(tx_pkt.buf, " ");
            strcat(tx_pkt.buf, argv[3]);
            send(conn, (void *)&tx_pkt, sizeof(packet), 0);
        }
        else {
            printf("%s --- Error:%s Usage: /register username firstname lastname\n", RED, NORMAL);
        }
        //recv(conn, (void *)&tx_pkt, sizeof(packet),0);
        //if(strcmp(tx_pkt.buf, "ERROR") == 0) printf("Invalid username. Please try again.\n");
        //else {
        //  printf("Your name is: %s\n", tx_pkt.buf);
        //}
    }
    // Handle login command
    else if (strncmp((void *)tx_pkt.buf, "login", strlen("login")) == 0) {
        argv[i] = strsep(&tmp, " \t");
        while ((i < sizeof(argv) - 1) && (argv[i] != '\0')) { 
            argv[++i] = strsep(&tmp, " \t"); 
        }
        if (i == 2) {
            strcpy(tx_pkt.buf, argv[1]);
            tx_pkt.timestamp = time(NULL);
            tx_pkt.options = 1;
            send(conn, (void *)&tx_pkt, sizeof(packet), 0);
        }
        else {
            printf("%s --- Error:%s Usage: /login username\n", RED, NORMAL);
        }
    }
    // If it wasn't any of that, invalid command
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
   
      // Inspect a non empty buffer 
      if(i > 0 && tx_pkt.buf[i] != EOF) {

         // Timestamp and send packet
         tx_pkt.timestamp = time(NULL);
  
         // Handle user commands
         if(strncmp("/", (void *)tx_pkt.buf, 1) == 0) {
             // Remove leading slash and pass to userCommand
             memmove(tx_pkt.buf, tx_pkt.buf + 1, sizeof(tx_pkt.buf) - 1);
             userCommand(conn, tx_pkt);
         }
         // Otherwise transmit a message
         else {
             send(conn, (void *)&tx_pkt, sizeof(packet), 0);
         }
      }
   
      // Wipe packet buffer clean
      memset(&tx_pkt, 0, sizeof(packet));
   }
}


/* Print helpful and unhelpful things */
void showHelp() {
   printf("%s\t/help%s\t\t | Display a list of commands.\n", YELLOW, NORMAL);
   printf("%s\t/exit%s\t\t | Exit the client.\n", YELLOW, NORMAL);
   printf("%s\t/register%s\t | Usage: /register username firstname lastname\n", YELLOW, NORMAL);
   printf("%s\t/login%s\t\t | Usage: /login username.\n", YELLOW, NORMAL);
}


/* Handle SIGINT (CTRL+C) */
void sigintHandler(int sig_num) {
   printf("\b\b%s --- Error:%s Forced Exit.\n", RED, NORMAL);
   exit_flag = 0;
}


/* Print messages as they are received */
void *chatRX(void *ptr) {
   packet rx_pkt;
   int received;
   int *conn = (int *)ptr;
   char *timestamp;
   char partner_name[32];
 
   recv(*conn, (void *)&rx_pkt, sizeof(packet), 0);
   strcpy(partner_name, rx_pkt.buf);
   printf("Conversation started with %s.\n", partner_name);

   while(exit_flag) {
      // Wait for message to arrive..
      received = recv(*conn, (void *)&rx_pkt, sizeof(packet), 0);
      
      //Handle 'EXIT' message
      if(strcmp("EXIT", (void *)rx_pkt.buf) == 0) {
          printf("Other user disconnected.\n");
          close(*conn);
          exit(0);  
      }
      
      // Print if not empty
      if(received > 0) {
         // Format timestamp
         timestamp = asctime(localtime(&(rx_pkt.timestamp)));
         timestamp[strlen(timestamp) - 1] = '\0';
         printf("\a%s%s [%s]:%s %s\n", RED, timestamp, partner_name,
                 NORMAL, rx_pkt.buf);
         memset(&rx_pkt, 0, sizeof(packet));
      }
   }
   return NULL;
}


/* Establish server connection */
int get_server_connection(char *hostname, char *port) {
   int serverfd;
   struct addrinfo hints, *servinfo, *p;
   int status;
   
   /* How much of this do we need? */
   
   memset(&hints, 0, sizeof hints);
   hints.ai_family = PF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   
   if((status = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
      printf("getaddrinfo: %s\n", gai_strerror(status));
      return -1;
   }
   
   print_ip(servinfo);
   for (p = servinfo; p != NULL; p = p ->ai_next) {
      if((serverfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
         printf("%s --- Error:%s socket socket \n", RED, NORMAL);
         continue; // What is this error?
      }
      
      if(connect(serverfd, p->ai_addr, p->ai_addrlen) == -1) {
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
void print_ip( struct addrinfo *ai) {
   struct addrinfo *p;
   void *addr;
   char *ipver;
   char ipstr[INET6_ADDRSTRLEN];
   struct sockaddr_in *ipv4;
   struct sockaddr_in6 *ipv6;
   short port = 0;
   
   for (p = ai; p !=  NULL; p = p->ai_next) {
      if(p->ai_family == AF_INET) {
         ipv4 = (struct sockaddr_in *)p->ai_addr;
         addr = &(ipv4->sin_addr);
         port = ipv4->sin_port;
         ipver = "IPV4";
      }
      else {
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
