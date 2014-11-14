#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

struct user {
   char username[50];
   char real_name[64];
   char password[32];
   int sock;
   struct user *next;
};

struct room {
   int ID;
   char name[32];
   struct user *user_list;
   struct room *next;
};

typedef struct user User;
typedef struct room Room;
int insert(User **head, User *new_user);
char *get_real_name(User **head, char *user);
char *get_password(User **head, char *user);
void readUserFile(User **head, char *filename);
void writeUserFile(User **head, char *filename);
void printList(User **head);
User *get_user(User **head, char *user);

int Rinsert(Room **head, Room *new_room);
int Rget_ID(Room **head, char *name);
char *Rget_name(Room **head, int ID);
void RprintList(Room **head);
Room *Rget_roomFID(Room **head, int ID);
Room *Rget_roomFNAME(Room **head, char *name);

