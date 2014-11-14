/* Program:             Simple Chat Client
   Authors:             Matthew Owens, Michael Geitz, Shayne Wierbowski
   Date:                10/23/2014
   File Name:           chat_client.c
   Compile:             gcc -o chat_client chat_client.c -l pthread
   Run:                 ./chat_client IP_ADDRESS PORT

   The client program for a simple two way chat utility

*/
#include "client_commands.h"

int serverfd;
//volatile int currentRoom;
//volatile int debugMode;
//char username[64];
pthread_t chat_rx_thread;


int main(int argc, char **argv) {
   int bufSize, send_flag;
   packet tx_pkt;
   char *timestamp;
   packet *tx_pkt_ptr = &tx_pkt;
   
   // Handle CTRL+C
   signal(SIGINT, sigintHandler);
   
   // Clear terminal and display splash text
   printf("\33[2J\33[H");
   printf("Fancy ascii logo splash. Use /help to view a list of available commands.\n");
   
   
   while (1) {
      tx_pkt.options = INVALID;
      bufSize = userInput(tx_pkt_ptr);
      send_flag = 1;
      if(bufSize > 0 && tx_pkt.buf[bufSize] != EOF) {
         if(strncmp("/", (void *)tx_pkt.buf, 1) == 0) {
             send_flag = userCommand(tx_pkt_ptr);
         }
         if (send_flag && serverfd) {
            pthread_mutex_lock(&unameMutex);
            strcpy(tx_pkt.alias, username);
            pthread_mutex_unlock(&unameMutex);
            tx_pkt.timestamp = time(NULL);
            pthread_mutex_lock(&roomMutex);
            if (currentRoom >= 1000 && tx_pkt.options == -1) {
               timestamp = asctime(localtime(&(tx_pkt.timestamp)));
               timestamp[strlen(timestamp) - 1] = '\0';
               printf("%s%s [%s]:%s %s\n", BLUE, timestamp, tx_pkt.alias,
                      NORMAL, tx_pkt.buf);
               tx_pkt.options = currentRoom;
            }
            pthread_mutex_unlock(&roomMutex);
            if (tx_pkt.options > 0) {
               send(serverfd, (void *)&tx_pkt, sizeof(packet), 0);
            }
         }
         else if (send_flag && !serverfd)  {
            printf("%s --- Error:%s Not connected to any server. See /help for command usage.\n", RED, NORMAL);
         } 
      }
      if (tx_pkt.options == EXIT) {
         break;
      }
      // Wipe packet
      memset(&tx_pkt, 0, sizeof(packet));
   }
   
   // Close connection
   printf("Exiting.\n");
   close(serverfd);
   if(pthread_join(chat_rx_thread, NULL)) {
      printf("%s --- Error:%s chatRX thread not joining.\n", RED, NORMAL);
   }
   exit(0);
}


/* Handle SIGINT (CTRL+C) */
void sigintHandler(int sig_num) {
   printf("\b\b%s --- Error:%s Forced Exit.\n", RED, NORMAL);
   exit(1);
}


/* Process user commands and mutate buffer accordingly */
int userCommand(packet *tx_pkt) {
   
   // Handle exit command
   if (strncmp((void *)tx_pkt->buf, "/exit", strlen("/exit")) == 0) {
       tx_pkt->options = EXIT;
       return 1;;
   }
   // Handle help command
   else if (strncmp((void *)tx_pkt->buf, "/help", strlen("/help")) == 0) {
       showHelp();
       return 0;
   }
   else if (strncmp((void *)tx_pkt->buf, "/debug", strlen("/debug")) == 0) {
       pthread_mutex_lock(&debugModeMutex);
       if (debugMode) {
         debugMode = 0;
       }
       else {
          debugMode = 1;
       }
       pthread_mutex_unlock(&debugModeMutex);
       return 0;
   }
   // Handle connect command
   else if (strncmp((void *)tx_pkt->buf, "/connect", strlen("/connect")) == 0) {
      if (!newServerConnection((void *)tx_pkt->buf)) {
          printf("%s --- Error:%s Server connect failed.\n", RED, NORMAL);
      }
      return 0;
   }
   // Handle register command
   else if (strncmp((void *)tx_pkt->buf, "/register", strlen("/register")) == 0) {
      if (!serverRegistration(tx_pkt)) {
         printf("%s --- Error:%s Server registration failed.\n", RED, NORMAL);
         return 0;
      }
      else {
         return 1;
      }
   }
   // Handle login command
   else if (strncmp((void *)tx_pkt->buf, "/login", strlen("/login")) == 0) {
      if (!serverLogin(tx_pkt)) {
         printf("%s --- Error:%s Server login failed.\n", RED, NORMAL);
         return 0;
      }
      else {
         return 1;
      }
   }
   // Handle setname command
   else if (strncmp((void *)tx_pkt->buf, "/setname", strlen("/setname")) == 0) {
      return setName(tx_pkt);
   }
   // Handle setpass command
   else if (strncmp((void *)tx_pkt->buf, "/setpass", strlen("/setpass")) == 0) {
      if (!setPassword(tx_pkt)) {
         printf("%s --- Error:%s Password mismatch.\n", RED, NORMAL);
         return 0;
      }
      else {
         return 1;
      }
   }
   // Handle invite command
   if (strncmp((void *)tx_pkt->buf, "/invite", strlen("/invite")) == 0) {
       tx_pkt->options = JOIN;
       return 1;;
   }
   // Handle join command
   else if (strncmp((void *)tx_pkt->buf, "/join", strlen("/join")) == 0) {
       tx_pkt->options = INVITE;
       return 1;;
   }
   // Handle who command
   else if (strncmp((void *)tx_pkt->buf, "/who", strlen("/who")) == 0) {
       tx_pkt->options = GETUSERS;
       return 1;;
   }
   // If it wasn't any of that, invalid command
   else {
      printf("%s --- Error:%s Invalid command.\n", RED, NORMAL);
      return 0;
   }
}


