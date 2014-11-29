/* 
//   Program:             TBD Chat Client
//   File Name:           chat_client.c
//   Authors:             Matthew Owens, Michael Geitz, Shayne Wierbowski
//   Date Started:        10/23/2014
//   Compile:             gcc -Wformat -Wall -lpthread -lncurses visual.c client_commands.c chat_client.c -o chat_client
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
//char *USERCOLORS[4] = {BLUE, CYAN, MAGENTA, GREEN};

// Curses Windows
WINDOW *mainWin, *inputWin, *chatWin, *chatWinBox, *inputWinBox, *infoLine, *infoLineBottom;

int main(int argc, char **argv) {
   int bufSize, send_flag;
   packet tx_pkt;
   //struct tm *timestamp;
   char *config_file_name = CONFIG_FILENAME;
   char full_config_path[64];
   //int LINES, COLS;
   packet *tx_pkt_ptr = &tx_pkt;


   // Sig Handlers
   signal(SIGINT, sigintHandler);
   signal(SIGWINCH, resizeHandler);

   // Initialize curses
   if ((mainWin = initscr()) == NULL) { exit(1); }
   // Start colors
   start_color();
   use_default_colors();
   // Do not echo key presses
   noecho();
   // Read input one char at a time
   cbreak();
   // Capture special keys
   keypad(mainWin, TRUE);
 
   // Initialize color types
   init_pair(1, -1, -1); // Default                             
   init_pair(2, COLOR_CYAN, -1);                      
   init_pair(3, COLOR_YELLOW, -1);                    
   init_pair(4, COLOR_RED, -1);                       
   init_pair(5, COLOR_BLUE, -1);
   init_pair(6, COLOR_MAGENTA, -1);
   init_pair(7, COLOR_GREEN, -1);
   init_pair(8, COLOR_WHITE, COLOR_RED);
   init_pair(9, COLOR_WHITE, COLOR_BLUE);
   init_pair(10, COLOR_WHITE, COLOR_GREEN);
   init_pair(11, COLOR_BLACK, COLOR_MAGENTA);

   // Create chat box and window
   chatWinBox = subwin(mainWin, (LINES * 0.8), COLS, 0, 0);
   box(chatWinBox, 0, 0);
   mvwaddch(chatWinBox, 0, (COLS * 0.5) - 6, ACS_RTEE);
   wattron(chatWinBox, COLOR_PAIR(3));
   mvwaddstr(chatWinBox, 0, (COLS * 0.5) - 5, " TBDChat " );
   wattroff(chatWinBox, COLOR_PAIR(3));
   mvwaddch(chatWinBox, 0, (COLS * 0.5) + 4, ACS_LTEE );
   wrefresh(chatWinBox);
   chatWin = subwin(chatWinBox, (LINES * 0.8 - 2), COLS - 2, 1, 1);
   scrollok(chatWin, TRUE);

   // Create info lines
   infoLine = subwin(mainWin, 1, COLS, (LINES * 0.8), 0);
   wbkgd(infoLine, COLOR_PAIR(3));
   wprintw(infoLine, " Type /help to view a list of available commands");
   wrefresh(infoLine);
   wrefresh(infoLineBottom);
   infoLineBottom = subwin(mainWin, 1, COLS, LINES - 1, 0);

   // Create input box and window
   inputWinBox = subwin(mainWin, (LINES * 0.2) - 1, COLS, (LINES * 0.8) + 1, 0);
   box(inputWinBox, 0, 0);
   inputWin = subwin(inputWinBox, (LINES * 0.2) - 3, COLS - 2, (LINES * 0.8) + 2, 1);

   // Get home dir, check config
   strcpy(full_config_path, getenv("HOME"));
   strcat(full_config_path, config_file_name);
   config_file = full_config_path;
   // If config does not exist, create the default config file
   if (access(config_file, F_OK) == -1) {
      buildDefaultConfig();
   }

   asciiSplash();

   // Check autoconnect, run if set
   if (auto_connect()) {
      wprintFormat(chatWin, time(NULL), "Client", "Auto connecting to most recently connected host . . .", 1);
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
               //timestamp = localtime(&(tx_pkt.timestamp));
               wprintFormat(chatWin, tx_pkt.timestamp, tx_pkt.realname, tx_pkt.buf, 2);
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
            wprintFormat(chatWin, time(NULL), "Error", "Not connected to any server. See /help for command usage.", 8);
         } 
      }
      // If an exit packet was just transmitted, break from primary execution loop
      if (tx_pkt.options == EXIT) {
         break;
      }
   }
   
   // Safely close connection
   wprintFormat(chatWin, time(NULL), "Client", " Preparing to exit . . .", 1);
   wrefresh(chatWin);
   close(serverfd);
   // Join chatRX if it was launched
   if (chat_rx_thread) {
      if(pthread_join(chat_rx_thread, NULL)) {
         wprintFormat(chatWin, time(NULL), "Error", "chatRX thread not joining.", 8);
      }
   }
   // Destroy mutexes
   pthread_mutex_destroy(&nameMutex);
   pthread_mutex_destroy(&debugModeMutex);
   pthread_mutex_destroy(&configFileMutex);
   pthread_mutex_destroy(&roomMutex);
   // Close curses
   wprintFormat(chatWin, time(NULL), "Client", " Exiting client.", 1);
   wrefresh(chatWin);
   endwin();
   printf("\33[2J\33[H");
   printf("Thanks for using TBD Chat.\n");
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
      else if (ch == KEY_RESIZE) {
         continue;
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
               wprintFormat(chatWin, rx_pkt.timestamp, rx_pkt.realname, rx_pkt.buf, 3);
            }
            else {
               int i = hash(rx_pkt.username, 3);
               wprintFormat(chatWin, rx_pkt.timestamp, rx_pkt.realname, rx_pkt.buf, i + 4);  
            }
         }
         // If the received packet is a nonmessage option, handle option response
         else if (rx_pkt.options > 0 && rx_pkt.options < 1000) {
            serverResponse(rx_pkt_ptr);
         }
         // If the received packet contains 0 as the option, we likely received and empty packet, end transmission
         else {
            wprintFormat(chatWin, time(NULL), "Client", " Communication with server has terminated.", 1);
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
      wprintFormat(chatWin, time(NULL), rx_pkt->realname, rx_pkt->buf, 3);
   }
   //else if (rx_pkt->options == REGSUC) {
   //   pthread_mutex_lock(&roomMutex);
   //   currentRoom = DEFAULT_ROOM;
   //   pthread_mutex_unlock(&roomMutex);
   //   wprintFormat(chatWin, time(NULL), "Client", " --- Success: Registration successful!\n");
   //}
   else if (rx_pkt->options == LOGSUC) {
      pthread_mutex_lock(&nameMutex);
      strcpy(username, rx_pkt->username);
      strcpy(realname, rx_pkt->realname);
      pthread_mutex_unlock(&nameMutex);
      pthread_mutex_lock(&roomMutex);
      currentRoom = DEFAULT_ROOM;
      werase(infoLine);
      wprintw(infoLine, " Current room: Lobby"); 
      wrefresh(infoLine);
      pthread_mutex_unlock(&roomMutex);
      wprintFormat(chatWin, rx_pkt->timestamp, rx_pkt->realname, "Login Successful!", 2);
   }
   else if (rx_pkt->options == GETUSERS || \
            rx_pkt->options == GETALLUSERS || \
            rx_pkt->options == GETUSER) {
      wprintFormat(chatWin, rx_pkt->timestamp, rx_pkt->realname, rx_pkt->buf, 8);
   }
   else if (rx_pkt->options == PASSSUC) {
      wprintFormat(chatWin, rx_pkt->timestamp, rx_pkt->realname, "Password change successful", 3);
   }
   else if (rx_pkt->options == NAMESUC) {
      pthread_mutex_lock(&nameMutex);
      memset(&realname, 0, sizeof(realname));
      strncpy(realname, rx_pkt->buf, sizeof(realname));
      pthread_mutex_unlock(&nameMutex);
      wprintFormat(chatWin, rx_pkt->timestamp, rx_pkt->realname, "Name change successful", 3);
   }
   else if (rx_pkt->options == JOINSUC) {
      newRoom(rx_pkt);
   }
   else if (rx_pkt->options == INVITE) {
      wprintFormat(chatWin, rx_pkt->timestamp, rx_pkt->realname, rx_pkt->buf, 3);
   }
   else if (rx_pkt->options == INVITESUC) {
      wprintFormat(chatWin, rx_pkt->timestamp, rx_pkt->realname, "Invite sent", 3);
   }
   else if (rx_pkt->options == GETROOMS) {
      wprintFormat(chatWin, rx_pkt->timestamp, rx_pkt->realname, rx_pkt->buf, 8);
   }
   else if (rx_pkt->options == MOTD) {
      wprintSeperatorTitle(chatWin, "MOTD", 1, 2);

      wprintFormatTime(chatWin, rx_pkt->timestamp);
      wattron(chatWin, COLOR_PAIR(5));
      wprintw(chatWin, "%s\n", rx_pkt->buf);
      wattroff(chatWin, COLOR_PAIR(7));

      wprintSeperator(chatWin, 1);
   }
   else if(rx_pkt->options == EXIT) {
      wprintFormat(chatWin, rx_pkt->timestamp, rx_pkt->realname, "Server has closed its connection with you", 3);
      wprintFormat(chatWin, rx_pkt->timestamp, "Client", "Closing socket connection with server", 1);
      close(serverfd);
   }
   else {
      wprintFormat(chatWin, time(NULL), "Error", "Unknown message received from server", 8);
   }
}


/* Print with a timestamp */
void wprintFormat(WINDOW *win, time_t ts, char *from, char *buf, int from_color) {

   // Print formatted time
   wprintFormatTime(win, ts);

   // Print from and buffer
   wattron(win, COLOR_PAIR(1));
   wprintw(win, "[");
   wattroff(win, COLOR_PAIR(1));
   wattron(win, COLOR_PAIR(from_color));
   wprintw(win, "%s", from);
   wattroff(win, COLOR_PAIR(from_color));
   wattron(win, COLOR_PAIR(1));
   wprintw(win, "] %s\n", buf);
   wattroff(win, COLOR_PAIR(1));

}


