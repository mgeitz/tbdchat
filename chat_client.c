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
pthread_mutex_t roomMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t nameMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t debugModeMutex = PTHREAD_MUTEX_INITIALIZER;
volatile int currentRoom;
volatile int debugMode;
char realname[64];
char username[64];
pthread_t chat_rx_thread;
char *config_file;

int main(int argc, char **argv) {
   int bufSize, send_flag;
   packet tx_pkt;
   char *timestamp;
   char *config_file_name = "/tbd_chat.ini";
   char full_config_path[64];
   packet *tx_pkt_ptr = &tx_pkt;
   
   // Handle CTRL+C
   signal(SIGINT, sigintHandler);

   strcpy(full_config_path, getenv("HOME"));
   printf("User home is %s\n", full_config_path);
   strcat(full_config_path, config_file_name);
   printf("full config path is %s\n", full_config_path);
   config_file = full_config_path;
   if (access(config_file, F_OK) == -1) {
      buildDefaultConfig();
   }

   printf("\33[2J\33[H");
   asciiSplash();

   if (auto_connect()) {
      printf("Auto connecting to most recently connected host . . .\n");
      reconnect(tx_pkt.buf);
   }
   
   while (1) {
      memset(&tx_pkt, 0, sizeof(packet));
      tx_pkt.options = INVALID;
      bufSize = userInput(tx_pkt_ptr);
      send_flag = 1;
      if(bufSize > 0 && tx_pkt.buf[bufSize] != EOF) {
         if(strncmp("/", (void *)tx_pkt.buf, 1) == 0) {
             send_flag = userCommand(tx_pkt_ptr);
         }
         if (send_flag && serverfd) {
            pthread_mutex_lock(&nameMutex);
            strcpy(tx_pkt.username, username);
            strcpy(tx_pkt.realname, realname);
            pthread_mutex_unlock(&nameMutex);
            tx_pkt.timestamp = time(NULL);
            pthread_mutex_lock(&roomMutex);
            if (currentRoom >= 1000 && tx_pkt.options == -1) {
               timestamp = asctime(localtime(&(tx_pkt.timestamp)));
               timestamp[strlen(timestamp) - 1] = '\0';
               printf("%s%s [%s]:%s %s\n", BLUE, timestamp, tx_pkt.realname,
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
   }
   
   // Close connection
   printf("Exiting.\n");
   close(serverfd);
   if(pthread_join(chat_rx_thread, NULL)) {
      printf("%s --- Error:%s chatRX thread not joining.\n", RED, NORMAL);
   }
   exit(0);
}


/* Build the default config file */
void buildDefaultConfig() {
      FILE *configfp;
      configfp = fopen(config_file, "a+");
      fputs("###\n#\n#\tTBD Chat Configuration\n#\n###\n\n\n", configfp);
      fputs("## Stores auto reconnect state\nauto-reconnect: 0\n\n", configfp);
      fputs("## Stores last client connection\nlast connection:\n", configfp);
      fclose(configfp);
}


/* Check if auto-reconnect is set enabled */
int auto_connect() {
   FILE *configfp;
   char line[128];
   
   configfp = fopen(config_file, "r");
   if (configfp != NULL) {
      while (!feof(configfp)) {
         if (fgets(line, sizeof(line), configfp)) {
            if (strncmp(line, "auto-reconnect:", strlen("auto-reconnect:")) == 0) {
               fclose(configfp);
               strncpy(line, line + strlen("auto-reconnect: "), 1);
               if (strncmp(line, "0", 1) == 0) {
                  return 0;
               }
               else {
                  return 1;
               }
            }
         }
      }
   }
   fclose(configfp);
   return 0;
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
            if(strcmp(rx_pkt.realname, "SERVER") == 0) {
            printf("%s%s [%s]:%s %s\n", YELLOW, timestamp, rx_pkt.realname,
                   NORMAL, rx_pkt.buf);
            }
            else {
            printf("%s%s [%s]:%s %s\n", RED, timestamp, rx_pkt.realname,
                   NORMAL, rx_pkt.buf);
            }
         }
         else if (rx_pkt.options > 0 && rx_pkt.options < 1000) {
            if (rx_pkt.options == EXIT) return NULL;
            serverResponse(rx_pkt_ptr);
         }
         else {
            printf("%s --- Error:%s Something horrible has happened.\n", RED, NORMAL);
            break;
         }
      }
      memset(&rx_pkt, 0, sizeof(packet));
   }
   return NULL;
}


/* Handle non message packets from server */
void serverResponse(packet *rx_pkt) {
   if (rx_pkt->options == REGFAIL) {
      printf("%s --- Error:%s Registration failed.\n", RED, NORMAL);
   }
   else if (rx_pkt->options == REGSUC) {
      pthread_mutex_lock(&roomMutex);
      // Hardcoded lobby room
      currentRoom = 1000;
      pthread_mutex_unlock(&roomMutex);
      printf("%s --- Success:%s Registration successful!\n", GREEN, NORMAL);
   }
   else if (rx_pkt->options == LOGFAIL) {
      printf("%s --- Error:%s Login failed.\n", RED, NORMAL);
   }
   else if (rx_pkt->options == LOGSUC) {
      pthread_mutex_lock(&nameMutex);
      strcpy(username, rx_pkt->username);
      strcpy(realname, rx_pkt->realname);
      pthread_mutex_unlock(&nameMutex);
      pthread_mutex_lock(&roomMutex);
      // Hardcoded lobby room
      currentRoom = 1000;
      pthread_mutex_unlock(&roomMutex);
      printf("%s --- Success:%s Login successful!\n", GREEN, NORMAL);
   }
   else if(rx_pkt->options == GETUSERS) {
      printf("%s\n", rx_pkt->buf);
   }
   else if(rx_pkt->options == PASSFAIL) {
      printf("%s --- Error:%s Password change failed.\n", RED, NORMAL);
   }
   else if(rx_pkt->options == PASSSUC) {
      printf("%s --- Success:%s Password change successful!\n", GREEN, NORMAL);
   }
   else if(rx_pkt->options == NAMESUC) {
      pthread_mutex_lock(&nameMutex);
      memset(&realname, 0, sizeof(realname));
      strncpy(realname, rx_pkt->buf, sizeof(realname));
      pthread_mutex_unlock(&nameMutex);
      printf("%s --- Success:%s Name change successful!\n", GREEN, NORMAL);
   }
   else if(rx_pkt->options == NAMEFAIL) {
      printf("%s --- Error:%s Name change failed.\n", RED, NORMAL);
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


/* Handle SIGINT (CTRL+C) */
void sigintHandler(int sig_num) {
   printf("\b\b%s --- Error:%s Forced Exit.\n", RED, NORMAL);
   exit(1);
}


void asciiSplash() {

   printf("%s         __%s\n", GREEN, NORMAL);
   printf("%s        /_/%s\\        %s_____ %s____  %s____    %s ____ _           _   %s\n", GREEN, GREEN, CYAN, CYAN, CYAN, WHITE, NORMAL);
   printf("%s       / /%s\\ \\      %s|_   _%s| __ )%s|  _ \\ %s  / ___| |__   __ _| |_ %s\n", RED, GREEN, CYAN, CYAN, CYAN, WHITE, NORMAL);
   printf("%s      / / /%s\\ \\       %s| | %s|  _ \\%s| | | |%s | |   | '_ \\ / _` | __|%s\n", RED, GREEN, CYAN, CYAN, CYAN, WHITE, NORMAL);
   printf("%s     / / /%s\\ \\ \\      %s| | %s| |_) %s| |_| |%s | |___| | | | (_| | |_ %s\n", RED, GREEN, CYAN, CYAN, CYAN, WHITE, NORMAL);
   printf("%s    / /%s_%s/%s__%s\\ \\ \\     %s|_| %s|____/%s|____/ %s  \\____|_| |_|\\__,_|\\__|%s\n", RED, BLUE, RED, BLUE, GREEN, CYAN, CYAN, CYAN, WHITE, NORMAL);
   printf("%s   /_/%s______%s\\%s_%s\\%s/%s\\%s\n", RED, BLUE, GREEN, BLUE, GREEN, BLUE, BLUE, NORMAL);
   printf("%s   \\_\\%s_________\\/%s\n\n", RED, BLUE, NORMAL);
   printf("%sEnter /help to view a list of available commands.%s\n\n", WHITE, NORMAL);
   
   /*
   printf("%s", GREEN);
   printf(" ___       __   _______   ___       ________  ________  _____ ______   _______                      \n");
   printf("|\\  \\     |\\  \\|\\  ___ \\ |\\  \\     |\\   ____\\|\\   __  \\|\\   _ \\  _   \\|\\  ___ \\                     \n");
   printf("\\ \\  \\    \\ \\  \\ \\   __/|\\ \\  \\    \\ \\  \\___|\\ \\  \\|\\  \\ \\  \\\\\\__\\ \\  \\ \\   __/|                    \n");
   printf(" \\ \\  \\  __\\ \\  \\ \\  \\_|/_\\ \\  \\    \\ \\  \\    \\ \\  \\\\\\  \\ \\  \\\\|__| \\  \\ \\  \\_|/__                  \n");
   printf("  \\ \\  \\|\\__\\_\\  \\ \\  \\_|\\ \\ \\  \\____\\ \\  \\____\\ \\  \\\\\\  \\ \\  \\    \\ \\  \\ \\  \\_|\\ \\                 \n");
   printf("   \\ \\____________\\ \\_______\\ \\_______\\ \\_______\\ \\_______\\ \\__\\    \\ \\__\\ \\_______\\                \n");
   printf("    \\|____________|\\|_______|\\|_______|\\|_______|\\|_______|\\|__|     \\|__|\\|_______|                \n");
   printf(" _________  ________          _________  _______   ________  _____ ______   ________                \n");
   printf("|\\___   ___\\\\   __  \\        |\\___   ___\\\\  ___ \\ |\\   __  \\|\\   _ \\  _   \\|\\_____  \\               \n");
   printf("\\|___ \\  \\_\\ \\  \\|\\  \\       \\|___ \\  \\_\\ \\   __/|\\ \\  \\|\\  \\ \\  \\\\\\__\\ \\  \\|____|\\ /_              \n");
   printf("     \\ \\  \\ \\ \\  \\\\\\  \\           \\ \\  \\ \\ \\  \\_|/_\\ \\   __  \\ \\  \\\\|__| \\  \\    \\|\\  \\             \n");
   printf("      \\ \\  \\ \\ \\  \\\\\\  \\           \\ \\  \\ \\ \\  \\_|\\ \\ \\  \\ \\  \\ \\  \\    \\ \\  \\  __\\_\\  \\            \n");
   printf("       \\ \\__\\ \\ \\_______\\           \\ \\__\\ \\ \\_______\\ \\__\\ \\__\\ \\__\\    \\ \\__\\|\\_______\\           \n");
   printf("        \\|__|  \\|_______|            \\|__|  \\|_______|\\|__|\\|__|\\|__|     \\|__|\\|_______|           \n");
   printf(" ________  ___  ___  ________  _________  ________  ___       ___  _______   ________   _________   \n");
   printf("|\\   ____\\|\\  \\|\\  \\|\\   __  \\|\\___   ___\\\\   ____\\|\\  \\     |\\  \\|\\  ___ \\ |\\   ___  \\|\\___   ___\\ \n");
   printf("\\ \\  \\___|\\ \\  \\\\\\  \\ \\  \\|\\  \\|___ \\  \\_\\ \\  \\___|\\ \\  \\    \\ \\  \\ \\   __/|\\ \\  \\\\ \\  \\|___ \\  \\_| \n");
   printf(" \\ \\  \\    \\ \\   __  \\ \\   __  \\   \\ \\  \\ \\ \\  \\    \\ \\  \\    \\ \\  \\ \\  \\_|/_\\ \\  \\\\ \\  \\   \\ \\  \\  \n");
   printf("  \\ \\  \\____\\ \\  \\ \\  \\ \\  \\ \\  \\   \\ \\  \\ \\ \\  \\____\\ \\  \\____\\ \\  \\ \\  \\_|\\ \\ \\  \\\\ \\  \\   \\ \\  \\ \n");
   printf("   \\ \\_______\\ \\__\\ \\__\\ \\__\\ \\__\\   \\ \\__\\ \\ \\_______\\ \\_______\\ \\__\\ \\_______\\ \\__\\\\ \\__\\   \\ \\__\\\n");
   printf("    \\|_______|\\|__|\\|__|\\|__|\\|__|    \\|__|  \\|_______|\\|_______|\\|__|\\|_______|\\|__| \\|__|    \\|__|\n");
   printf("                                                                                                    \n");
   printf("                                                                                                    \n");
   printf("                                                                                                    \n");
   printf("%s", NORMAL);
   */
}

