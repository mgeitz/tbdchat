/* 
//   Program:             TBD Chat Client
//   File Name:           client_commands.c
//   Authors:             Matthew Owens, Michael Geitz, Shayne Wierbowski
*/

#include "chat_client.h"

/* Declared.  Defined and allocated in chat_client.c */
extern int serverfd;
extern volatile int currentRoom;
extern volatile int debugMode;
extern char username[64];
extern char realname[64];
extern char *config_file;
extern WINDOW *mainWin, *chatWin, *inputWin;
extern pthread_t chat_rx_thread;
extern pthread_mutex_t roomMutex;
extern pthread_mutex_t nameMutex;
extern pthread_mutex_t debugModeMutex;
extern pthread_mutex_t configFileMutex;

/* Process user commands and mutate buffer accordingly */
int userCommand(packet *tx_pkt) {

   // Handle exit command
   if (strncmp((void *)tx_pkt->buf, "/exit", strlen("/exit")) == 0) {
       tx_pkt->options = EXIT;
       return 1;;
   }
   // Handle quit command
   else if (strncmp((void *)tx_pkt->buf, "/quit", strlen("/quit")) == 0) {
       tx_pkt->options = EXIT;
       return 1;;
   }
   // Handle help command
   else if (strncmp((void *)tx_pkt->buf, "/help", strlen("/help")) == 0) {
       showHelp((void *)tx_pkt->buf);
       return 0;
   }
   // Handle debug command
   else if (strncmp((void *)tx_pkt->buf, "/debug", strlen("/debug")) == 0) {
       pthread_mutex_lock(&debugModeMutex);
       if (debugMode) {
         debugMode = 0;
          wprintFormat(chatWin, time(NULL), "Alert", "Debug disabled", 10);
         //wprintw(chatWin, " --- Client: Debug disabled.\n");
       }
       else {
          debugMode = 1;
          wprintFormat(chatWin, time(NULL), "Alert", "Debug enabled", 10);
         //wprintw(chatWin, " --- Client: Debug enabled.\n");
       }
       pthread_mutex_unlock(&debugModeMutex);
       return 0;
   }
   // Handle connect command
   else if (strncmp((void *)tx_pkt->buf, "/connect", strlen("/connect")) == 0) {
      if (!newServerConnection((void *)tx_pkt->buf)) {
          wprintFormat(chatWin, time(NULL), "Error", "Server connect failed.", 8);
      }
      return 0;
   }
   // Handle reconnect command
   else if (strncmp((void *)tx_pkt->buf, "/reconnect", strlen("/reconnect")) == 0) {
      if (!reconnect((void *)tx_pkt->buf)) {
          wprintFormat(chatWin, time(NULL), "Error", "Server connect failed.", 8);
      }
      return 0;
   }
   // Handle autoconnect command
   else if (strncmp((void *)tx_pkt->buf, "/autoconnect", strlen("/autoconnect")) == 0) {
       if (toggleAutoConnect()) {
          wprintFormat(chatWin, time(NULL), "Alert", "Autoconnect enabled", 10);
          //wprintw(chatWin, " --- Client: Autoconnect enabled.\n");
       }
       else {
          wprintFormat(chatWin, time(NULL), "Alert", "Autoconnect disabled", 10);
          //wprintw(chatWin, " --- Client: Autoconnect disabled.\n");
       }
       return 0;
   }
   // Handle register command
   else if (strncmp((void *)tx_pkt->buf, "/register", strlen("/register")) == 0) {
      return (serverRegistration(tx_pkt));
   }
   // Handle login command
   else if (strncmp((void *)tx_pkt->buf, "/login", strlen("/login")) == 0) {
      return (serverLogin(tx_pkt));
   }
   // Handle setname command
   else if (strncmp((void *)tx_pkt->buf, "/setname", strlen("/setname")) == 0) {
      return setName(tx_pkt);
   }
   // Handle setpass command
   else if (strncmp((void *)tx_pkt->buf, "/setpass", strlen("/setpass")) == 0) {
      if (!setPassword(tx_pkt)) {
         //wprintw(chatWin, " --- Error: Password mismatch.\n");
          wprintFormat(chatWin, time(NULL), "Error", "Password mismath", 8);
         return 0;
      }
      else {
         return 1;
      }
   }
   // Handle motd command
   else if (strncmp((void *)tx_pkt->buf, "/motd", strlen("/motd")) == 0) {
       tx_pkt->options = GETMOTD;
       return 1;;
   }
   // Handle invite command
   else if (strncmp((void *)tx_pkt->buf, "/invite", strlen("/invite")) == 0) {
      return validInvite(tx_pkt);
   }
   // Handle join command
   else if (strncmp((void *)tx_pkt->buf, "/join", strlen("/join")) == 0) {
       return validJoin(tx_pkt);
   }
   // Handle leave command
   else if (strncmp((void *)tx_pkt->buf, "/leave", strlen("/leave")) == 0) {
       tx_pkt->options = LEAVE;
       pthread_mutex_lock(&roomMutex);
       memset(&tx_pkt->buf, 0, sizeof(tx_pkt->buf));
       sprintf(tx_pkt->buf, "/leave %d", currentRoom);
       pthread_mutex_unlock(&roomMutex);
       return 1;
   }
   // Handle who command
   else if (strncmp((void *)tx_pkt->buf, "/who", strlen("/who")) == 0) {
       if (strcmp((void *)tx_pkt->buf, "/who all") == 0) {
          tx_pkt->options = GETALLUSERS;
          return 1;
       }
       else if (strlen(tx_pkt->buf) > strlen("/who ")) {
          tx_pkt->options = GETUSER;
          return 1;
       }
       tx_pkt->options = GETUSERS;
       pthread_mutex_lock(&roomMutex);
       sprintf( tx_pkt->buf, "%s %d", tx_pkt->buf, currentRoom);
       pthread_mutex_unlock(&roomMutex);
       return 1;
   }
   // Handle rooms command
   else if (strncmp((void *)tx_pkt->buf, "/list", strlen("/list")) == 0) {
       tx_pkt->options = GETROOMS;
       return 1;
   }
   // If it wasn't any of that, invalid command
   else {
      wprintFormat(chatWin, time(NULL), "Error", "Invalid command", 8);
      //wprintw(chatWin, " --- Error: Invalid command.\n");
      return 0;
   }
}