/* Print formatted and colored error */
void wprintFormatError(WINDOW *win, time_t ts, char *buf) {
   // Print formatted time
   wprintFormatTime(win, ts);

   wattron(win, A_BOLD);
   wprintw(chatWin, " ");
   waddch(chatWin, ACS_HLINE);
   waddch(chatWin, ACS_HLINE);
   waddch(chatWin, ACS_HLINE);
   wprintw(chatWin, " ");
   wattron(win, COLOR_PAIR(8));
   wprintw(chatWin, "Error");
   wattroff(win, COLOR_PAIR(8));
   wattroff(win, A_BOLD);
   wattron(win, COLOR_PAIR(1));
   wprintw(chatWin, " %s\n", buf);
   wattroff(win, COLOR_PAIR(1));
}


/* Print properly formatted and colored timestamp */
void wprintFormatTime(WINDOW *win, time_t ts) {
   struct tm *timestamp;
   timestamp = localtime(&ts); 

   wattron(win, COLOR_PAIR(1));
   wprintw(win, "%02d", timestamp->tm_hour);
   wattroff(win, COLOR_PAIR(1));
   wattron(win, COLOR_PAIR(3));
   wprintw(win, ":");
   wattroff(win, COLOR_PAIR(3));
   wattron(win, COLOR_PAIR(1));
   wprintw(win, "%02d", timestamp->tm_min);
   wattroff(win, COLOR_PAIR(1));
   wattron(win, COLOR_PAIR(3));
   wprintw(win, ":");
   wattroff(win, COLOR_PAIR(3));
   wattron(win, COLOR_PAIR(1));
   wprintw(win, "%02d", timestamp->tm_sec);
   wattroff(win, COLOR_PAIR(1));

   wattron(win, COLOR_PAIR(7));
   wprintw(win, " ");
   waddch(win, ACS_VLINE);
   wprintw(win, " ");
   wattroff(win, COLOR_PAIR(7));
}


