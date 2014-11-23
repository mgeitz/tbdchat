/* 
//   Program:             TBD Chat Client
//   File Name:           chat_client.c
//   Authors:             Matthew Owens, Michael Geitz, Shayne Wierbowski
//   Date Started:        10/23/2014
//   Compile:             gcc -Wall -l pthread client_commands.c chat_client.c -o chat_client
//   Run:                 ./chat_client
//
//   The client for a simple chat utility
*/
#include "chat_client.h"

int serverfd = 0;
pthread_mutex_t roomMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t nameMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t debugModeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t configFileMutex = PTHREAD_MUTEX_INITIALIZER;
volatile int currentRoom;
volatile int debugMode;
char realname[64];
char username[64];
pthread_t chat_rx_thread;
char *config_file;
char *USERCOLORS[4] = {BLUE, CYAN, MAGENTA, GREEN};

// Curses Windows
WINDOW *mainWin, *inputWin, *chatWin, *chatWinBox, *inputWinBox;

int main(int argc, char **argv) {
   int bufSize, send_flag;
   packet tx_pkt;
   struct tm *timestamp;
   char *config_file_name = CONFIG_FILENAME;
   char full_config_path[64];
   packet *tx_pkt_ptr = &tx_pkt;


   // Sig Handlers
   signal(SIGINT, sigintHandler);
   signal(SIGWINCH, resizeHandler);

   // Initialize curses
   if ((mainWin = initscr()) == NULL) { exit(1); }
   chatWinBox = subwin(mainWin, (LINES * 0.8), COLS, 0, 0);
   box(chatWinBox, 0, 0);
   chatWin = subwin(chatWinBox, (LINES * 0.8 - 2), COLS - 2, 1, 1);
   scrollok(chatWin, TRUE);
   //box(chatWin, 0, 0);
   
   inputWinBox = subwin(mainWin, (LINES * 0.2), COLS, (LINES * 0.8), 0);
   box(inputWinBox, 0, 0);
   inputWin = subwin(inputWinBox, (LINES * 0.2) - 2, COLS - 2, (LINES * 0.8) + 1, 1);

   cbreak();
   noecho();
   keypad(mainWin, TRUE);

   //setup_screens();
   //text_win = create_text_window();
   //wrefresh(text_win);
   //in_win = create_input_window();
   //wrefresh(in_win);

   // Get home dir, check config
   strcpy(full_config_path, getenv("HOME"));
   strcat(full_config_path, config_file_name);
   config_file = full_config_path;
   // If config does not exist, create the default config file
   if (access(config_file, F_OK) == -1) {
      buildDefaultConfig();
   }

   //wprintw(chatWin, "\33[2J\33[H"); // Removed anticipating curses
   asciiSplash();

   // Check autoconnect, run if set
   if (auto_connect()) {
      wprintw(chatWin, " Auto connecting to most recently connected host . . .\n");
      reconnect(tx_pkt.buf);
   }
  
   // Primary execution loop 
   while (1) {
      wrefresh(chatWin);
      // Wipe packet space
      memset(&tx_pkt, 0, sizeof(packet));
      // Set packet options as untouched
      tx_pkt.options = INVALID;
      // Read user kb input, return number of chars
      bufSize = userInput(tx_pkt_ptr);
      // Set default send flag to True
      send_flag = 1;
      // If the input buffer is not empty
      if(bufSize > 0 && tx_pkt.buf[bufSize] != EOF) {
         // Check if the input should be read as a command, if so process the command
         if(strncmp("/", (void *)tx_pkt.buf, 1) == 0) {
             
             send_flag = userCommand(tx_pkt_ptr);
         }
         // If connected and handling a packet flagged to be sent
         if (send_flag && serverfd) {
            // Copy current username and realname to packet
            pthread_mutex_lock(&nameMutex);
            strcpy(tx_pkt.username, username);
            strcpy(tx_pkt.realname, realname);
            pthread_mutex_unlock(&nameMutex);
            // Timestamp packet
            tx_pkt.timestamp = time(NULL);
            // If sending a message, print the message client side
            pthread_mutex_lock(&roomMutex);
            if (currentRoom >= 1000 && tx_pkt.options == -1) {
               timestamp = localtime(&(tx_pkt.timestamp));
               wprintw(chatWin, "  %d:%d:%d  | [%s] %s\n", timestamp->tm_hour, timestamp->tm_min, timestamp->tm_sec, \
                       tx_pkt.realname, tx_pkt.buf);
               tx_pkt.options = currentRoom;
            }
            pthread_mutex_unlock(&roomMutex);
            // If packet options has been altered appropriately, send it
            if (tx_pkt.options > 0) {
               send(serverfd, (void *)&tx_pkt, sizeof(packet), MSG_NOSIGNAL);
            }
         }
         // If send flag is true but serverfd is still 0, print error
         else if (send_flag && !serverfd)  {
            wprintw(chatWin, " --- Error: Not connected to any server. See /help for command usage.\n");
         } 
      }
      // If an exit packet was just transmitted, break from primary execution loop
      if (tx_pkt.options == EXIT) {
         break;
      }
   }
   
   // Safely close connection
   wprintw(chatWin, " Preparing to exit . . .\n");
   wrefresh(chatWin);
   close(serverfd);
   // Join chatRX if it was launched
   if (chat_rx_thread) {
      if(pthread_join(chat_rx_thread, NULL)) {
         wprintw(chatWin, " --- Error: chatRX thread not joining.\n");
      }
   }
   // Destroy mutexes
   pthread_mutex_destroy(&nameMutex);
   pthread_mutex_destroy(&debugModeMutex);
   pthread_mutex_destroy(&configFileMutex);
   pthread_mutex_destroy(&roomMutex);
   // Close curses
   wprintw(chatWin, " Exiting client.\n");
   wrefresh(chatWin);
   endwin();
   exit(0);
}