/* Uses first word after /invite as username arg, appends currentroom */
int validInvite(packet *tx_pkt) {
   int i;
   char *args[16];
   char cpy[BUFFERSIZE];
   char *tmp = cpy;
   strcpy(tmp, tx_pkt->buf);

   args[i] = strsep(&tmp, " \t");
   while ((i < sizeof(args) - 1) && (args[i] != '\0')) {
       //wprintw(chatWin, "%d: %s\n", i, args[i]);
       args[++i] = strsep(&tmp, " \t");
   }

   if (i > 8) {
      tx_pkt->options = INVITE;
      memset(&tx_pkt->buf, 0, sizeof(tx_pkt->buf));
      pthread_mutex_lock(&roomMutex);
      sprintf(tx_pkt->buf, "%s %d", args[9], currentRoom);
      pthread_mutex_unlock(&roomMutex);
      return 1;
   }
   else {
      wprintFormat(chatWin, time(NULL), "Error", "Usage: /invite username", 8);
      //wprintw(chatWin, " --- Error: Usage: /invite username\n");
      return 0;
   }
}


/* Uses first word after /join as roomname arg, appends currentroom */
int validJoin(packet *tx_pkt) {
   int i;
   char *args[16];
   char cpy[BUFFERSIZE];
   char *tmp = cpy;
   strcpy(tmp, tx_pkt->buf);

   args[i] = strsep(&tmp, " \t");
   while ((i < sizeof(args) - 1) && (args[i] != '\0')) {
       args[++i] = strsep(&tmp, " \t");
   }

   if (i > 8) {
      tx_pkt->options = JOIN;
      memset(&tx_pkt->buf, 0, sizeof(tx_pkt->buf));
      pthread_mutex_lock(&roomMutex);
      sprintf( tx_pkt->buf, "%s %d", (char *)args[9], currentRoom);
      pthread_mutex_unlock(&roomMutex);
      return 1;
   }
   else {
      wprintFormat(chatWin, time(NULL), "Error", "Usage: /join roomname", 8);
      //wprintw(chatWin, " --- Error: Usage: /join roomname\n");
      return 0;
   }
}


