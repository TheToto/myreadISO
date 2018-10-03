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

#include "../iso9660.h"

struct state
{
  char *iso;
  struct iso_prim_voldesc *iso_desc;
  struct iso_dir *root_dir;
  struct iso_dir *dir_cur;
  int depth;
  char pwd[8][15];
};

void *go_to(void *ptr, long int offset)
{
  char *char_ptr = ptr;
  return char_ptr + offset;
}

char *to_char(void *ptr)
{
  return ptr;
}

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
        iso_state->dir_cur = iso_state->iso + dir_cur->data_blk.le * ISO_BLOCK_SIZE;
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

int parseline(char *cmd, struct state *iso_state)
{
  char name[100] = {0};
  char arg[4096] =  {0};
  int res = sscanf(cmd,"%s %s\n", name, arg);
  if (res == -1)
    return 0;
  if (strcmp(name, "quit") == 0)
    return -1;
  else if (strcmp(name, "ls") == 0)
    command_ls(iso_state->dir_cur);
  else if (strcmp(name, "pwd") == 0)
    command_pwd(iso_state);
  else if (strcmp(name, "cat") == 0)
    command_cat(iso_state->iso, iso_state->dir_cur, arg);
  /*else if (strcmp(name, "tree") == 0)
    print_tree();*/
  else if (strcmp(name, "get") == 0)
    command_get(iso_state->iso, iso_state->dir_cur, arg);
  else if (strcmp(name, "cd") == 0 && res == 1)
  {
    iso_state->dir_cur = iso_state->root_dir;
    printf("Changing to '%s' directory\n", "/");

  }
  else if (strcmp(name, "cd") == 0)
    command_cd(iso_state, arg);
  else if (strcmp(name, "info") == 0)
    print_info(iso_state->iso_desc);
  else if (strcmp(name, "help") == 0)
    print_help();
  else
    fprintf(stderr,"Fait help d'abord tocard %d\n", res);
  return 0;
}

int main(int argc, char *argv[])
{
  if (argc != 2)
    errx(1, "PAS D'ARGUMENTS PD !!!!!!");
  int fd = open(argv[1], O_RDONLY);
  if (fd == -1)
    errx(1, "J'ARRIVE PAS A L'OPEN Oo oO");

  struct stat buffer;
  if (fstat(fd, &buffer) == -1)
      errx(1, "tocard");

  char *iso = mmap(NULL, buffer.st_size,
      PROT_READ, MAP_PRIVATE, fd, 0);
  struct iso_prim_voldesc *iso_desc = go_to(iso, 32768);
  //print_info(iso_desc);

  struct iso_dir dir_root = iso_desc->root_dir;
  struct iso_dir *dir_cur = iso + dir_root.data_blk.le * ISO_BLOCK_SIZE;

  int depth = 0;
  struct state iso_state = { iso, iso_desc, dir_cur, dir_cur, depth,{"","","","","","","",""} };

  while(isatty(STDIN_FILENO))
  {
    printf("> ");
    char cmd[4096] = {0};
    fgets(cmd, 4096, stdin);
    if (strlen(cmd) == 1)
      continue;
    if (parseline(cmd, &iso_state) == -1)
      break;
  }
  if (!isatty(STDIN_FILENO))
  {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    while ((nread = getline(&line, &len, stdin)) != -1)
      if (parseline(line, &iso_state) == -1)
        break;
    free(line);
  }

  munmap(iso, buffer.st_size);
  close(fd);
  return 0;
}
