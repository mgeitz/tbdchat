#include "chat_client.h"

/* Declared.  Defined and allocated in chat_client.c */
extern int serverfd;
extern volatile int currentRoom;
extern volatile int debugMode;
extern char username[64];
extern char realname[64];
extern char *config_file;

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
       showHelp();
       return 0;
   }
   // Handle debug command
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
   // Handle reconnect command
   else if (strncmp((void *)tx_pkt->buf, "/reconnect", strlen("/reconnect")) == 0) {
      if (!reconnect((void *)tx_pkt->buf)) {
          printf("%s --- Error:%s Server connect failed.\n", RED, NORMAL);
      }
      return 0;
   }
   // Handle autoconnect command
   else if (strncmp((void *)tx_pkt->buf, "/autoconnect", strlen("/autoconnect")) == 0) {
       if (toggleAutoConnect()) {
          printf("%sAutoconnect enabled.%s\n", WHITE, NORMAL);
       }
       else {
          printf("%sAutoconnect disabled.%s\n", WHITE, NORMAL);
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
   // Handle motd command
   if (strncmp((void *)tx_pkt->buf, "/motd", strlen("/motd")) == 0) {
       tx_pkt->options = GETMOTD;
       return 1;;
   }
   // Handle invite command
   if (strncmp((void *)tx_pkt->buf, "/invite", strlen("/invite")) == 0) {
       tx_pkt->options = INVITE;
       return 1;;
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
       if (strncmp((void *)tx_pkt->buf, "/who all", strlen("/who all")) == 0) {
          tx_pkt->options = GETALLUSERS;
          return 1;
       }
       else if (strlen(tx_pkt->buf) > strlen("/who ")) {
          tx_pkt->options = GETUSER;
          return 1;
       }
       tx_pkt->options = GETUSERS;
       pthread_mutex_lock(&roomMutex);
       sprintf(tx_pkt->buf, "%s %d", tx_pkt->buf, currentRoom);
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
      printf("%s --- Error:%s Invalid command.\n", RED, NORMAL);
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

   if (i > 1) {
      tx_pkt->options = JOIN;
      pthread_mutex_lock(&roomMutex);
      memset(&tx_pkt->buf, 0, sizeof(tx_pkt->buf));
      sprintf(tx_pkt->buf, "%s %d", args[1], currentRoom);
      pthread_mutex_unlock(&roomMutex);
      return 1;
   }
   else {
      printf("%s --- Error:%s Usage: /join roomname.\n", RED, NORMAL);
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
         printf("%s --- Error:%s Could not connect to server.\n", RED, NORMAL);
         return 0;
      }
      if(pthread_create(&chat_rx_thread, NULL, chatRX, (void *)&serverfd)) {
         printf("%s --- Error: %s chatRX thread not created.\n", RED, NORMAL);
         return 0;
      }
      printf("Connected.\n");
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
       printf("%s --- Error:%s Usage: /connect address port\n", RED, NORMAL);
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
                  printf("%s --- Error:%s No previous connection to reconnect to.\n", RED, NORMAL);
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
   if (i == 3) {
      pthread_mutex_lock(&nameMutex);
      strcpy(username, args[1]);
      strcpy(realname, args[1]);
      pthread_mutex_unlock(&nameMutex);
      tx_pkt->options = LOGIN;
      return 1;
   }
   else {
      printf("%s --- Error:%s Usage: /login username password\n", RED, NORMAL);
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
         pthread_mutex_lock(&nameMutex);
         strcpy(username, args[1]);
         strcpy(tx_pkt->username, username);
         strcpy(realname, args[1]);
         strcpy(tx_pkt->realname, realname);
         pthread_mutex_unlock(&nameMutex);
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
      printf("%s --- Error:%s New password mismatch\n", RED, NORMAL);
      return 0;
      }
   }
   else {
      printf("%s --- Error:%s Usage: /setpass oldpassword newpassword newpassword\n", RED, NORMAL);
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
      printf("%s --- Error:%s Usage: /setname newname\n", RED, NORMAL);
      return 0;
   }
}


/* Dump contents of received packet from server */
void debugPacket(packet *rx_pkt) {
   printf("%s --------------------- PACKET REPORT --------------------- %s\n", CYAN, NORMAL);
   printf("%s Timestamp: %s%lu\n", MAGENTA, NORMAL, rx_pkt->timestamp);
   printf("%s User Name: %s%s\n", MAGENTA, NORMAL, rx_pkt->username);
   printf("%s Real Name: %s%s\n", MAGENTA, NORMAL, rx_pkt->realname);
   printf("%s Option: %s%d\n", MAGENTA, NORMAL, rx_pkt->options);
   printf("%s Buffer: %s%s\n", MAGENTA, NORMAL, rx_pkt->buf);
   printf("%s --------------------------------------------------------- %s\n", CYAN, NORMAL);
}


/* Print helpful and unhelpful things */
void showHelp() {
   printf("%s\t/connect%s\t | Usage: /connect address port\n", YELLOW, NORMAL);
   printf("%s\t/reconnect%s\t | Connect to last known host\n", YELLOW, NORMAL);
   printf("%s\t/autoconnect%s\t | Toggle automatic connection to last known host on startup\n", YELLOW, NORMAL);
   printf("%s\t/help%s\t\t | Display a list of commands\n", YELLOW, NORMAL);
   printf("%s\t/debug%s\t\t | Toggle debug mode\n", YELLOW, NORMAL);
   printf("%s\t/exit%s\t\t | Exit the client\n", YELLOW, NORMAL);
   printf("%s\t/register%s\t | Usage: /register username password password\n", YELLOW, NORMAL);
   printf("%s\t/login%s\t\t | Usage: /login username password\n", YELLOW, NORMAL);
   printf("%s\t/setpass%s\t | Usage: /setpass oldpassword newpassword newpassword\n", YELLOW, NORMAL);
   printf("%s\t/setname%s\t | Usage: /setname fname lname\n", YELLOW, NORMAL);
   printf("%s\t/who%s\t\t | Return a list of users in your current room or a specific user\n", YELLOW, NORMAL);
   printf("%s\t/who all%s\t | Return a list of all connected users\n", YELLOW, NORMAL);
   printf("%s\t/list%s\t\t | Return a list of all public rooms with active users in them\n", YELLOW, NORMAL);
   printf("%s\t/invite%s\t\t | Usage: /invite username\n", YELLOW, NORMAL);
   printf("%s\t/join%s\t\t | Usage: /join roomname\n", YELLOW, NORMAL);
   printf("%s\t/leave%s\t\t | Leave the room you are in\n", YELLOW, NORMAL);
}
