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
#define BACKLOG 2               // how many pending connections the queue will hold
#define BUFFERSIZE 128

struct Packet
{
   time_t timestamp;
   char buf[BUFFERSIZE];
   int options;
};
typedef struct Packet packet;

struct chatSession
{
   int clientA_fd;
   int clientB_fd;
   char aliasA[32];
   char aliasB[32];
   int running;
};
typedef struct chatSession session;

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

int get_server_socket(char *hostname, char *port);
int start_server(int serv_socket, int backlog);
int accept_client(int serv_sock);
void *clientA_thread(void *ptr);
void *clientB_thread(void *ptr);
void *subserver(void *ptr);
void end(session *ptr);
void start_subserver(int A_fd, int B_fd, char* clientA_usrID, char* clientB_usrID);
void sigintHandler(int sig_num);
void establish_identity(int fd, char *ID, char *name, User **user_list);
