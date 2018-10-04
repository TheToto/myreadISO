#define _GNU_SOURCE

#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <err.h>

#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "iso9660.h"
#include "readiso.h"

void print_info(struct iso_prim_voldesc *iso_desc)
{
  printf("System Identifier: %.*s\n", ISO_SYSIDF_LEN, iso_desc->syidf);
  printf("Volume Identifier: %.*s\n", ISO_VOLIDF_LEN, iso_desc->vol_idf);
  printf("Block count: %d\n",iso_desc->vol_blk_count.le);
  printf("Block size: %d\n", iso_desc->vol_blk_size.le);
  printf("Creation date: %.*s\n", ISO_LDATE_LEN,iso_desc->date_creat);
  printf("Application Identifier: %.*s\n",ISO_APP_LEN, iso_desc->app_idf);
}

void print_help(void)
{
  printf("help: display command help\n");
  printf("info: display volume info\n");
  printf("ls: display the content of a directory\n");
  printf("cd: change current directory\n");
  printf("tree: display the tree of a directory\n");
  printf("get: copy file to local directory\n");
  printf("cat: display file content\n");
  printf("pwd: print current path\n");
  printf("quit: program exit\n");
}

static void tree_rec(char *iso, struct iso_dir *dir_cur, char *d,
    int *c_fil, int *c_dir)
{
  //.
  int offset = dir_cur->idf_len + sizeof(struct iso_dir);
  if (dir_cur->idf_len % 2 == 0) // PADDING FIELD
    offset += 1;
  dir_cur = go_to(dir_cur, offset);
  //..
  offset = dir_cur->idf_len + sizeof(struct iso_dir);
  if (dir_cur->idf_len % 2 == 0) // PADDING FIELD
    offset += 1;
  dir_cur = go_to(dir_cur, offset);

  while (dir_cur->idf_len != 0)
  {
    char pre[100];
    strcpy(pre, d);
    int len = 3;
    char *name;
    void *name_void = dir_cur + 1;
    name = name_void;
    len = dir_cur->idf_len;
    if ((dir_cur->type & 2) == 0)
      len -= 2;

    int offset = dir_cur->idf_len + sizeof(struct iso_dir);
    if (dir_cur->idf_len % 2 == 0) // PADDING FIELD
      offset += 1;
    struct iso_dir *next_dir_cur = go_to(dir_cur, offset);

    if (next_dir_cur->idf_len == 0)
    {
      printf("%s+-- %.*s", d, len, name);
      strcat(pre, "    ");
    }
    else
    {
      printf("%s|-- %.*s", d, len, name);
      strcat(pre, "|   ");
    }
    if ((dir_cur->type & 2) > 0)
    {
      *c_dir += 1;
      printf("/\n");
      tree_rec(iso, go_to(iso, dir_cur->data_blk.le * ISO_BLOCK_SIZE),
          pre, c_fil, c_dir);
    }
    else
    {
      *c_fil += 1;;
      printf("\n");
    }

    dir_cur = next_dir_cur;
  }

}

void command_tree(char *iso, struct iso_dir *cur, char *dir_name)
{
  printf("%s\n",dir_name);
  char pre[100];
  int c_fil = 0;
  int c_dir = 0;
  tree_rec(iso, cur, pre, &c_fil, &c_dir);
  printf("\n%d directories, %d files\n", c_dir, c_fil);
}

void command_pwd(struct state *iso_state)
{
  printf("/");
  for (int i = 0; i < iso_state->depth; i++)
  {
    printf("%s", iso_state->pwd[i]);
    printf("/");
  }
  printf("\n");
}

void command_ls(struct iso_dir *dir_cur)
{
  int dot_dir = 0;
  while (dir_cur->idf_len != 0)
  {
    int len = 3;
    char *name;
    if (dot_dir == 0)
      name = ".";
    else if (dot_dir == 1)
      name = "..";
    else
    {
      void *name_void = dir_cur + 1;
      name = name_void;
      len = dir_cur->idf_len;
      if ((dir_cur->type & 2) == 0)
        len -= 2;
    }
    char flag_d = '-';
    char flag_h = '-';
    if ((dir_cur->type & 2) > 0)
      flag_d = 'd';
    if ((dir_cur->type & 1) > 0)
      flag_h = 'h';

    printf("%c%c %9u %04d/%02d/%02d %02d:%02d %.*s\n",
      flag_d, flag_h, dir_cur->file_size.le, dir_cur->date[0] + 1900,
      dir_cur->date[1], dir_cur->date[2], dir_cur->date[3],
      dir_cur->date[4], len, name);
    int offset = dir_cur->idf_len + sizeof(struct iso_dir);
    if (dir_cur->idf_len % 2 == 0) // PADDING FIELD
      offset += 1;
    dir_cur = go_to(dir_cur, offset);
    dot_dir++;
  }
}

