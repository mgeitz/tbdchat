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

#define BUFFERSIZE 128
#define CONNECT 0
#define REGISTER 1
#define SETPASS 2 
#define SETNAME 3
#define LOGIN 4
#define EXIT 5
#define INVITE 6
#define JOIN 7
#define GETUSERS 8
#define RECFAIL 100

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

// Structure to be sent to server containing the send time, username, and message
struct Packet {
   time_t timestamp;
   char buf[BUFFERSIZE];
   char alias[64];
   int options;
};
// Declare structure type
typedef struct Packet packet;

// Function prototypes
void sigintHandler(int sig_num);
void print_ip( struct addrinfo *ai);
int get_server_connection(char *hostname, char *port);
void *chatRX(void *ptr);
int userInput(char *buf);
void userCommand(packet *tx_pkt);
int serverRegistration(packet *tx_pkt);
int newServerConnection(char *buf);
void showHelp();
int setPassword(char *pwd1, char*pwd2);
void setName(char *name);