/* Build the default config file */
void buildDefaultConfig() {
      pthread_mutex_lock(&configFileMutex);
      FILE *configfp;
      configfp = fopen(config_file, "a+");
      fputs("###\n#\n#\tTBD Chat Configuration\n#\n###\n\n\n", configfp);
      fputs("## Stores auto reconnect state\nauto-reconnect: 0\n\n", configfp);
      fputs("## Stores last client connection\nlast connection:\n", configfp);
      fclose(configfp);
      pthread_mutex_unlock(&configFileMutex);
}


/* Check if auto-reconnect is set enabled */
int auto_connect() {
   FILE *configfp;
   char line[128];
   
   pthread_mutex_lock(&configFileMutex);
   configfp = fopen(config_file, "r");
   if (configfp != NULL) {
      while (!feof(configfp)) {
         if (fgets(line, sizeof(line), configfp)) {
            if (strncmp(line, "auto-reconnect:", strlen("auto-reconnect:")) == 0) {
               fclose(configfp);
               pthread_mutex_unlock(&configFileMutex);
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
   pthread_mutex_unlock(&configFileMutex);
   return 0;
}


/* Read keyboard input into buffer */
int userInput(packet *tx_pkt) {
   int i = 0;
   int ch;
   wmove(inputWin, 0, 0);
   // Read 1 char at a time
   while ((ch = getch()) != '\n') {
      // Backspace
      if (ch == 8 || ch == 127 || ch == KEY_LEFT) {
         if (i > 0) {
            wprintw(inputWin, "\b \b\0");
            tx_pkt->buf[--i] = '\0';
            wrefresh(inputWin);
         }
         else {
            wprintw(inputWin, "\b \0");
         }
      }
      // Otherwise put in buffer
      else {
         if (i < BUFFERSIZE - 1) {
            strcat(tx_pkt->buf, (char *)&ch);
            i++;
            wprintw(inputWin, (char *)&ch);
            wrefresh(inputWin);
         }
         // Unless buffer is full
         else {
            wprintw(inputWin, "\b%s", (char *)&ch);
            tx_pkt->buf[(i - 1)] = '\0';
            strcat(tx_pkt->buf, (char *)&ch);
            wrefresh(inputWin);
         }
      }
   }
   // Null terminate, clear input window
   tx_pkt->buf[i] = '\0';
   wclear(inputWin);
   wrefresh(inputWin);
   return i;
}


/* Print messages as they are received */
void *chatRX(void *ptr) {
   packet rx_pkt;
   packet *rx_pkt_ptr = &rx_pkt;
   int received;
   int *serverfd = (int *)ptr;
   struct tm *timestamp;
   while(1) {
      // Wait for message to arrive..
      received = recv(*serverfd, (void *)&rx_pkt, sizeof(packet), 0);
      
      if(received) {
         // If debug mode is enabled, dump packet contents
         pthread_mutex_lock(&debugModeMutex);
         if (debugMode) {
            debugPacket(rx_pkt_ptr);
         }
         pthread_mutex_unlock(&debugModeMutex);
         // If the received packet is a message packet, print accordingly
         if (rx_pkt.options >= 1000) {
            timestamp = localtime(&(rx_pkt.timestamp));
            if(strcmp(rx_pkt.realname, SERVER_NAME) == 0) {
               wprintw(chatWin, "  %d:%d:%d  | [%s] %s\n", timestamp->tm_hour, timestamp->tm_min, timestamp->tm_sec, \
                       rx_pkt.realname, rx_pkt.buf);
            }
            else {
               //int i = hash(rx_pkt.username);
               wprintw(chatWin, "  %d:%d:%d  | [%s] %s\n", timestamp->tm_hour, timestamp->tm_min, timestamp->tm_sec, \
                       rx_pkt.realname, rx_pkt.buf);
            }
         }
         // If the received packet is a nonmessage option, handle option response
         else if (rx_pkt.options > 0 && rx_pkt.options < 1000) {
            serverResponse(rx_pkt_ptr);
         }
         // If the received packet contains 0 as the option, we likely received and empty packet, end transmission
         else {
            wprintw(chatWin, " Communication with server has terminated.\n");
            wrefresh(chatWin);
            break;
         }
      }
      // Wipe packet space
      wrefresh(chatWin);
      memset(&rx_pkt, 0, sizeof(packet));
   }
   return NULL;
}


/* Handle non message packets from server */
void serverResponse(packet *rx_pkt) {
   if (rx_pkt->options == SERV_ERR) {
      wprintw(chatWin, " --- Error: %s\n", rx_pkt->buf);
   }
   else if (rx_pkt->options == REGSUC) {
      pthread_mutex_lock(&roomMutex);
      currentRoom = DEFAULT_ROOM;
      pthread_mutex_unlock(&roomMutex);
      wprintw(chatWin, " --- Success: Registration successful!\n");
   }
   else if (rx_pkt->options == LOGSUC) {
      pthread_mutex_lock(&nameMutex);
      strcpy(username, rx_pkt->username);
      strcpy(realname, rx_pkt->realname);
      pthread_mutex_unlock(&nameMutex);
      pthread_mutex_lock(&roomMutex);
      currentRoom = DEFAULT_ROOM;
      pthread_mutex_unlock(&roomMutex);
      wprintw(chatWin, " --- Success: Login successful!\n");
   }
   else if (rx_pkt->options == GETUSERS || \
            rx_pkt->options == GETALLUSERS || \
            rx_pkt->options == GETUSER) {
      wprintw(chatWin, " --- User: %s\n", rx_pkt->buf);
   }
   else if (rx_pkt->options == PASSSUC) {
      wprintw(chatWin, " --- Success: Password change successful!\n");
   }
   else if (rx_pkt->options == NAMESUC) {
      pthread_mutex_lock(&nameMutex);
      memset(&realname, 0, sizeof(realname));
      strncpy(realname, rx_pkt->buf, sizeof(realname));
      pthread_mutex_unlock(&nameMutex);
      wprintw(chatWin, " --- Success: Name change successful!\n");
   }
   else if (rx_pkt->options == JOINSUC) {
      newRoom((void *)rx_pkt->buf);
   }
   else if (rx_pkt->options == INVITE) {
      wprintw(chatWin, " --- Invite: %s\n", rx_pkt->buf);
   }
   else if (rx_pkt->options == INVITESUC) {
      wprintw(chatWin, " --- Success: Invite sent!\n");
   }
   else if (rx_pkt->options == GETROOMS) {
      wprintw(chatWin, " --- Room: %s\n", rx_pkt->buf);
   }
   else if (rx_pkt->options == MOTD) {
      wprintw(chatWin, " ------------------------------------------------------------------- \n");
      wprintw(chatWin, " %s\n", rx_pkt->buf);
      wprintw(chatWin, " ------------------------------------------------------------------- \n");
   }
   else if(rx_pkt->options == EXIT) {
      wprintw(chatWin, " Server has closed its connection with you.\n");
      wprintw(chatWin, " Closing socket connection with server.\n");
      close(serverfd);
   }
   else {
      wprintw(chatWin, " --- Error: Unknown message received from server.\n");
   }
}


/* Change the clients current room (for sending) */
void newRoom(char *buf) {
   int i = 0, roomNumber;
   char *args[16];
   char cpy[BUFFERSIZE];
   char *tmp = cpy;
   strcpy(tmp, buf);

   args[i] = strsep(&tmp, " \t");
   while ((i < sizeof(args) - 1) && (args[i] != '\0')) {
       args[++i] = strsep(&tmp, " \t");
   }
   if (i >= 1) {
      roomNumber = atoi(args[1]);
      pthread_mutex_lock(&roomMutex);
      if (roomNumber != currentRoom) {
         currentRoom = roomNumber;
         wprintw(chatWin, " --- Success: Joined room %s.\n", \
                args[0]);
      }
      pthread_mutex_unlock(&roomMutex);
   }
   else {
      wprintw(chatWin, " --- Error: Problem reading JOINSUC from server.\n");

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
      wprintw(chatWin, " getaddrinfo: %s\n", gai_strerror(status));
      return -1;
   }
   
   print_ip(servinfo);
   for (p = servinfo; p != NULL; p = p ->ai_next) {
      if((serverfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
         wprintw(chatWin, " --- Error: socket socket \n");
         continue;
      }
      
      if(connect(serverfd, p->ai_addr, p->ai_addrlen) == -1) {
         close(serverfd);
         wprintw(chatWin, " --- Error: socket connect \n");
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
      wprintw(chatWin, " Connecting to %s: %s:%d . . .\n", ipver, ipstr, ntohs(port));
   }
}


/* Handle SIGINT (CTRL+C) */
void sigintHandler(int sig_num) {
   wprintw(chatWin, "\b\b --- Error: Forced Exit.\n");
   // If the client is connected, safely close the connection
   if (serverfd) { 
      packet tx_pkt;
      pthread_mutex_lock(&nameMutex);
      strcpy(tx_pkt.username, username);
      strcpy(tx_pkt.realname, realname);
      strcpy(tx_pkt.buf, "/exit");
      pthread_mutex_unlock(&nameMutex);
      tx_pkt.timestamp = time(NULL);
      tx_pkt.options = EXIT;
      send(serverfd, (void *)&tx_pkt, sizeof(packet), 0);
      close(serverfd); 
      if (chat_rx_thread) {
         if(pthread_join(chat_rx_thread, NULL)) {
            wprintw(chatWin, " --- Error: chatRX thread not joining.\n");
         }
      }
   }
   wrefresh(chatWin);
   pthread_mutex_destroy(&nameMutex);
   pthread_mutex_destroy(&debugModeMutex);
   pthread_mutex_destroy(&configFileMutex);
   pthread_mutex_destroy(&roomMutex);
   endwin();
   exit(0);
}


/* Handle window resizing */
void resizeHandler(int sig) {
   // This currently is not working, although has stopped segfault on resize

   //int nh, nw;
   //getmaxyx(stdscr, nh, nw);
}


/* Print message on startup */
void asciiSplash() {
   wprintw(chatWin, "\n");
   wprintw(chatWin, "         __\n");
   wprintw(chatWin, "        /_/\\        _____ ____  ____     ____ _           _   \n");
   wprintw(chatWin, "       / /\\ \\      |_   _| __ )|  _ \\   / ___| |__   __ _| |_ \n");
   wprintw(chatWin, "      / / /\\ \\       | | |  _ \\| | | | | |   | '_ \\ / _` | __|\n");
   wprintw(chatWin, "     / / /\\ \\ \\      | | | |_) | |_| | | |___| | | | (_| | |_ \n");
   wprintw(chatWin, "    / /_/__\\ \\ \\     |_| |____/|____/   \\____|_| |_|\\__,_|\\__|\n");
   wprintw(chatWin, "   /_/______\\_\\/\\\n");
   wprintw(chatWin, "   \\_\\_________\\/\n\n");
   wprintw(chatWin, " Enter /help to view a list of available commands.\n\n");
   wrefresh(chatWin);
}


/* Return number between 0-4 determined from string passed in */
int hash(char *str) {
   unsigned long hash = 5381;
   int c;
   while ((c = *str++)) {
      hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
   }
   return hash % 4;
}
