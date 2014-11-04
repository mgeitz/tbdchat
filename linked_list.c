#include <string.h>
#include "linked_list.h"
#include <stdio.h>
void insert(User **head, User *new_user)
{
   User *temp = *head;
   if(*head == NULL)
   {
      new_user->next = NULL;
      *head = new_user;
      return;
   } 

   while(temp->next != NULL) 
   {
       temp = temp->next;
   }
   temp->next = new_user;
   new_user->next = NULL;
}

char *get_real_name(User **head, char *user)
{
   char *error = "ERROR";
   User *temp = *head;

   if(*head == NULL) return error;

   while(strcmp(user, temp->username) != 0)
   {
      if(temp->next == NULL) return error;
      temp=temp->next;
   }

   return temp->real_name;
}

