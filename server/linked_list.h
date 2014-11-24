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

/* Structures */
struct user {
   char username[50];
   char real_name[64];
   char password[32];
   int sock;
   struct user *next;
};
typedef struct user User;

struct room {
   int ID;
   char name[32];
   pthread_mutex_t user_list_mutex;
   struct user *user_list;
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
int insertUser(User **head, User *new_user, pthread_mutex_t mutex);
int removeUser(User **head, User *new_user, pthread_mutex_t mutex);
char *get_real_name(User **head, char *user, pthread_mutex_t mutex);
char *get_password(User **head, char *user, pthread_mutex_t mutex);
User *clone_user(User *user, pthread_mutex_t mutex);
void readUserFile(User **head, char *filename, pthread_mutex_t mutex);
void writeUserFile(User **head, char *filename, pthread_mutex_t mutex);
void printList(User **head, pthread_mutex_t mutex);
User *get_user(User **head, char *user, pthread_mutex_t mutex);
// room nodes
int insertRoom(Room **head, Room *new_room, pthread_mutex_t mutex);
int Rget_ID(Room **head, char *name, pthread_mutex_t mutex);
char *Rget_name(Room **head, int ID, pthread_mutex_t mutex);
void RprintList(Room **head, pthread_mutex_t mutex);
Room *Rget_roomFID(Room **head, int ID, pthread_mutex_t mutex);
Room *Rget_roomFNAME(Room **head, char *name, pthread_mutex_t mutex);
int createRoom(Room **head, int ID, char *name, pthread_mutex_t mutex);

#endif
