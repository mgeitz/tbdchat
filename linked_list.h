struct user {
   char username[50];
   char real_name[64];
   struct user *next;
};

typedef struct user User;
int insert(User **head, User *new_user);
char *get_real_name(User **head, char *user);