/* Connect to a new server */
int newServerConnection(char *buf) {
   int i = 0;
   char *args[16];
   char cpy[BUFFERSIZE];
   char *tmp = cpy;
   strcpy(tmp, buf);
   FILE *configfp;
   char line[128];

   args[i] = strsep(&tmp, " \t");
   while ((i < sizeof(args) - 1) && (args[i] != '\0')) {
       args[++i] = strsep(&tmp, " \t");
   }
   if (i > 2) {
      if((serverfd = get_server_connection(args[1], args[2])) == -1) {
         wprintFormat(chatWin, time(NULL), "Error", "Could not connect to server", 8);
         return 0;
      }
      if(pthread_create(&chat_rx_thread, NULL, chatRX, (void *)&serverfd)) {
         wprintFormat(chatWin, time(NULL), "Error", "chatRX thread not created", 8);
         return 0;
      }
      wprintFormat(chatWin, time(NULL), "Client", "Connected.", 1);
      i = 0;
      pthread_mutex_lock(&configFileMutex);
      configfp = fopen(config_file, "r+");
      if (configfp != NULL) {
         while (!feof(configfp)) {
            if (fgets(line, sizeof(line), configfp)) {
               if (strncmp(line, "last connection:", strlen("last connection:")) == 0) {
                  fseek(configfp, -strlen(line), SEEK_CUR);
                  for (i = 0; i < strlen(line); i++) {
                     fputs(" ", configfp);
                  }
                  fseek(configfp, -strlen(line), SEEK_CUR);
                  fputs("last connection: ", configfp);
                  fputs(buf + strlen("/connect "), configfp);
               }
            }
         }
      }
      fclose(configfp);
      pthread_mutex_unlock(&configFileMutex);
      return 1;
   }
   else {
       wprintFormat(chatWin, time(NULL), "Error", "Usage: /connect address port", 8);
       //wprintw(chatWin, " --- Error: Usage: /connect address port\n");
       return 0;
   }
}


/* Reconnect using the last connection settings */
int reconnect(char *buf) {
   FILE *configfp;
   char line[128];

   pthread_mutex_lock(&configFileMutex);
   configfp = fopen(config_file, "r");
   if (configfp != NULL) {
      while (!feof(configfp)) {
         if (fgets(line, sizeof(line), configfp)) {
            if (strncmp(line, "last connection:", strlen("last connection:")) == 0) {
               if (strlen(line) > (strlen("last connection: "))) {
                  strcpy(buf, "/connect ");
                  strcat(buf, line + strlen("last connection: "));
                  fclose(configfp);
                  pthread_mutex_unlock(&configFileMutex);
                  return newServerConnection(buf);
               }
               else {
                  fclose(configfp);
                  pthread_mutex_unlock(&configFileMutex);
                  //wprintw(chatWin, " --- Error: No previous connection to reconnect to.\n");
                  wprintFormat(chatWin, time(NULL), "Error", "No previous connect to reconnect to", 8);
                  box(chatWin, 0, 0);
                  wrefresh(chatWin);
                  return 0;
               }
            }
         }
      }
   }
   fclose(configfp);
   pthread_mutex_unlock(&configFileMutex);
   return 0;
}


/* Toggle autoconnect state in config file */
int toggleAutoConnect() {
   FILE *configfp;
   char line[128];
   int ret;
 
   pthread_mutex_lock(&configFileMutex);
   configfp = fopen(config_file, "r+");
   if (configfp != NULL) {
      while (!feof(configfp)) {
         if (fgets(line, sizeof(line), configfp)) {
            if (strncmp(line, "auto-reconnect:", strlen("auto-reconnect:")) == 0) {
               fseek(configfp, -2, SEEK_CUR);
               if (strncmp(line + strlen("auto-reconnect: "), "0", 1) == 0) {
                  fputs("1", configfp);
                  ret = 1;
               }
               else {
                  fputs("0", configfp);
                  ret = 0;
               }
            }
         }
      }
   }
   fclose(configfp);
   pthread_mutex_unlock(&configFileMutex);
   return ret;
}


/* Connect to a new server */
int serverLogin(packet *tx_pkt) {
   char *args[16];
   char cpy[64];
   int i;
   strcpy(cpy, tx_pkt->buf);
   char *tmp = cpy;
   args[i] = strsep(&tmp, " \t");
   while ((i < sizeof(args) - 1) && (args[i] != '\0')) {
      args[++i] = strsep(&tmp, " \t");
   }
   if (i > 9) {
      tx_pkt->options = LOGIN;
      strcpy(tx_pkt->username, args[9]);
      strcpy(tx_pkt->realname, args[9]);
      return 1;
   }
   else {
      wprintFormat(chatWin, time(NULL), "Error", "Usage: /login username password", 8);
      //wprintw(chatWin, " --- Error: Usage: /login username password\n");
      return 0;
   }
}