void command_cat(char *iso, struct iso_dir *dir_cur, char *file_name)
{
  while (dir_cur->idf_len != 0)
  {
    size_t len = 3;
    char *name;
    void *name_void = dir_cur + 1;
    name = name_void;
    len = dir_cur->idf_len;
    if ((dir_cur->type & 2) == 0)
      len -= 2;
    if (strlen(file_name) == len && strncmp(name, file_name, len) == 0)
    {
      //PRINT
      if ((dir_cur->type & 2) > 0)
      {
        fprintf(stderr,"Cet element est un dossier.\n");
        return;
      }
      char *cat_file = go_to(iso, dir_cur->data_blk.le * ISO_BLOCK_SIZE);
      fwrite(cat_file, 1, dir_cur->file_size.le, stdout);
      return;
    }
    int offset = dir_cur->idf_len + sizeof(struct iso_dir);
    if (dir_cur->idf_len % 2 == 0) // PADDING FIELD
      offset += 1;
    dir_cur = go_to(dir_cur, offset);
  }
  fprintf(stderr, "Cet element n'existe pas.\n");
}

void command_get(char *iso, struct iso_dir *dir_cur, char *file_name)
{
  while (dir_cur->idf_len != 0)
  {
    size_t len = 3;
    char *name;
    void *name_void = dir_cur + 1;
    name = name_void;
    len = dir_cur->idf_len;
    if ((dir_cur->type & 2) == 0)
      len -= 2;
    if (strlen(file_name) == len && strncmp(name, file_name, len) == 0)
    {
      //PRINT
      if ((dir_cur->type & 2) > 0)
      {
        fprintf(stderr,"Cet element est un dossier.\n");
        return;
      }
      char *cat_file = go_to(iso, dir_cur->data_blk.le * ISO_BLOCK_SIZE);
      FILE *output = fopen(file_name, "w");
      fwrite(cat_file, 1, dir_cur->file_size.le, output);
      fclose(output);
      return;
    }
    int offset = dir_cur->idf_len + sizeof(struct iso_dir);
    if (dir_cur->idf_len % 2 == 0) // PADDING FIELD
      offset += 1;
    dir_cur = go_to(dir_cur, offset);
  }
  fprintf(stderr, "Cet element n'existe pas.\n");
}

void command_cd(struct state *iso_state, char *dir_name)
{
  struct iso_dir *dir_cur = iso_state->dir_cur;
  int dot_dir = 0;
  while (dir_cur->idf_len != 0)
  {
    size_t len = 3;
    char *name;
    if (dot_dir == 0)
    {
      name = ".";
      len = 1;
    }
    else if (dot_dir == 1)
    {
      name = "..";
      len = 2;
    }
    else
    {
      void *name_void = dir_cur + 1;
      name = name_void;
      len = dir_cur->idf_len;
      if ((dir_cur->type & 2) == 0)
        len -= 2;
    }
    if (strlen(dir_name) == len && strncmp(name, dir_name, len) == 0)
    {
      //PRINT
      if ((dir_cur->type & 2) > 0)
      {
        iso_state->dir_cur = go_to(iso_state->iso,
            dir_cur->data_blk.le * ISO_BLOCK_SIZE);
        if (!strcmp(".", name))
        {
          if (iso_state->depth < 1)
            dir_name = "/";
          else
            dir_name = iso_state->pwd[iso_state->depth - 1];
        }
        else if (!strcmp("..", name))
        {
          if (iso_state->depth <= 1)
            dir_name = "/";
          else
            dir_name = iso_state->pwd[iso_state->depth - 2];
          iso_state->depth -= 1;
        }
        else
        {
          strcpy(iso_state->pwd[iso_state->depth], dir_name);
          iso_state->depth += 1;
        }
        printf("Changing to '%s' directory\n", dir_name);
        return;
      }
      fprintf(stderr,"Je rentre pas dans les fichiers !\n");
      return;
    }
    int offset = dir_cur->idf_len + sizeof(struct iso_dir);
    if (dir_cur->idf_len % 2 == 0) // PADDING FIELD
      offset += 1;
    dir_cur = go_to(dir_cur, offset);
    dot_dir++;
  }
  fprintf(stderr, "Cet element n'existe pas.\n");
}
