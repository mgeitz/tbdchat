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
volatile int currentRoom;
char username[64];
pthread_t chat_rx_thread;
pthread_mutex_t roomMutex = PTHREAD_MUTEX_INITIALIZER;


int main() {
   int bufSize, send_flag;
   packet tx_pkt;
   char *timestamp;
   packet *tx_pkt_ptr = &tx_pkt;
   
   // Handle CTRL+C
   signal(SIGINT, sigintHandler);

   printf("Fancy ascii logo splash. Use /help to see a list of available commands.\n");
   
   while (1) {
      strcpy(tx_pkt.alias, username);
      tx_pkt.options = -1;
      bufSize = userInput(tx_pkt_ptr);
      send_flag = 1;
      if(bufSize > 0 && tx_pkt.buf[bufSize] != EOF) {
         if(strncmp("/", (void *)tx_pkt.buf, 1) == 0) {
             send_flag = userCommand(tx_pkt_ptr);
         }
         if (send_flag && serverfd) {
            tx_pkt.timestamp = time(NULL);
            pthread_mutex_lock(&roomMutex);
            if (currentRoom >= 1000 && tx_pkt.options == -1) {
               printf("%s%s [%s]:%s %s\n", BLUE, timestamp, tx_pkt.alias,
                      NORMAL, tx_pkt.buf);
               tx_pkt.options = currentRoom;
            pthread_mutex_unlock(&roomMutex);
            }
            if (tx_pkt.options > 0) {
               send(serverfd, (void *)&tx_pkt, sizeof(packet), 0);
            }
         }
         if (send_flag && !serverfd)  {
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
   if(pthread_join(chat_rx_thread, NULL)) {
      printf("%s --- Error:%s chatRX thread not joining.\n", RED, NORMAL);
   }
   printf("Exiting.\n");
   close(serverfd);
   exit(0);
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
   // Handle connect command
   else if (strncmp((void *)tx_pkt->buf, "/connect ", strlen("/connect ")) == 0) {
      if (!newServerConnection((void *)tx_pkt->buf)) {
          printf("%s --- Error:%s Server connect failed.\n", RED, NORMAL);
      }
      return 0;
   }
   // Handle register command
   else if (strncmp((void *)tx_pkt->buf, "/register ", strlen("/register ")) == 0) {
      if (!serverRegistration(tx_pkt)) {
         printf("%s --- Error:%s Server registration failed.\n", RED, NORMAL);
         return 0;
      }
      else {
         return 1;
      }
   }
   // Handle login command
   else if (strncmp((void *)tx_pkt->buf, "/login ", strlen("/login ")) == 0) {
      if (!serverLogin(tx_pkt)) {
         printf("%s --- Error:%s Server login failed.\n", RED, NORMAL);
         return 0;
      }
      else {
         return 1;
      }
   }
   // Handle setname command
   else if (strncmp((void *)tx_pkt->buf, "/setname ", strlen("/setname ")) == 0) {
      setName(tx_pkt);
      return 1;
   }
   // Handle setpass command
   else if (strncmp((void *)tx_pkt->buf, "/setpass ", strlen("/setpass ")) == 0) {
      if (!setPassword(tx_pkt)) {
         printf("%s --- Error:%s Password mismatch.\n", RED, NORMAL);
         return 0;
      }
      else {
         return 1;
      }
   }
   // Handle invite command
   if (strncmp((void *)tx_pkt->buf, "/invite ", strlen("/invite ")) == 0) {
       tx_pkt->options = JOIN;
       return 1;;
   }
   // Handle join command
   else if (strncmp((void *)tx_pkt->buf, "/join ", strlen("/join ")) == 0) {
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


/* Connect to a new server */
int serverLogin(packet *tx_pkt) {
   char *args[3];
   char arr[64];
   strcpy(arr, tx_pkt->buf);
   char *tmp = &arr;
   //pull command
   args[0] = strsep(&tmp, " \t");
   //pull username
   args[1] = strsep(&tmp, " \t");
   strcpy(username, args[1]);
   tx_pkt->options = LOGIN;
   return 1;
}


/* Connect to a new server */
int newServerConnection(char *buf) {
   int i = 0;
   char *argv[16];
   char tmp[128];
   char *tmp_ptr = tmp;
   strcpy(tmp_ptr, buf);
   
   argv[i] = strsep(&tmp_ptr, " \t");
   while ((i < sizeof(argv) - 1) && (argv[i] != '\0')) {
       argv[++i] = strsep(&tmp_ptr, " \t");
   }
   if (i == 3) {
      if((serverfd = get_server_connection(argv[1], argv[2])) == -1) {
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


/* Handle registration for server  */
int serverRegistration(packet *tx_pkt) {
   int i = 0;
   char *argv[16];
   char tmp_arr[128];
   char *tmp = tmp_arr;
   strcpy(tmp, tx_pkt->buf);
   
   // Split command args
   argv[i] = strsep(&tmp, " \t");
   while ((i < sizeof(argv) - 1) && (argv[i] != '\0')) {
       argv[++i] = strsep(&tmp, " \t");
   }
   if (i == 4) {
      // if the passwords patch mark options
      if (strcmp(argv[2], argv[3]) == 0) {
         tx_pkt->options = REGISTER;
         return 1;
      } 
      else {
       printf("%s --- Error:%s Password mismatch\n", RED, NORMAL);
       return 0;
      }
   }
   else {
       printf("%s --- Error:%s Usage: /register username password password\n", RED, NORMAL);
       return 0;
   }
}


/* Set user password */
int setPassword(packet *tx_pkt) {
   int i = 0;
   char *argv[16];
   char tmp_arr[128];
   char *tmp = tmp_arr;
   strcpy(tmp, tx_pkt->buf);
   
   // Split command args
   argv[i] = strsep(&tmp, " \t");
   while ((i < sizeof(argv) - 1) && (argv[i] != '\0')) {
       argv[++i] = strsep(&tmp, " \t");
   }
   if (strcmp(argv[1], argv[2])  == 0) {
      tx_pkt->options = SETPASS;
      return 1;
   }
   else {
      return 0;
   }
}


/* Set user real name */
void setName(packet *tx_pkt) {
   strncpy(username, tx_pkt->buf + strlen("/setname "), strlen(tx_pkt->buf) - strlen("/setname "));
   tx_pkt->options = SETNAME;
}


/* Handle SIGINT (CTRL+C) */
void sigintHandler(int sig_num) {
   printf("\b\b%s --- Error:%s Forced Exit.\n", RED, NORMAL);
   exit(1);
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
         debugPacket(rx_pkt_ptr);
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


/* Dump contents of received packet from server */
void debugPacket(packet *rx_pkt) {
   printf("%s --------------------- PACKET REPORT --------------------- %s\n", CYAN, NORMAL);
   printf("%s Timestamp: %s%lu\n", MAGENTA, NORMAL, rx_pkt->timestamp);
   printf("%s Alias: %s%s\n", MAGENTA, NORMAL, rx_pkt->alias);
   printf("%s Option: %s%d\n", MAGENTA, NORMAL, rx_pkt->options);
   printf("%s Buffer: %s%s\n", MAGENTA, NORMAL, rx_pkt->buf);
   printf("%s --------------------------------------------------------- %s\n", CYAN, NORMAL);

}

/* Handle non message packets from server */
void serverResponse(packet *rx_pkt) {
   if (rx_pkt->options == REGFAIL) {
      printf("%s --- Error:%s Registration failed.\n", RED, NORMAL);
   }
   else if (rx_pkt->options == LOGFAIL) {
      printf("%s --- Error:%s Login failed.\n", RED, NORMAL);
   }
   else if (rx_pkt->options == LOGAUTH) {
      //pthread_mutex_lock(&roomMutex);
      //memcpy(&currentRoom, &rx_pkt->buf, sizeof(rx_pkt->buf));
      printf("%s --- Success:%s Login successful! Joined lobby room.\n", GREEN, NORMAL);
      //pthread_mutex_unlock(&roomMutex);
   }
   else if(rx_pkt->options == GETUSERS) {
      printf("%s\n", rx_pkt->buf);
   }
   else {
      printf("%s --- Error:%s Unknown message received from server.\n", RED, NORMAL);
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


/* Print helpful and unhelpful things */
void showHelp() {
   printf("%s\t/help%s\t\t | Display a list of commands.\n", YELLOW, NORMAL);
   printf("%s\t/exit%s\t\t | Exit the client.\n", YELLOW, NORMAL);
   printf("%s\t/register%s\t | Usage: /register username password password\n", YELLOW, NORMAL);
   printf("%s\t/login%s\t\t | Usage: /login username password.\n", YELLOW, NORMAL);
   printf("%s\t/who%s\t\t | Return a list of other users.\n", YELLOW, NORMAL);
   printf("%s\t/invite%s\t\t | Usage: /invite username.\n", YELLOW, NORMAL);
   printf("%s\t/room%s\t\t | Usage: /join room.\n", YELLOW, NORMAL);
   printf("%s\t/setpass%s\t | Usage: /setpass password password.\n", YELLOW, NORMAL);
   printf("%s\t/setname%s\t | Usage: /setname fname lname.\n", YELLOW, NORMAL);
   printf("%s\t/connect%s\t | Usage: /connect address port.\n", YELLOW, NORMAL);
}