/* Print seperator bar with title 2/3rd the length of the window passed in */
void wprintSeperatorTitle(WINDOW *win, char *title, int color, int title_color) {
   int height, width, i;
   getmaxyx(win, height, width);

   // Print formatted time
   wprintFormatTime(win, time(NULL));
   wattron(win, COLOR_PAIR(color));
   for (i = 0; i < ((width / 3) - ((strlen(title) + 2) / 2)); i++) {
      waddch(win, ACS_HLINE);
   }
   waddch(win, ACS_RTEE);
   wattroff(win, COLOR_PAIR(color));
   wattron(win, COLOR_PAIR(title_color));
   wprintw(win, " %s ", title);
   wattroff(win, COLOR_PAIR(title_color));
   wattron(win, COLOR_PAIR(color));
   waddch(win, ACS_LTEE);
   for (i = 0; i < ((width / 3) - ((strlen(title) + 2) / 2)); i++) {
      waddch(win, ACS_HLINE);
   }
   wprintw(win, "\n");
   wattroff(win, COLOR_PAIR(color));
}


void wprintSeperator(WINDOW *win, int color) {
   int height, width, i;
   getmaxyx(win, height, width);

   wprintFormatTime(win, time(NULL));
   wattron(win, COLOR_PAIR(color));
   for (i = 0; i < (2 * (width / 3) + 2); i++) {
      waddch(win, ACS_HLINE);
   }
   wprintw(win, "\n");
   wattroff(win, COLOR_PAIR(color));
}


