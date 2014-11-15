#include "linked_list.h"

/* Insert a new user node into the list over user nodes passed in */
int insert(User **head, User *new_user){
   printf("Inserting %s\n", new_user->username);
   User *temp = *head;
   if (*head == NULL) {
      new_user->next = NULL;
      *head = new_user;
      printf("Insert Success\n");
      return 1;
   } 
   
   if (strcmp(temp->username, new_user->username) == 0) {
      printf("Insert Failure\n");
      return 0;
   }
   
   while (temp->next != NULL) {
      temp = temp->next;
      if (strcmp(temp->username, new_user->username) == 0) {
         printf("Insert Failure\n");
         return 0;
      }
   }
   temp->next = new_user;
   new_user->next = NULL;
   printf("Insert Success\n");
   return 1;
}


/* Return the display name for given user name in the list */
char *get_real_name(User **head, char *user) {
   char *error = "ERROR";
   User *temp = *head;
   
   if (*head == NULL) return error;
   
   while (strcmp(user, temp->username) != 0) {
      if (temp->next == NULL) return error;
      temp=temp->next;
   }
   
   return temp->real_name;
}


/* Return stored password for user */
char *get_password(User **head, char *user) {
   char *error = "ERROR";
   User *temp = *head;

   if(*head == NULL) return error;

   while(strcmp(user, temp->username) != 0) {
      if(temp->next == NULL) return error;
      temp=temp->next;
   }

   return temp->password;
}


User *get_user(User **head, char *user) {
   User *temp = *head;

   if(*head == NULL) return NULL;

   while(strcmp(user, temp->username) != 0  &&
         strcmp(user, temp->real_name) != 0) {

      if(temp->next == NULL) return NULL;
      temp = temp->next;
   }

   return temp;
}


/* Populate user list from Users.bin */
void readUserFile(User **head, char *filename) {
   int fd = open(filename, O_RDONLY);
   int n;
   int i;
   struct stat st;
   User *temp;
   
   *head = NULL;
   if(fd == -1) {
      close(fd);
      return;
   }
   else {
      fstat(fd, &st);
      n = (st.st_size / sizeof(User));
      //printf("n: %d\n", n);
      for(i = 0; i < n; i++) {
         temp = (User *)malloc(sizeof(User));
         read(fd, temp, sizeof(User));
         temp->next = NULL;
         //printf("%s, %s, %s\n", temp->username, temp->real_name, temp->password);
         insert(head, temp);
      }
   }
   close(fd);
}


/* Write user list to Users.bin */
void writeUserFile(User **head, char *filename) {
   int fd = open(filename, O_WRONLY | O_CREAT, S_IRWXU);
   User *temp = *head;
   
   if(*head == NULL) {
      close(fd);
      return;
   }
   
   //printf("wrote: %s, %s, %s\n", temp->username, temp->real_name, temp->password);
   write(fd, temp, sizeof(User));
   while(temp->next != NULL) {
      temp = temp->next;
      //printf("wrote: %s, %s, %s\n", temp->username, temp->real_name, temp->password);
      write(fd, temp, sizeof(User));
   }
}


/* Print contents of list */
void printList(User **head) {
   User *temp = *head;
   printf("Printing List\n");
   if(*head == NULL) {
      printf("NULL\n");
      return;
   }
   
   printf("%s, %s, %s\n", temp->username, temp->real_name, temp->password);
   while(temp->next != NULL) {
      temp = temp->next;
      printf("%s, %s, %s\n", temp->username, temp->real_name, temp->password);
   }
   printf("End List\n");
}


// ROOM METHODS
/* Insert new room node to room list */
int Rinsert(Room **head, Room *new_room){
   Room *temp = *head;
   if(*head == NULL) {
      new_room->next = NULL;
      *head = new_room;
      return 1;
   }
   
   if(strcmp(temp->name, new_room->name) == 0 || temp->ID == new_room->ID) {
      return 0;
   }
   
   while(temp->next != NULL) {
      temp = temp->next;
      if(strcmp(temp->name, new_room->name) == 0 || temp->ID == new_room->ID) {
         return 0;
      }
   }
   temp->next = new_room;
   new_room->next = NULL;
   return 1;
}


/* Return ID of room node from its name*/
int Rget_ID(Room **head, char *name) {
   int error = -1;
   Room *temp = *head;
   
   if(*head == NULL) return error;
   
   while(strcmp(name, temp->name) != 0) {
      if(temp->next == NULL) return error;
      temp=temp->next;
   }
   
   return temp->ID;
}


/* Return name of room node from ID */
char *Rget_name(Room **head, int ID) {
   char *error = "ERROR";
   Room *temp = *head;
   
   if(*head == NULL) return error;
   
   while(ID != temp->ID) {
      if(temp->next == NULL) return error;
      temp=temp->next;
   }
   
   return temp->name;
}


/* Print contents of room list */
void RprintList(Room **head) {
   Room *temp = *head;
   printf("Printing Room List\n");
   if(*head == NULL) {
      printf("NULL\n");
      return;
   }
   
   printf("Room ID: %d, Room Name: %s,\n", temp->ID, temp->name);
   printf("Contains Users...\n");
   printList(&(temp->user_list));
   while(temp->next != NULL) {
      temp = temp->next;
      printf("Room ID: %d, Room Name: %s,\n", temp->ID, temp->name);
      printf("Contains Users...\n");
      printList(&(temp->user_list));
   }
   printf("End Room List\n");
}


/*  */
Room *Rget_roomFID(Room **head, int ID) {
   Room *temp = *head;
   
   if(*head == NULL) return NULL;
   
   while(ID != temp->ID) {
      if(temp->next == NULL) return NULL;
      temp = temp->next;
   }
   
   return temp;
}


/*  */
Room *Rget_roomNAME(Room **head, char *name) {
   Room *temp = *head;
   
   if(*head == NULL) return NULL;
   
   while(strcmp(name, temp->name) != 0) {
      if(temp->next == NULL) return NULL;
      temp = temp->next;
   }
   
   return temp;
}
