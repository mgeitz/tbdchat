#include <string.h>
#include "linked_list.h"
#include <stdio.h>
int insert(User **head, User *new_user)
{
   User *temp = *head;
   if(*head == NULL)
   {
      new_user->next = NULL;
      *head = new_user;
      return 1;
   } 
   
   if(strcmp(temp->username, new_user->username) == 1)
   {
      return 0;
   }
   
   while(temp->next != NULL) 
   {
      temp = temp->next;
      if(strcmp(temp->username, new_user->username) == 1)
      {
         return 0;
      }
   }
   temp->next = new_user;
   new_user->next = NULL;
   return 1;
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
