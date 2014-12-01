#ifndef CHAT_CLIENT_H
#define CHAT_CLIENT_H


/* System Header Files */
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
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <ncurses.h>


/* Preprocessor Macros */

// Client buffer size
#define BUFFERSIZE 128
#define DEFAULT_ROOM 1000
#define SERVER_NAME "SERVER"
#define VERSION "0.3.3" // Completely made up version progression, change at will
#define CONFIG_FILENAME "/.tbdchat"

// Client options
#define INVALID -1
#define REGISTER 1
#define SETPASS 2 
#define SETNAME 3
#define LOGIN 4
#define EXIT 5
#define INVITE 6
#define JOIN 7
#define GETUSERS 8
#define GETALLUSERS 9
#define GETUSER 10
#define LEAVE 11
#define GETMOTD 12
#define GETROOMS 13

// Server responses
#define LOGSUC 100
#define REGSUC 101
#define PASSSUC 102
#define NAMESUC 103
#define JOINSUC 104
#define MOTD 105
#define INVITESUC 106
#define SERV_ERR 107

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


/* Structures */

// Packet
struct Packet {
   time_t timestamp;
   char buf[BUFFERSIZE];
   char username[64];
   char realname[64];
   int options;
};
typedef struct Packet packet;


/* Function Prototypes */

// chat_client.c
void sigintHandler(int sig_num);
void print_ip( struct addrinfo *ai);
int get_server_connection(char *hostname, char *port);
void *chatRX(void *ptr);
int userInput(packet *tx_pkt);
void buildDefaultConfig();
int auto_connect();
void newRoom(packet *rx_pkt);
int hash(char *str, int mod);

// client_commands.c
int userCommand(packet *tx_pkt);
int newServerConnection(char *buf);
int reconnect(char *buf);
int serverLogin(packet *tx_pkt);
int serverRegistration(packet *tx_pkt);
int setPassword(packet *tx_pkt);
int setName(packet *tx_pkt);
void serverResponse(packet *rx_pkt);
void debugPacket(packet *rx_pkt);
int toggleAutoConnect();
int validJoin(packet *tx_pkt);
int validInvite(packet *tx_pkt);
void showHelp(char *buf);

// visual.c
void initializeCurses();
void drawChatWin();
void drawInputWin();
void drawInfoLines();
void asciiSplash();
void colors();
void resizeHandler(int sig);
void wprintFormatTime(WINDOW *win, time_t ts);
void wprintFormat(WINDOW *win, time_t ts, char *from, char *buf, int from_color);
void wprintFormatMessage(WINDOW *win, time_t ts, char *from, char *buf, int from_color);
void wprintFormatError(WINDOW *win, time_t ts, char *buf);
void wprintFormatNotice(WINDOW *win, time_t ts, char *buf);
void wprintSeperatorTitle(WINDOW *win, char *title, int color, int title_color);
void wprintSeperator(WINDOW *win, int color);


#endif