/* Handle registration for server  */
int serverRegistration(packet *tx_pkt) {
   int i = 0;
   char *args[16];
   char cpy[128];
   char *tmp = cpy;
   strcpy(tmp, tx_pkt->buf);
   
   // Split command args
   args[i] = strsep(&tmp, " \t");
   while ((i < sizeof(args) - 1) && (args[i] != '\0')) {
      args[++i] = strsep(&tmp, " \t");
   }
   if (i == 4) {
      // if the passwords patch mark options
      if (strcmp(args[2], args[3]) == 0) {
         tx_pkt->options = REGISTER;
         strcpy(tx_pkt->username, args[1]);
         strcpy(tx_pkt->realname, args[1]);
         return 1;
      }
      else {
         wprintFormat(chatWin, time(NULL), "Error", "Password mismatch", 8);
         //wprintw(chatWin, " --- Error: Password mismatch\n");
         return 0;
      }
   }
   else {
      wprintFormat(chatWin, time(NULL), "Error", "Usage: /register username password password", 8);
      //wprintw(chatWin, " --- Error: Usage: /register username password password\n");
      return 0;
   }
}


/* Set user password */
int setPassword(packet *tx_pkt) {
   int i = 0;
   char *args[16];
   char cpy[128];
   char *tmp = cpy;
   strcpy(tmp, tx_pkt->buf);
   
   // Split command args
   args[i] = strsep(&tmp, " \t");
   while ((i < sizeof(args) - 1) && (args[i] != '\0')) {
      args[++i] = strsep(&tmp, " \t");
   }
   if (i == 4) {
      if (strcmp(args[2], args[3])  == 0) {
         tx_pkt->options = SETPASS;
         return 1;
      }
      else {
      wprintFormat(chatWin, time(NULL), "Error", "New password mismatch", 8);
      //wprintw(chatWin, " --- Error: New password mismatch\n");
      return 0;
      }
   }
   else {
      wprintFormat(chatWin, time(NULL), "Error", "Usage: /setpass oldpasswprd newpassword newpassword", 8);
      //wprintw(chatWin, " --- Error: Usage: /setpass oldpassword newpassword newpassword\n");
      return 0;
   }
}


/* Set user real name */
int setName(packet *tx_pkt) {
   if(strlen(tx_pkt->buf) > strlen("/setname ")) {
      memmove(tx_pkt->buf, tx_pkt->buf + strlen("/setname "), sizeof(tx_pkt->buf));
      tx_pkt->options = SETNAME;
      return 1;
   }
   else {
      //wprintw(chatWin, " --- Error: Usage: /setname newname\n");
      wprintFormat(chatWin, time(NULL), "Error", "Usage: /setname newname", 8);
      return 0;
   }
}


/* Dump contents of received packet from server */
void debugPacket(packet *rx_pkt) {

   wprintSeperatorTitle(chatWin, "PACKET REPORT", 6, 3 );

   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, A_BOLD);
   wprintw(chatWin, "\tTimestamp: ");
   wattroff(chatWin, A_BOLD);
   wprintw(chatWin, "%lu\n", rx_pkt->timestamp);
   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, A_BOLD);
   wprintw(chatWin, "\tUser Name: ");
   wattroff(chatWin, A_BOLD);
   wprintw(chatWin, "%s\n", rx_pkt->username);
   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, A_BOLD);
   wprintw(chatWin, "\tReal Name: ");
   wattroff(chatWin, A_BOLD);
   wprintw(chatWin, "%s\n", rx_pkt->realname);
   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, A_BOLD);
   wprintw(chatWin, "\tOption: ");
   wattroff(chatWin, A_BOLD);
   wprintw(chatWin, "%d\n", rx_pkt->options);
   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, A_BOLD);
   wprintw(chatWin, "\tBuffer: ");
   wattroff(chatWin, A_BOLD);
   wprintw(chatWin, "%s\n", rx_pkt->buf);

   wprintSeperator(chatWin, 6);
}


