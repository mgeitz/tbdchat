/* 
//   Program:             TBD Chat Server
//   File Name:           linked_list.c
//   Authors:             Matthew Owens, Michael Geitz, Shayne Wierbowski
*/

#include "linked_list.h"

extern int numRooms;

int insertNode(Node **head, Node *new_node, pthread_mutex_t mutex) {
   pthread_mutex_lock(&mutex);
   Node *temp = *head;
   if(*head == NULL) {
      new_node->next = NULL;
      *head = new_node;
      pthread_mutex_unlock(&mutex);
      return 1;
   }

   while(temp->next != NULL) { temp = temp->next; }

   temp->next = new_node;
   new_node->next = NULL;
   pthread_mutex_unlock(&mutex);
   return 1;
}

int removeNode(Node **head, Node *to_remove, pthread_mutex_t mutex) {
   pthread_mutex_lock(&mutex);
   if(*head == NULL) {
      printf("Cannot remove from an empty list\n");
      pthread_mutex_unlock(&mutex);
      return 0;
   
   }

   Node *temp = *head;

   if(temp->data == to_remove->data) {
      *head = temp->next;
      pthread_mutex_unlock(&mutex);
      return 1;
   }

   while(temp->next != NULL) {
      if(temp->next->data ==  to_remove->data) {
         temp->next = temp->next->next;
         pthread_mutex_unlock(&mutex);
         return 1;
      }
   }

   printf("Specified node not found\n");
   pthread_mutex_unlock(&mutex);
   return 0;

   
}


/* Insert a new user node into the list over user nodes passed in */
int insertUser(Node **head, User *new_user, pthread_mutex_t mutex) {
   pthread_mutex_lock(&mutex);
   printf("Inserting %s\n", new_user->username);
   Node *temp = *head;
   Node *new_node = (Node *)malloc(sizeof(Node));
   new_node->data = (void *)new_user;
   
   if (*head == NULL) {
      *head = new_node;
      printf("Insert Success\n");
      pthread_mutex_unlock(&mutex);
      return 1;
   } 
   
   User *temp_user = (User *)temp->data;

   if (strcmp(temp_user->username, new_user->username) == 0) {
      printf("Insert Failure\n");
      pthread_mutex_unlock(&mutex);
      return 0;
   }
   
   while (temp->next != NULL) {
      temp = temp->next;
      temp_user = (User *)temp->data;

      if (strcmp(temp_user->username, new_user->username) == 0) {
         printf("Insert Failure\n");
         pthread_mutex_unlock(&mutex);
         return 0;
      }
   }
   temp->next = new_node;
   printf("Insert Success\n");
   pthread_mutex_unlock(&mutex);
   return 1;
}


/* Remove a user node from the list of user nodes passed in */
int removeUser(Node **head, User *user, pthread_mutex_t mutex) {
   printf("Removing user: %s\n", user->username);
   pthread_mutex_lock(&mutex);
   printList(head, NULL);
   Node *current = *head;
   Node *next;

   if (*head == NULL) {
      printf("Can't remove from empty list.\n");
      pthread_mutex_unlock(&mutex);
      return 0;
   }
   
   User *current_user = (User *)current->data;
   User *next_user;

   if (strcmp(current_user->username, user->username) == 0) {
      pthread_mutex_unlock(&mutex);
      removeNode(head, current, mutex);
      printf("Potentially removed a user from a list.\n");
      printList(head, NULL);
      return 1;
   }
   while (current->next != NULL) {
      current = current->next;
      next = current->next;
      next_user = (User *)next->data;

      if (strcmp(next_user->username, user->username) == 0) {
         pthread_mutex_unlock(&mutex);
         removeNode(head, next, mutex);
         printf("Potentially removed a user from a list.\n");
         printList(head, NULL);
         return 1;
      }
   }
   printf("User not found in list, nothing removed.\n");
   pthread_mutex_unlock(&mutex);
   return 0;
}


/* Return the display name for given user name in the list */
char *get_real_name(Node **head, char *user, pthread_mutex_t mutex) {
   char *error = "ERROR";
   pthread_mutex_lock(&mutex);
   Node *temp = *head;

   if (*head == NULL) {
      pthread_mutex_unlock(&mutex);
      return error;
   }

   User *current = (User *)temp->data;
   if(strcmp(user, current->username) == 0){ 
      pthread_mutex_unlock(&mutex);
      return current->real_name;
   }

   while(temp->next != NULL) {
      temp = temp->next;
      current = (User *)temp->data;

      if(strcmp(user, current->username) == 0) {
         pthread_mutex_unlock(&mutex);
         return current->real_name;
      }
   }

   pthread_mutex_unlock(&mutex);
   return error;

}