/* Change the clients current room (for sending) */
void newRoom(packet *rx_pkt) {
   int i = 0, roomNumber;
   char *args[16];
   char cpy[BUFFERSIZE];
   char *tmp = cpy;
   strcpy(tmp, rx_pkt->buf);

   args[i] = strsep(&tmp, " \t");
   while ((i < sizeof(args) - 1) && (args[i] != '\0')) {
       args[++i] = strsep(&tmp, " \t");
   }
   if (i >= 1) {
      roomNumber = atoi(args[1]);
      pthread_mutex_lock(&roomMutex);
      if (roomNumber != currentRoom) {
         currentRoom = roomNumber;
         wprintFormatTime(chatWin, rx_pkt->timestamp);
         wprintw(chatWin, "Joined Room ");
         wattron(chatWin, COLOR_PAIR(3));
         wprintw(chatWin, "%s\n", args[0]);
         wattroff(chatWin, COLOR_PAIR(3));
         werase(infoLine);
         wbkgd(infoLine, COLOR_PAIR(3));
         wprintw(infoLine, " Current room: %s", args[0]); 
         wrefresh(infoLine);
      }
      pthread_mutex_unlock(&roomMutex);
   }
   else {
         wprintFormat(chatWin, time(NULL), "Error", "Problem reading JOINSUC.", 8);
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
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "getaddrinfo: %s\n", gai_strerror(status));
      return -1;
   }
   
   for (p = servinfo; p != NULL; p = p ->ai_next) {
      if((serverfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
         wprintFormat(chatWin, time(NULL), "Error", "socket socket", 8);
         continue;
      }
      
      if(connect(serverfd, p->ai_addr, p->ai_addrlen) == -1) {
         close(serverfd);
         wprintFormat(chatWin, time(NULL), "Error", "socket connect", 8);
         return -1;
      }
      break;
   }
   print_ip(servinfo);
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
         ipver = "IPv4";
      }
      else {
         ipv6= (struct sockaddr_in6 *)p->ai_addr;
         addr = &(ipv6->sin6_addr);
         port = ipv4->sin_port;
         ipver = "IPv6";
      }
      // Write readable form of IP to ipstr
      inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
      // Print connection information
      wbkgd(infoLineBottom, COLOR_PAIR(3));
      wprintw(infoLineBottom, " Connected to [%s] %s:%d", ipver, ipstr, ntohs(port));
      wrefresh(infoLineBottom);
   }
}


/* Handle SIGINT (CTRL+C) */
void sigintHandler(int sig_num) {
   wprintFormat(chatWin, time(NULL), "Error", "\b\bForced Exit.", 8);
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
            wprintFormat(chatWin, time(NULL), "Error", "chatRX thread not joining.", 8);
         }
      }
   }
   wrefresh(chatWin);
   pthread_mutex_destroy(&nameMutex);
   pthread_mutex_destroy(&debugModeMutex);
   pthread_mutex_destroy(&configFileMutex);
   pthread_mutex_destroy(&roomMutex);
   endwin();
   printf("\33[2J\33[H");
   printf("Thanks for using TBD Chat.\n");
   exit(0);
}


