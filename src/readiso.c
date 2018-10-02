#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <err.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../iso9660.h"

void *go_to(void *ptr, long int offset)
{
  char *char_ptr = ptr;
  return char_ptr + offset;
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

void command_ls(struct iso_prim_voldesc *iso_desc)
{

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

  struct iso_dir cur_dir = iso_desc->root_dir;
  

  while(1)
  {
    printf("> ");
    char cmd[4096];
    fgets(cmd, 4096, stdin);
    //printf("cmd %d %s",1,cmd);
    char name[100];
    char arg[4096];
    int res = sscanf(cmd,"%s %s\n", name, arg);
    //printf("res : %d, :%s:%s:\n",res, name ,arg);
    if (strcmp(name, "quit") == 0)
      break;
    else if (strcmp(name, "info") == 0)
      print_info(iso_desc);
    else if (strcmp(name, "help") == 0)
      print_help();
    else
      fprintf(stderr,"Fait help d'abord tocard\n");
  }

  munmap(iso, buffer.st_size);
  close(fd);
  return 0;
}
