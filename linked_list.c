#include <string.h>
#include "linked_list.h"
#include <stdio.h>
void insert(User **head, User *new_user) {
   if(*head == NULL) {
      new_user->next = *head;
      *head = new_user;
   } 
   else {
       User *curr = *head;
       while(curr->next != NULL) { curr = curr->next; }
       new_user->next = curr->next;
       curr->next = new_user;
   }
}

char *get_real_name(User **head, char *user) {
   char *error = "ERROR";

   if(*head == NULL) return error;

   User *curr = *head;
   while(strcmp(user, curr->username) != 0 && curr->next != NULL) {
      //if(curr->next == NULL) return error;
      curr=curr->next;
   }
   return curr->real_name;
}

