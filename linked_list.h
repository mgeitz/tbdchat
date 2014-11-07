#include <string.h>
#include "linked_list.h"
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
   struct user *next;
};

typedef struct user User;
int insert(User **head, User *new_user);
char *get_real_name(User **head, char *user);
char *get_password(User **head, char *user);
void readUserFile(User **head, char *filename);
void writeUserFile(User **head, char *filename);
void printList(User **head);
