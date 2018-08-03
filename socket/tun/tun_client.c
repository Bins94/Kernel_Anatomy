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

#define REMOTE_IP "172.20.20.187"

int main(void) {
  struct ifreq ifr;
  int fd, sock_fd, net_fd, c;
  int err, remotelen = 1024;
  struct sockaddr_in local, remote;

  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket()\n");
    exit(1);
  }
  close(fd);

  memset(&remote, 0, sizeof(remote));
  remote.sin_family = AF_INET;
  /* Remote ip set in tun_device */
  remote.sin_addr.s_addr = inet_addr("172.20.20.187");
  remote.sin_port = htons(20000);

  if( connect(sock_fd, (struct sockaddr*)&remote, sizeof(remote)) < 0) {
    perror("connect()\n");
    exit(1);
  }
  printf("Connect OK!\n");
  while(1) {
    sleep(2);
    char *msg = "AAAAAAAA\n";
    if(c = sendto(sock_fd, msg, sizeof(msg), MSG_DONTWAIT, (struct sockaddr*)&remote, sizeof(remote)))
      printf("Sent %d byte\n", c);
    else
      perror("sendto()\n");
  }
  return 0;
}




