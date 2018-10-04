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

void *go_to(void *ptr, long int offset)
{
  char *char_ptr = ptr;
  return char_ptr + offset;
}

char *to_char(void *ptr)
{
  return ptr;
}

struct iso_dir *tmp_cd(struct state *iso_state, struct iso_dir *cur,
    char *dir_name, int is_cd)
{
  if (cur == NULL)
    return NULL;
  struct iso_dir *dir_cur = cur;
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
      if ((dir_cur->type & 2) > 0)
      {
        dir_cur = go_to(iso_state->iso, dir_cur->data_blk.le * ISO_BLOCK_SIZE);
        if (!is_cd)
          return dir_cur;
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
        return dir_cur;
      }
      return NULL;
    }
    int offset = dir_cur->idf_len + sizeof(struct iso_dir);
    if (dir_cur->idf_len % 2 == 0) // PADDING FIELD
      offset += 1;
    dir_cur = go_to(dir_cur, offset);
    dot_dir++;
  }
  return NULL;
}

static void save_pwd(struct state *iso_state)
{
  char old_pwd[120] = {0};
  strcat(old_pwd, "/");
  for (int i = 0; i < iso_state->depth; i++)
  {
    strcat(old_pwd, iso_state->pwd[i]);
    strcat(old_pwd, "/");
  }
  strcpy(iso_state->old_pwd, old_pwd);
}

int parseline(char *cmd, struct state *iso_state)
{
  char name[100] = {0};
  char arg[4096] =  {0};
  int res = sscanf(cmd,"%s %[^\n]", name, arg);
  if (res == -1)
    return 0;
  if (res == 2 && arg[strlen(arg)-1] == '/' && strlen(arg) > 1)
  {
    arg[strlen(arg)-1] = '\0';
  }
  char keeparg[4096];
  strcpy(keeparg, arg);
  char *token = arg;
  struct iso_dir *cur = iso_state->dir_cur;
  if (res == 2)
  {
    int is_cd = 0;
    if (strcmp(name, "cd") == 0)
    {
        is_cd = 1;
        if (*arg != '-')
          save_pwd(iso_state);
    }
    if (*arg == '/')
    {
      cur = iso_state->root_dir;
      if (is_cd)
      {
        iso_state->dir_cur = iso_state->root_dir;
        iso_state->depth = 0;
      }
    }
    const char s[2] = "/";
    char *next_token;
    token = strtok(arg, s);

    while(token != NULL)
    {
      next_token = strtok(NULL, s);
      if (next_token == NULL)
        break;
      cur = tmp_cd(iso_state, cur, token, is_cd);
      if (cur == NULL)
      {
        fprintf(stderr, "Le chemin est incorrect.\n");
        return 0;
      }
      token = next_token;
    }
    if (strcmp(name, "ls") == 0 || strcmp(name, "tree") == 0)
      cur = tmp_cd(iso_state,cur, token, is_cd);
  }

  if (strcmp(name, "quit") == 0 && res == 1)
    return -1;
  else if (strcmp(name, "ls") == 0)
    command_ls(cur);
  else if (strcmp(name, "pwd") == 0 && res == 1)
    command_pwd(iso_state);
  else if (strcmp(name, "cat") == 0)
    command_cat(iso_state->iso, cur, token);
  else if (strcmp(name, "tree") == 0 && res == 1)
    command_tree(iso_state->iso, cur, ".");
  else if (strcmp(name, "tree") == 0)
    command_tree(iso_state->iso, cur, keeparg);
  else if (strcmp(name, "get") == 0)
    command_get(iso_state->iso, cur, token);
  else if (strcmp(name, "cd") == 0 && (res == 1 || token == NULL))
  {
    iso_state->dir_cur = iso_state->root_dir;
    iso_state->depth = 0;
    printf("Changing to '%s' directory\n", "/");
  }
  else if (strcmp(name, "cd") == 0)
  {
    if (strcmp("-", arg) == 0)
    {
      char new_cmd[150] = {0};
      strcat(new_cmd, "cd ");
      strcat(new_cmd, iso_state->old_pwd);
      return parseline(new_cmd, iso_state);
    }
    else
    {
      iso_state->dir_cur = cur;
      command_cd(iso_state, token);
    }
  }
  else if (strcmp(name, "info") == 0 && res == 1)
    print_info(iso_state->iso_desc);
  else if (strcmp(name, "help") == 0 && res == 1)
    print_help();
  else
    fprintf(stderr,"Commande invalide.\n");
  return 0;
}

int main(int argc, char *argv[])
{
  if (argc != 2)
    errx(1, "Un argument est necessaire (ISO)");
  int fd = open(argv[1], O_RDONLY);
  if (fd == -1)
    errx(1, "Ouverture de l'ISO impossible.");

  struct stat buffer;
  if (fstat(fd, &buffer) == -1)
      errx(1, "tocard");

  long int min_len = 32768 + sizeof(struct iso_prim_voldesc);
  if (buffer.st_size <= 32768 + min_len)
    errx(1, "Cet ISO n'est pas valide (trop petit)");
  char *iso = mmap(NULL, buffer.st_size,
      PROT_READ, MAP_PRIVATE, fd, 0);
  struct iso_prim_voldesc *iso_desc = go_to(iso, 32768);
  //print_info(iso_desc);
  if (iso_desc->std_identifier[0] != 'C' ||
      iso_desc->std_identifier[1] != 'D' ||
      iso_desc->std_identifier[2] != '0' ||
      iso_desc->std_identifier[3] != '0' ||
      iso_desc->std_identifier[4] != '1' )
    errx(1, "Cet ISO n'est pas valide (MAGIC CD001)");

  struct iso_dir dir_root = iso_desc->root_dir;
  struct iso_dir *dir_cur = go_to(iso, dir_root.data_blk.le * ISO_BLOCK_SIZE);

  int depth = 0;
  struct state iso_state = { iso, iso_desc, dir_cur, dir_cur, depth,
    {"","","","","","","",""}, "/"};

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