/* Print helpful and unhelpful things */
void showHelp(char *buf) {
   int bar = 7;
   int command = 3;
   int title = 2;
   int i = 0;
   char *args[16];
   char cpy[128];
   char *tmp = cpy;
   strcpy(tmp, buf);
   char *cmd = "all";

   // Split command args
   args[i] = strsep(&tmp, " \t");
   while ((i < sizeof(args) - 1) && (args[i] != '\0')) {
      args[++i] = strsep(&tmp, " \t");
   }

   if (args[1] != '\0') { cmd = args[1]; }

   wprintSeperatorTitle(chatWin, "Command List", bar, command);

   // connect
   if (strcmp(cmd, "connect")  == 0 || strcmp(cmd, "all") == 0) {
      wprintFormatTime(chatWin, time(NULL));
      wattron(chatWin, COLOR_PAIR(command));
      wprintw(chatWin, "   /connect     ");
      wattroff(chatWin, COLOR_PAIR(command));
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " \n");
      wattroff(chatWin, COLOR_PAIR(bar));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "        ");         
      waddch(chatWin, ACS_LLCORNER);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_TTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Usage: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "/connect address port\n");
      wattroff(chatWin, COLOR_PAIR(1));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LLCORNER);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Desc: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "Connect to a chat server\n");
      wattroff(chatWin, COLOR_PAIR(1));
   }

   // reconnect
   if (strcmp(cmd, "reconnect")  == 0 || strcmp(cmd, "all") == 0) {
      wprintFormatTime(chatWin, time(NULL));
      wattron(chatWin, COLOR_PAIR(command));
      wprintw(chatWin, "   /reconnect   ");
      wattroff(chatWin, COLOR_PAIR(command));
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " \n");
      wattroff(chatWin, COLOR_PAIR(bar));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "        ");
      waddch(chatWin, ACS_LLCORNER);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_TTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Usage: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "/reconnect\n");
      wattroff(chatWin, COLOR_PAIR(1));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LLCORNER);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Desc: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "Connect using last connect arguments (surprisingly correct spelling)\n");
      wattroff(chatWin, COLOR_PAIR(1));
   }

   // autoconnect
   if (strcmp(cmd, "autoconnect")  == 0 || strcmp(cmd, "all") == 0) {
      wprintFormatTime(chatWin, time(NULL));
      wattron(chatWin, COLOR_PAIR(command));
      wprintw(chatWin, "   /autoconnect ");
      wattroff(chatWin, COLOR_PAIR(command));
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " \n");
      wattroff(chatWin, COLOR_PAIR(bar));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "        ");
      waddch(chatWin, ACS_LLCORNER);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_TTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Usage: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "/autoconnect\n");
      wattroff(chatWin, COLOR_PAIR(1));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LLCORNER);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Desc: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "Toggle autoconnect state on chat startup\n");
      wattroff(chatWin, COLOR_PAIR(1));
   }

   // help
   if (strcmp(cmd, "help")  == 0 || strcmp(cmd, "all") == 0) {
      wprintFormatTime(chatWin, time(NULL));
      wattron(chatWin, COLOR_PAIR(command));
      wprintw(chatWin, "   /help        ");
      wattroff(chatWin, COLOR_PAIR(command));
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " \n");
      wattroff(chatWin, COLOR_PAIR(bar));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "        ");
      waddch(chatWin, ACS_LLCORNER);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_TTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Usage: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "/help\n");
      wattroff(chatWin, COLOR_PAIR(1));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Desc: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "Display a usage and description list of all commands\n");
      wattroff(chatWin, COLOR_PAIR(1));

      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Usage: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "/help command_name\n");
      wattroff(chatWin, COLOR_PAIR(1));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LLCORNER);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Desc: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "Display a usage and description for a single command\n");
      wattroff(chatWin, COLOR_PAIR(1));

   }

   // exit
   if (strcmp(cmd, "exit")  == 0 || strcmp(cmd, "all") == 0) {
      wprintFormatTime(chatWin, time(NULL));
      wattron(chatWin, COLOR_PAIR(command));
      wprintw(chatWin, "   /exit        ");
      wattroff(chatWin, COLOR_PAIR(command));
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " \n");
      wattroff(chatWin, COLOR_PAIR(bar));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "        ");
      waddch(chatWin, ACS_LLCORNER);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_TTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Usage: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "/exit\n");
      wattroff(chatWin, COLOR_PAIR(1));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LLCORNER);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Desc: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "Exit the client\n");
      wattroff(chatWin, COLOR_PAIR(1));
   }

   // debug
   if (strcmp(cmd, "debug")  == 0 || strcmp(cmd, "all") == 0) {
      wprintFormatTime(chatWin, time(NULL));
      wattron(chatWin, COLOR_PAIR(command));
      wprintw(chatWin, "   /debug       ");
      wattroff(chatWin, COLOR_PAIR(command));
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " \n");
      wattroff(chatWin, COLOR_PAIR(bar));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "        ");
      waddch(chatWin, ACS_LLCORNER);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_TTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Usage: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "/debug\n");
      wattroff(chatWin, COLOR_PAIR(1));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LLCORNER);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Desc: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "Toggle printing of received packet contents\n");
      wattroff(chatWin, COLOR_PAIR(1));
   }

   // register
   if (strcmp(cmd, "register")  == 0 || strcmp(cmd, "all") == 0) {
      wprintFormatTime(chatWin, time(NULL));
      wattron(chatWin, COLOR_PAIR(command));
      wprintw(chatWin, "   /register    ");
      wattroff(chatWin, COLOR_PAIR(command));
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " \n");
      wattroff(chatWin, COLOR_PAIR(bar));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "        ");
      waddch(chatWin, ACS_LLCORNER);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_TTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Usage: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "/register username password password\n");
      wattroff(chatWin, COLOR_PAIR(1));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LLCORNER);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Desc: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "Register a new user account with the connected server\n");
      wattroff(chatWin, COLOR_PAIR(1));
   }

   // login
   if (strcmp(cmd, "login")  == 0 || strcmp(cmd, "all") == 0) {
      wprintFormatTime(chatWin, time(NULL));
      wattron(chatWin, COLOR_PAIR(command));
      wprintw(chatWin, "   /login       ");
      wattroff(chatWin, COLOR_PAIR(command));
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " \n");
      wattroff(chatWin, COLOR_PAIR(bar));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "        ");
      waddch(chatWin, ACS_LLCORNER);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_TTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Usage: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "/login username password\n");
      wattroff(chatWin, COLOR_PAIR(1));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LLCORNER);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Desc: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "Log in to chat with the connected sever\n");
      wattroff(chatWin, COLOR_PAIR(1));
   }

   // setpass
   if (strcmp(cmd, "setpass")  == 0 || strcmp(cmd, "all") == 0) {
      wprintFormatTime(chatWin, time(NULL));
      wattron(chatWin, COLOR_PAIR(command));
      wprintw(chatWin, "   /setpass     ");
      wattroff(chatWin, COLOR_PAIR(command));
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " \n");
      wattroff(chatWin, COLOR_PAIR(bar));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "        ");
      waddch(chatWin, ACS_LLCORNER);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_TTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Usage: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "/setpass oldpass newpass newpass\n");
      wattroff(chatWin, COLOR_PAIR(1));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LLCORNER);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Desc: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "Change the password for the user currently logged in\n");
      wattroff(chatWin, COLOR_PAIR(1));
   }

   // setname
   if (strcmp(cmd, "setname")  == 0 || strcmp(cmd, "all") == 0) {
      wprintFormatTime(chatWin, time(NULL));
      wattron(chatWin, COLOR_PAIR(command));
      wprintw(chatWin, "   /setname     ");
      wattroff(chatWin, COLOR_PAIR(command));
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " \n");
      wattroff(chatWin, COLOR_PAIR(bar));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "        ");
      waddch(chatWin, ACS_LLCORNER);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_TTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Usage: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "/setname new display name\n");
      wattroff(chatWin, COLOR_PAIR(1));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LLCORNER);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Desc: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "Request a new display name to be show with all messages\n");
      wattroff(chatWin, COLOR_PAIR(1));
   }

   // who
   if (strcmp(cmd, "who")  == 0 || strcmp(cmd, "all") == 0) {
      wprintFormatTime(chatWin, time(NULL));
      wattron(chatWin, COLOR_PAIR(command));
      wprintw(chatWin, "   /who         ");
      wattroff(chatWin, COLOR_PAIR(command));
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " \n");
      wattroff(chatWin, COLOR_PAIR(bar));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "        ");
      waddch(chatWin, ACS_LLCORNER);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_TTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Usage: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "/who\n");
      wattroff(chatWin, COLOR_PAIR(1));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Desc: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "Display a list of users in your current room\n");
      wattroff(chatWin, COLOR_PAIR(1));
 
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Usage: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "/who all\n");
      wattroff(chatWin, COLOR_PAIR(1));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Desc: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "Display a list of all connected users\n");
      wattroff(chatWin, COLOR_PAIR(1));

      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Usage: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "/who username\n");
      wattroff(chatWin, COLOR_PAIR(1));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LLCORNER);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Desc: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "Display users real name.\n");
      wattroff(chatWin, COLOR_PAIR(1));
   }

   // list
   if (strcmp(cmd, "list")  == 0 || strcmp(cmd, "all") == 0) {
      wprintFormatTime(chatWin, time(NULL));
      wattron(chatWin, COLOR_PAIR(command));
      wprintw(chatWin, "   /list        ");
      wattroff(chatWin, COLOR_PAIR(command));
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " \n");
      wattroff(chatWin, COLOR_PAIR(bar));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "        ");
      waddch(chatWin, ACS_LLCORNER);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_TTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Usage: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "/list\n");
      wattroff(chatWin, COLOR_PAIR(1));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LLCORNER);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Desc: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "Return a list of all public room\n");
      wattroff(chatWin, COLOR_PAIR(1));
   }

   // invite
   if (strcmp(cmd, "invite")  == 0 || strcmp(cmd, "all") == 0) {
      wprintFormatTime(chatWin, time(NULL));
      wattron(chatWin, COLOR_PAIR(command));
      wprintw(chatWin, "   /invite      ");
      wattroff(chatWin, COLOR_PAIR(command));
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " \n");
      wattroff(chatWin, COLOR_PAIR(bar));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "        ");
      waddch(chatWin, ACS_LLCORNER);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_TTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Usage: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "/invite username\n");
      wattroff(chatWin, COLOR_PAIR(1));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LLCORNER);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Desc: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "Invite a user to your current room\n");
      wattroff(chatWin, COLOR_PAIR(1));
   }

   // join
   if (strcmp(cmd, "join")  == 0 || strcmp(cmd, "all") == 0) {
      wprintFormatTime(chatWin, time(NULL));
      wattron(chatWin, COLOR_PAIR(command));
      wprintw(chatWin, "   /join        ");
      wattroff(chatWin, COLOR_PAIR(command));
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " \n");
      wattroff(chatWin, COLOR_PAIR(bar));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "        ");
      waddch(chatWin, ACS_LLCORNER);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_TTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Usage: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "/join roomname\n");
      wattroff(chatWin, COLOR_PAIR(1));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LLCORNER);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Desc: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "Join a room or create a new one\n");
      wattroff(chatWin, COLOR_PAIR(1));
   }

   // leave
   if (strcmp(cmd, "leave")  == 0 || strcmp(cmd, "all") == 0) {
      wprintFormatTime(chatWin, time(NULL));
      wattron(chatWin, COLOR_PAIR(command));
      wprintw(chatWin, "   /leave       ");
      wattroff(chatWin, COLOR_PAIR(command));
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " \n");
      wattroff(chatWin, COLOR_PAIR(bar));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "        ");
      waddch(chatWin, ACS_LLCORNER);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_HLINE);
      waddch(chatWin, ACS_TTEE);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Usage: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "/leave\n");
      wattroff(chatWin, COLOR_PAIR(1));
      wprintFormatTime(chatWin, time(NULL));
      wprintw(chatWin, "               ");
      waddch(chatWin, ACS_LLCORNER);
      wattron(chatWin, COLOR_PAIR(bar));
      wprintw(chatWin, " ");
      waddch(chatWin, ACS_VLINE);
      wprintw(chatWin, " ");
      wattroff(chatWin, COLOR_PAIR(bar));
      wattron(chatWin, COLOR_PAIR(title));
      wprintw(chatWin, "Desc: ");
      wattroff(chatWin, COLOR_PAIR(title));
      wattron(chatWin, COLOR_PAIR(1));
      wprintw(chatWin, "Leave the room you are in and return to the lobby\n");
      wattroff(chatWin, COLOR_PAIR(1));
   }

   wprintSeperator(chatWin, bar);
}