/* Read keyboard input into buffer */
int userInput(packet *tx_pkt) {
   int i = 0;
   
   // Read up to 126 input chars into packet buffer until newline or EOF (CTRL+D)
   tx_pkt->buf[i] = getc(stdin);
   while(tx_pkt->buf[i] != '\n' && tx_pkt->buf[i] != EOF) {
      tx_pkt->buf[++i] = getc(stdin);
      // Automatically send message once it reaches 127 characters
      if (i >= 126) {
         tx_pkt->buf[++i] = '\n';
         break;
      }
   }
   printf("\33[1A\33[J");
   // If EOF is read, exit?
   if(tx_pkt->buf[i] == EOF) {
      exit(1);
   }
   else { 
      tx_pkt->buf[i] = '\0';
   }
   return i;
}


/* Print messages as they are received */
void *chatRX(void *ptr) {
   packet rx_pkt;
   packet *rx_pkt_ptr = &rx_pkt;
   int received;
   int *serverfd = (int *)ptr;
   char *timestamp;

   while(1) {
      // Wait for message to arrive..
      received = recv(*serverfd, (void *)&rx_pkt, sizeof(packet), 0);

      if(received) {
         pthread_mutex_lock(&debugModeMutex);
         if (debugMode) {
            debugPacket(rx_pkt_ptr);
         }
         pthread_mutex_unlock(&debugModeMutex);
         if (rx_pkt.options >= 1000) {
            // Format timestamp
            pthread_mutex_lock(&roomMutex);
            if (rx_pkt.options != currentRoom) {
               currentRoom = rx_pkt.options;
               printf("%s --- Success:%s Joined room %d.\n", GREEN, NORMAL, currentRoom);
            }
            pthread_mutex_unlock(&roomMutex);
            timestamp = asctime(localtime(&(rx_pkt.timestamp)));
            timestamp[strlen(timestamp) - 1] = '\0';
            printf("%s%s [%s]:%s %s\n", RED, timestamp, rx_pkt.alias,
                   NORMAL, rx_pkt.buf);
         }
         else {
            serverResponse(rx_pkt_ptr);
         }
      }
      memset(&rx_pkt, 0, sizeof(packet));
   }
   return NULL;
}


/* Connect to a new server */
int newServerConnection(char *buf) {
   int i = 0;
   char *args[16];
   char cpy[128];
   char *tmp = cpy;
   strcpy(tmp, buf);
   
   args[i] = strsep(&tmp, " \t");
   while ((i < sizeof(args) - 1) && (args[i] != '\0')) {
       args[++i] = strsep(&tmp, " \t");
   }
   if (i == 3) {
      if((serverfd = get_server_connection(args[1], args[2])) == -1) {
         // If connection fails, exit
         printf("%s --- Error:%s Could not connect to server.\n", RED, NORMAL);
         return 0;
      }
      if(pthread_create(&chat_rx_thread, NULL, chatRX, (void *)&serverfd)) {
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


/* Establish server connection */
int get_server_connection(char *hostname, char *port) {
   int serverfd;
   struct addrinfo hints, *servinfo, *p;
   int status;
   
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
         continue;
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


/* Print new connection information */
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