/* Return stored password for user */
char *get_password(Node  **head, char *user, pthread_mutex_t mutex) {
   char *error = "ERROR";
   pthread_mutex_lock(&mutex);
   Node *temp = *head;

   if(*head == NULL) {
      pthread_mutex_unlock(&mutex);
      return error;
   }
   User *current = (User *)temp->data;
   if(strcmp(user, current->username) == 0){
      pthread_mutex_unlock(&mutex);
      return current->password;
   }
      
   while(temp->next != NULL) {
      temp = temp->next;
      current = (User *)temp->data;

      if(strcmp(user, current->username) == 0) {
         pthread_mutex_unlock(&mutex);
         return current->password;
      }
   }
   pthread_mutex_unlock(&mutex);
   return error;
}


User *get_user(Node **head, char *user, pthread_mutex_t mutex) {
   pthread_mutex_lock(&mutex);
   Node *temp = *head;

   if(*head == NULL) {
      pthread_mutex_unlock(&mutex);
      return NULL;
   }

   User *current = (User *)temp->data;
   if(strcmp(user, current->username) == 0){
      pthread_mutex_unlock(&mutex);
      return current;
   }

   while(temp->next != NULL) {
      temp = temp->next;
      current = (User *)temp->data;

      if(strcmp(user, current->username) == 0) {
         pthread_mutex_unlock(&mutex);
         return current;
      }
   }
   pthread_mutex_unlock(&mutex);
   return NULL;

}

User *clone_user(User *user, pthread_mutex_t mutex) {
   printf("Cloning User\n");
   User *new_user = (User *)malloc(sizeof(User));
   pthread_mutex_lock(&mutex);
   printf("Old User: %s, %s, %s\n", user->username, user->real_name,
           user->password);
   strcpy(new_user->username, user->username);
   strcpy(new_user->real_name, user->real_name);
   strcpy(new_user->password, user->password);
   new_user->sock = user->sock;
   pthread_mutex_unlock(&mutex);
   new_user-> next = NULL;
   printf("New User: %s, %s, %s\n", new_user->username, new_user->real_name,
           new_user->password);
   return new_user;
}


/* Populate user list from Users.bin */
void readUserFile(Node **head, char *filename, pthread_mutex_t mutex) {
   int fd = open(filename, O_RDONLY);
   int n;
   int i;
   struct stat st;
   Node *temp;
   User *current;
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
         temp = (Node *)malloc(sizeof(Node));
         current = (User *)malloc(sizeof(User));
         read(fd, current, sizeof(User));
         temp->data = current;
         temp->next = NULL;
         //printf("%s, %s, %s\n", temp->username, temp->real_name, temp->password);
         insertNode(head, temp, mutex);
      }
   }
   close(fd);
}


/* Write user list to Users.bin */
void writeUserFile(Node  **head, char *filename, pthread_mutex_t mutex) {
   int fd = open(filename, O_WRONLY | O_CREAT, S_IRWXU);
   pthread_mutex_lock(&mutex);
   Node *temp = *head;

   if(*head == NULL) {
      pthread_mutex_unlock(&mutex);
      close(fd);
      return;
   }
   
   User *current = (User *)temp->data;
   //printf("wrote: %s, %s, %s\n", temp->username, temp->real_name, temp->password);
   write(fd, current, sizeof(User));
   while(temp->next != NULL) {
      temp = temp->next;
      current = (User *)temp->data;
      //printf("wrote: %s, %s, %s\n", temp->username, temp->real_name, temp->password);
      write(fd, current, sizeof(User));
   }
   pthread_mutex_unlock(&mutex);
}


/* Print contents of list */
void printList(Node **head, pthread_mutex_t mutex) {
   pthread_mutex_lock(&mutex);
   Node *temp = *head;
   printf("Printing User List\n");
   if(*head == NULL) {
      printf("NULL\n");
      pthread_mutex_unlock(&mutex);
      return;
   }
   User *current = (User *)temp->data;
   printf("%s, %s, %s, %d\n", current->username, current->real_name, current->password,
          current->sock);
   while(temp->next != NULL) {
      temp = temp->next;
      current = temp->data;
      printf("%s, %s, %s, %d\n", current->username, current->real_name, current->password,
             current->sock);
   }
   pthread_mutex_unlock(&mutex);
   printf("End User List\n");
}


// ROOM METHODS