/* Handle window resizing */
void resizeHandler(int sig) {
   endwin();
   refresh();
   clear();

   // Create chat box and window
   chatWinBox = subwin(mainWin, (LINES * 0.8), COLS, 0, 0);
   box(chatWinBox, 0, 0);
   mvwaddch(chatWinBox, 0, (COLS * 0.5) - 6, ACS_RTEE);
   wattron(chatWinBox, COLOR_PAIR(3));
   mvwaddstr(chatWinBox, 0, (COLS * 0.5) - 5, " TBDChat " );
   wattroff(chatWinBox, COLOR_PAIR(3));
   mvwaddch(chatWinBox, 0, (COLS * 0.5) + 4, ACS_LTEE );
   wrefresh(chatWinBox);
   chatWin = subwin(chatWinBox, (LINES * 0.8 - 2), COLS - 2, 1, 1);
   scrollok(chatWin, TRUE);

   // Create info lines
   infoLine = subwin(mainWin, 1, COLS, (LINES * 0.8), 0);
   wbkgd(infoLine, COLOR_PAIR(3));
   wbkgd(infoLineBottom, COLOR_PAIR(3));
   wprintw(infoLine, " Type /help to view a list of available commands");
   wrefresh(infoLine);
   wrefresh(infoLineBottom);
   infoLineBottom = subwin(mainWin, 1, COLS, LINES - 1, 0);

   // Create input box and window
   inputWinBox = subwin(mainWin, (LINES * 0.2) - 1, COLS, (LINES * 0.8) + 1, 0);
   box(inputWinBox, 0, 0);
   inputWin = subwin(inputWinBox, (LINES * 0.2) - 3, COLS - 2, (LINES * 0.8) + 2, 1);
}


/* Print message on startup */
void asciiSplash() {
   int icon = 2;
   int word = 1;
   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, COLOR_PAIR(icon));
   wprintw(chatWin, "         __\n");
   wattroff(chatWin, COLOR_PAIR(icon));
   
   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, COLOR_PAIR(icon));
   wprintw(chatWin, "        /_/\\       ");
   wattroff(chatWin, COLOR_PAIR(icon));
   wattron(chatWin, COLOR_PAIR(word));
   wprintw(chatWin, " _____ ____  ____     ____ _           _   \n");
   wattroff(chatWin, COLOR_PAIR(word));

   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, COLOR_PAIR(icon));
   wprintw(chatWin, "       / /\\ \\      ");
   wattroff(chatWin, COLOR_PAIR(icon));
   wattron(chatWin, COLOR_PAIR(word));
   wprintw(chatWin, "|_   _| __ )|  _ \\   / ___| |__   __ _| |_ \n");
   wattroff(chatWin, COLOR_PAIR(word));

   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, COLOR_PAIR(icon));
   wprintw(chatWin, "      / / /\\ \\      ");
   wattroff(chatWin, COLOR_PAIR(icon));
   wattron(chatWin, COLOR_PAIR(word));
   wprintw(chatWin, " | | |  _ \\| | | | | |   | '_ \\ / _` | __|\n");
   wattroff(chatWin, COLOR_PAIR(word));

   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, COLOR_PAIR(icon));
   wprintw(chatWin, "     / / /\\ \\ \\     ");
   wattroff(chatWin, COLOR_PAIR(icon));
   wattron(chatWin, COLOR_PAIR(word));
   wprintw(chatWin, " | | | |_) | |_| | | |___| | | | (_| | |_ \n");
   wattroff(chatWin, COLOR_PAIR(word));

   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, COLOR_PAIR(icon));
   wprintw(chatWin, "    / /_/__\\ \\ \\    ");
   wattroff(chatWin, COLOR_PAIR(icon));
   wattron(chatWin, COLOR_PAIR(word));
   wprintw(chatWin, " |_| |____/|____/   \\____|_| |_|\\__,_|\\__|\n");
   wattroff(chatWin, COLOR_PAIR(word));

   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, COLOR_PAIR(icon));
   wprintw(chatWin, "   /_/______\\_\\/\\\n");
   wattroff(chatWin, COLOR_PAIR(icon));

   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, COLOR_PAIR(icon));
   wprintw(chatWin, "   \\_\\_________\\/\t");
   wattroff(chatWin, COLOR_PAIR(icon));
   wattron(chatWin, COLOR_PAIR(1));
   wprintw(chatWin, "version ");
   wattroff(chatWin, COLOR_PAIR(1));
   wattron(chatWin, COLOR_PAIR(3));
   wprintw(chatWin, "%s\n", VERSION);
   wattroff(chatWin, COLOR_PAIR(3));

   wprintFormatTime(chatWin, time(NULL));
   wprintw(chatWin, "\n");
   
   wrefresh(chatWin);
}


/* Return number between 0-(mod-1) determined from string passed in */
int hash(char *str, int mod) {
   unsigned long hash = 5381;
   int c;
   while ((c = *str++)) {
      //Binary shift left by 5, add c
      hash = ((hash << 5) + hash) + c; 
   } 
   // Return modded value
   return hash % mod;
}
