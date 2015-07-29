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
#ifndef LINKED_LISTS_H
#define LINKED_LISTS_H

/* System Header Files */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <pthread.h>

#define SHA256_DIGEST 64
#define USERNAME_LENGTH 64
#define REALNAME_LENGTH 64
#define ROOMNAME_LENGTH 16

/* Structures */
struct user {
   char username[USERNAME_LENGTH];
   char real_name[REALNAME_LENGTH];
   unsigned char password[SHA256_DIGEST];
   int sock;
   int roomID;
   struct user *next;
};
typedef struct user User;

struct room {
   int ID;
   int fd;
   char name[ROOMNAME_LENGTH];
   pthread_mutex_t user_list_mutex;
   struct node *user_list;
   struct room *next;
};
typedef struct room Room;

struct node {
   void *data;
   struct node *next;
};
typedef struct node Node;

/* Function Prototypes */
int insertNode(Node **head, Node *new_node, pthread_mutex_t mutex);
int removeNode(Node **head, Node *new_node, pthread_mutex_t mutex);

// user nodes
int insertUser(Node  **head, User *new_user, pthread_mutex_t mutex);
int removeUser(Node  **head, User *new_user, pthread_mutex_t mutex);
char *get_real_name(Node  **head, char *user, pthread_mutex_t mutex);
unsigned char *get_password(Node  **head, char *user, pthread_mutex_t mutex);
void readUserFile(Node  **head, char *filename, pthread_mutex_t mutex);
void writeUserFile(Node **head, char *filename, pthread_mutex_t mutex);
void printList(Node **head, pthread_mutex_t mutex);
User *get_user(Node **head, char *user, pthread_mutex_t mutex);
int listLength(Node **head, pthread_mutex_t mutex);
// room nodes
int insertRoom(Node **head, Room *new_room, pthread_mutex_t mutex);
int Rget_ID(Node **head, char *name, pthread_mutex_t mutex);
char *Rget_name(Node **head, int ID, pthread_mutex_t mutex);
void RprintList(Node  **head, pthread_mutex_t mutex);
Room *Rget_roomFID(Node **head, int ID, pthread_mutex_t mutex);
Room *Rget_roomFNAME(Node **head, char *name, pthread_mutex_t mutex);
int createRoom(Node **head, int ID, char *name, pthread_mutex_t mutex);

#endif
