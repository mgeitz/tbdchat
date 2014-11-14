#include "chat_client.h"

int serverLogin(packet *tx_pkt);
int serverRegistration(packet *tx_pkt);
int setPassword(packet *tx_pkt);
void setName(packet *tx_pkt);
void serverResponse(packet *rx_pkt);
void debugPacket(packet *rx_pkt);
void showHelp();

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
void setName(packet *tx_pkt) {
   pthread_mutex_lock(&unameMutex);
   memset(&username, 0, sizeof(username));
   strncpy(username, tx_pkt->buf + strlen("/setname "), strlen(tx_pkt->buf) - strlen("/setname "));
   pthread_mutex_unlock(&unameMutex);
   tx_pkt->options = SETNAME;
}



/* Handle non message packets from server */
void serverResponse(packet *rx_pkt) {
   if (rx_pkt->options == REGFAIL) {
      printf("%s --- Error:%s Registration failed.\n", RED, NORMAL);
   }
   else if (rx_pkt->options == LOGFAIL) {
      printf("%s --- Error:%s Login failed.\n", RED, NORMAL);
   }
   else if (rx_pkt->options == LOGSUC) {
      pthread_mutex_lock(&unameMutex);
      strcpy(username, rx_pkt->buf);
      pthread_mutex_unlock(&unameMutex);
      pthread_mutex_lock(&roomMutex);
      // Hardcoded lobby room
      currentRoom = 1000;
      pthread_mutex_unlock(&roomMutex);
      printf("%s --- Success:%s Login successful!\n", GREEN, NORMAL);
   }
   else if (rx_pkt->options == REGSUC) {
      pthread_mutex_lock(&roomMutex);
      // Hardcoded lobby room
      currentRoom = 1000;
      pthread_mutex_unlock(&roomMutex);
      printf("%s --- Success:%s Registration successful!\n", GREEN, NORMAL);
   }
   else if(rx_pkt->options == GETUSERS) {
      printf("%s\n", rx_pkt->buf);
   }

   else if(rx_pkt->options == PASSFAIL) {
      printf("%s --- Error:%s Password change failed.\n", RED, NORMAL);
   }
   else if(rx_pkt->options == PASSSUC) {
      printf("%s --- Succes:%s Password change successful!\n", GREEN, NORMAL);
   }
   else {
      printf("%s --- Error:%s Unknown message received from server.\n", RED, NORMAL);
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

