#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

int main(void) {
  pid_t pid = getpid();
  char cmd[0x100];
  int fd;

  sprintf(cmd, "insmod kfunch_ftrace.ko obj_pid=%d", pid);
  printf("Run cmd: %s\n", cmd);
  system(cmd);

  fd = open("kfunch_ftrace.c", O_RDONLY);
  close(fd);
  system("rmmod kfunch_ftrace");

  return 0;
}
