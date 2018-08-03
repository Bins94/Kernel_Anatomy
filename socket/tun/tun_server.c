#include <stdio.h>
#include <stdlib.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/in.h>

#define PORT 20000
int main(void) {
  struct ifreq ifr;
  int fd, sock_fd, net_fd;
  int err, remotelen = 1024;
  struct sockaddr_in local, remote;

  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket()\n");
    exit(1);
  }

  memset(&local, 0, sizeof(remote));
  local.sin_family = AF_INET;
  local.sin_addr.s_addr = htonl(INADDR_ANY);
  local.sin_port = htons(POOT);

  if(bind(sock_fd, (struct sockaddr*)&local, sizeof(local)) < 0) {
    perror("bind()\n");
    exit(1);
  }
  if (listen(sock_fd, 5) < 0) {
    perror("listen()\n");
    exit(1);
  }
  while(1) {
    if(accept(sock_fd, (struct sockaddr*)&remote, &remotelen)) {
      printf("Connect!\n");
    }
  }

  return 0;
}


