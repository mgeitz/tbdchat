#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

/* System Header Files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>
#include <netdb.h>
#include <pthread.h>
#include "linked_list.h"

/* Preprocessor Macros */
// Misc constants
#define BACKLOG 2               // how many pending connections the queue will hold
#define BUFFERSIZE 128
#define DEFAULT_ROOM 1000
// Client options
#define INVALID -1
#define CONNECT 0
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
// Server responses
#define RECFAIL 100
#define REGFAIL 101
#define LOGFAIL 102
#define LOGSUC 103
#define REGSUC 104
#define PASSSUC 105
#define PASSFAIL 106
#define NAMESUC 107
#define NAMEFAIL 108
#define JOINSUC 109
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
struct Packet
{
   time_t timestamp;
   char buf[BUFFERSIZE];
   char username[64];
   char realname[64];
   int options;
};
typedef struct Packet packet;

struct chatSession
{
   char aliases[2][32];
   int clients[2];
   int this_client;
   int running;
   //mutex
};
typedef struct chatSession session;

/* Function Prototypes */
int get_server_socket(char *hostname, char *port);
int start_server(int serv_socket, int backlog);
int accept_client(int serv_sock);
void *subserver(void *ptr);
void end(session *ptr);
void start_subserver(int A_fd, int B_fd, char* clientA_usrID, char* clientB_usrID);
void sigintHandler(int sig_num);
void establish_identity(int fd, char *ID, char *name, User **user_list);
void *client_receive(void *ptr);
void register_user(packet *pkt, int fd);
void login(packet *pkt, int fd);
void invite(packet *pkt);
void exit_client(int fd);
void send_message(packet *pkt, int clientfd);
void get_active_users(int fd);
void set_pass(packet *pkt, int fd);
void set_name(packet *pkt, int fd);
void join(packet *pkt, int fd);
void debugPacket(packet *rx_pkt);

#endif
