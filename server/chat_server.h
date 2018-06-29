//   TBDChat is a simple chat client and server using BSD sockets
//   Copyright (C) 2014 Michael Geitz Matthew Owens Shayne Wierbowski
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License along
//   with this program; if not, write to the Free Software Foundation, Inc.,
//   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
#include <openssl/sha.h>
/* Local Header Files */
#include "linked_list.h"

/* Preprocessor Macros */
// Misc constants
#define BACKLOG 2               // how many pending connections the queue will hold
#define BUFFERSIZE 128
#define SHA256_DIGEST 64
#define DEFAULT_ROOM 1000
#define DEFAULT_ROOM_NAME "Lobby"
#define SERVER_NAME "SERVER"
#define USERS_FILE "Users.bin"
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
struct Packet {
   time_t timestamp;
   char buf[BUFFERSIZE];
   char username[64];
   char realname[64];
   int options;
};
typedef struct Packet packet;


/* Function Prototypes */
// chat_server.c
int get_server_socket(char *hostname, char *port);
int start_server(int serv_socket, int backlog);
void debugPacket(packet *rx_pkt);
void sigintHandler(int sig_num);
int accept_client(int serv_sock);
// server_clients.c
void *client_receive(void *ptr);
int sanitizeInput(char *buf, int type);
int validUsername(char *username, int client);
int validRealname(char *realname, int client);
int validRoomname(char *roomname, int client);
int validPassword(char *pass1, char *pass2, int client);
int register_user(packet *in_pkt, int fd);
int login(packet *pkt, int fd);
void exit_client(packet *pkt, int fd);
void send_message(packet *pkt, int clientfd);
void sendError(char *error, int clientfd);
void sendMOTD(int fd);
void get_active_users(int fd);
void get_room_users(packet *in_pkt, int fd);
void user_lookup(packet *in_pkt, int fd);
void get_room_list(int fd);
void set_pass(packet *pkt, int fd);
void set_name(packet *pkt, int fd);
void join(packet *pkt, int fd);
void invite(packet *in_pkt, int fd);
void leave(packet *pkt, int fd);
void log_message(packet *pkt, int fd);
//char *passEncrypt(char *s);
int comparePasswords(unsigned char *pass1, unsigned char *pass2, int size);

#endif
