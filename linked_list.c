#include <string.h>
#include "linked_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

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

char *get_password(User **head, char *user)
{
   char *error = "ERROR";
   User *temp = *head;

   if(*head == NULL) return error;

   while(strcmp(user, temp->username) != 0)
   {
      if(temp->next == NULL) return error;
      temp=temp->next;
   }

   return temp->password;
}

void readUserFile(User **head, char *filename)
{
   int fd = open(filename, O_RDONLY);
   int n;
   int i;
   struct stat st;
   User *temp;
   
   *head = NULL;
   if(fd == -1)
   {
      close(fd);
      return;
   }
   else
   {
      fstat(fd, &st);
      n = (st.st_size / sizeof(User));
      //printf("n: %d\n", n);
      for(i = 0; i < n; i++)
      {
         temp = (User *)malloc(sizeof(User));
         read(fd, temp, sizeof(User));
         temp->next = NULL;
         //printf("%s, %s, %s\n", temp->username, temp->real_name, temp->password);
         insert(head, temp);
      }
   }
   close(fd);
}

void writeUserFile(User **head, char *filename)
{
   int fd = open(filename, O_WRONLY | O_CREAT, S_IRWXU);
   User *temp = *head;
   
   if(*head == NULL)
   {
      close(fd);
      return;
   }
   
   //printf("wrote: %s, %s, %s\n", temp->username, temp->real_name, temp->password);
   write(fd, temp, sizeof(User));
   while(temp->next != NULL)
   {
      temp = temp->next;
      //printf("wrote: %s, %s, %s\n", temp->username, temp->real_name, temp->password);
      write(fd, temp, sizeof(User));
   }
}

void printList(User **head)
{
   User *temp = *head;
   printf("Printing List\n");
   if(*head == NULL)
   {
      printf("NULL\n");
      return;
   }
   
   printf("%s, %s, %s\n", temp->username, temp->real_name, temp->password);
   while(temp->next != NULL)
   {
      temp = temp->next;
      printf("%s, %s, %s\n", temp->username, temp->real_name, temp->password);
   }
   printf("End List\n");
}
