#include "chat_client.h"

/* Declared, defined and allocated in chat_client.c */
extern int serverfd;
extern volatile int currentRoom;
extern volatile int debugMode;
extern char username[64];
extern pthread_t chat_rx_thread;
extern pthread_mutex_t roomMutex;
extern pthread_mutex_t unameMutex;
extern pthread_mutex_t debugModeMutex;

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
   // Handle reconnect command
   else if (strncmp((void *)tx_pkt->buf, "/reconnect", strlen("/reconnect")) == 0) {
      if (!reconnect(tx_pkt)) {
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


/* Connect to a new server */
int newServerConnection(char *buf) {
   int i = 0;
   char *args[16];
   char cpy[128];
   char *tmp = cpy;
   strcpy(tmp, buf);
   int fd;
   
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
      fd = open("chat_client.ini", O_WRONLY | O_CREAT, O_TRUNC, S_IRWXU);
      write(fd, buf, 128);
      close(fd);
      return 1;
   }
   else {
       printf("%s --- Error:%s Usage: /connect address port\n", RED, NORMAL);
       return 0;
   }
}


/* Reconnect using the last connection settings */
int reconnect(packet *tx_pkt) {
   if(strcmp(tx_pkt->buf, "/reconnect") == 0) {
      int fd = open("chat_client.ini", O_RDONLY);
      if(fd > 0) {
         read(fd, tx_pkt->buf, sizeof(tx_pkt->buf));
         printf("Reconnecting");
         close(fd);
         return newServerConnection((void *)tx_pkt->buf);
      }
      else
      {
         printf("%s --- Error:%s Server connect failed; no previous connection\n", RED, NORMAL);
         close(fd);
         return 0;
      }
   }
   else {
      printf("%s --- Error:%s Usage: /reconnect\n", RED, NORMAL);
      return 0;
   }
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
      pthread_mutex_lock(&unameMutex);
      strcpy(username, args[1]);
      pthread_mutex_unlock(&unameMutex);
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
         strcpy(tx_pkt->alias, args[1]);
         pthread_mutex_lock(&unameMutex);
         strcpy(username, args[1]);
         pthread_mutex_unlock(&unameMutex);
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
   if (1 == 4) {
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
      pthread_mutex_lock(&unameMutex);
      memset(&username, 0, sizeof(username));
      strncpy(username, tx_pkt->buf + strlen("/setname "), strlen(tx_pkt->buf) - strlen("/setname "));
      pthread_mutex_unlock(&unameMutex);
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
   printf("%s Alias: %s%s\n", MAGENTA, NORMAL, rx_pkt->alias);
   printf("%s Option: %s%d\n", MAGENTA, NORMAL, rx_pkt->options);
   printf("%s Buffer: %s%s\n", MAGENTA, NORMAL, rx_pkt->buf);
   printf("%s --------------------------------------------------------- %s\n", CYAN, NORMAL);
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
   printf("%s\t/setpass%s\t | Usage: /setpass oldpassword newpassword newpassword.\n", YELLOW, NORMAL);
   printf("%s\t/setname%s\t | Usage: /setname fname lname.\n", YELLOW, NORMAL);
   printf("%s\t/connect%s\t | Usage: /connect address port.\n", YELLOW, NORMAL);
   printf("%s\t/debug%s\t\t | Toggle debug mode.\n", YELLOW, NORMAL);
}

