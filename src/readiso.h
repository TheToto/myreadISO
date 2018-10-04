
#ifndef READISO_H
# define READISO_H
# include "iso9660.h"

struct state
{
  char *iso;
  struct iso_prim_voldesc *iso_desc;
  struct iso_dir *root_dir;
  struct iso_dir *dir_cur;
  int depth;
  char pwd[8][15];
  char old_pwd[120];
};

void print_info(struct iso_prim_voldesc *iso_desc);
void print_help(void);
void command_pwd(struct state *iso_state);
void command_ls(struct iso_dir *dir_cur);
void command_cat(char *iso, struct iso_dir *dir_cur, char *file_name);
void command_get(char *iso, struct iso_dir *dir_cur, char *file_name);
void command_cd(struct state *iso_state, char *dir_name);
void command_tree(char *iso, struct iso_dir *cur, char *dir_name);

void *go_to(void *ptr, long int offset);
char *to_char(void *ptr);

#endif /* !READISO_H */