/* Insert new room node to room list */
int insertRoom(Node **head, Room *new_room, pthread_mutex_t mutex) {
   pthread_mutex_lock(&mutex);
   Node *temp = *head;
   Room *current;
   if(*head == NULL) {
      Node *new = (Node *)malloc(sizeof(Node));
      new->data = (void *)new_room;
      new->next = NULL;
      *head = new;
      pthread_mutex_unlock(&mutex);
      return 1;
   }
   current = (Room *)temp->data;
   
   if(strcmp(current->name, new_room->name) == 0 || current->ID == new_room->ID) {
      pthread_mutex_unlock(&mutex);
      return 0;
   }
   
   while(temp->next != NULL) {
      temp = temp->next;
      current = (Room *)temp->data;

      if(strcmp(current->name, new_room->name) == 0 || current->ID == new_room->ID) {
         pthread_mutex_unlock(&mutex);
         return 0;
      }
   }
   Node *new_node = (Node *)malloc(sizeof(Node));
   new_node->data = (void *)new_room;
   new_node->next = NULL;
   temp->next = new_node;
   pthread_mutex_unlock(&mutex);
   return 1;
}


int createRoom(Node **head, int ID, char *name, pthread_mutex_t mutex) {
   printf("Creating room %d %s\n", ID, name);
   Room *newRoom = (Room *) malloc(sizeof(Room));
   newRoom->ID = ID;
   newRoom->user_list_mutex = PTHREAD_MUTEX_INITIALIZER;
   strncpy(newRoom->name, name, sizeof(newRoom->name));
   newRoom->user_list = NULL;
   numRooms++;
   return insertRoom(head, newRoom, mutex);
}

/* Return ID of room node from its name*/
int Rget_ID(Node **head, char *name, pthread_mutex_t mutex) {
   int error = -1;
   pthread_mutex_lock(&mutex);
   Node *temp = *head;
   Room *current;

   if(*head == NULL) {
      pthread_mutex_unlock(&mutex);
      return error;
   }
   current = (Room *)temp->data;

   while(strcmp(name, current->name) != 0) {
      if(temp->next == NULL) {
         pthread_mutex_unlock(&mutex);
         return error;
      }
      temp=temp->next;
      current = (Room *)temp->data;
   }
   pthread_mutex_unlock(&mutex);
   return current->ID;
}


/* Return name of room node from ID */
char *Rget_name(Node **head, int ID, pthread_mutex_t mutex) {
   char *error = "ERROR";
   pthread_mutex_lock(&mutex);
   Node *temp = *head;
   Room *current;

   if(*head == NULL) {
      pthread_mutex_unlock(&mutex);
      return error;
   }
   current = (Room *)temp->data;

   while(ID != current->ID) {
      if(temp->next == NULL) {
         pthread_mutex_unlock(&mutex);
         return error;
      }
      temp=temp->next;
      current = (Room *)temp->data;
   }
   pthread_mutex_unlock(&mutex);
   return current->name;
}


/* Print contents of room list */
void RprintList(Node **head, pthread_mutex_t mutex) {
   pthread_mutex_lock(&mutex);
   Node *temp = *head;
   Room *current;

   printf("Printing Room List\n");
   if(*head == NULL) {
      printf("NULL\n");
      pthread_mutex_unlock(&mutex);
      return;
   }
   current = (Room *)temp->data; 
   printf("Room ID: %d, Room Name: %s,\n", current->ID, current->name);
   printf("Contains Users...\n");
   //printList(&(temp->user_list));
   while(temp->next != NULL) {
      temp = temp->next;
      current = (Room *)temp->data;
      printf("Room ID: %d, Room Name: %s,\n", current->ID, current->name);
      printf("Contains Users...\n");
      printList(&(current->user_list), current->user_list_mutex);
   }
   printf("End Room List\n");
   pthread_mutex_unlock(&mutex);
}


/*  */
Room *Rget_roomFID(Node **head, int ID, pthread_mutex_t mutex) {
   pthread_mutex_lock(&mutex);
   Node *temp = *head;
   Room *current;

   if(*head == NULL) {
      pthread_mutex_unlock(&mutex);
      return NULL;
   }
   current = (Room *)temp->data;
   while(ID != current->ID) {
      if(temp->next == NULL) {
         pthread_mutex_unlock(&mutex);
         return NULL;
      }
      temp = temp->next;
      current = (Room *)temp->data;
   }
   pthread_mutex_unlock(&mutex);
   return current;
}


/*  */
Room *Rget_roomFNAME(Node **head, char *name, pthread_mutex_t mutex) {
   pthread_mutex_lock(&mutex);
   Node *temp = *head;
   Room *current;

   if(*head == NULL) {
      pthread_mutex_unlock(&mutex);
      return NULL;
   }
   current = (Room *)temp->data;
   while(strcmp(name, current->name) != 0) {
      if(temp->next == NULL) {
         pthread_mutex_unlock(&mutex);
         return NULL;
      }
      temp = temp->next;
      current = (Room *)temp->data;
   }
   
   pthread_mutex_unlock(&mutex);
   return current;
}
