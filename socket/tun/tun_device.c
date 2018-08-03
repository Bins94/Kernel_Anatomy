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
#include <errno.h>
#include <stdint.h>

#define DEV_NAME "test_tun"

int main(void) {
  struct ifreq ifr;
  int fd, sock_fd, net_fd;
  int err, remotelen = 1024;
  struct sockaddr_in local, remote;

  /* Set device name, list in `ip link` */
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI | 0x10 | 0x20;
  strcpy(ifr.ifr_name, DEV_NAME);

  fd = open("/dev/net/tun", O_RDWR|O_NONBLOCK);
  if (fd < 0) {
    perror("Open failed\n");
    return 1;
  }
  err = ioctl(fd, TUNSETIFF, (void*)&ifr);
  if (err) {
    perror("Ioctl failed\n");
    close(fd);
    return 1;
  }

  /* Set ip/mac */
  system("ip link set dev test_tun address aa:aa:aa:aa:aa:bb");
  system("ip addr add 172.20.20.187/24 dev test_tun");
  system("ip link set dev test_tun up");

  while(1) {
    int ret, n;
    uint16_t plength;
    fd_set rd_set;

    FD_ZERO(&rd_set);
    FD_SET(fd, &rd_set);
    FD_SET(fd, &rd_set);

    ret = select(fd+1, &rd_set, NULL, NULL, NULL);
    if (ret < 0 && errno == EINTR) {
      continue;
    }
    if (ret < 0) {
      perror("select()");
      exit(1);
    }

    char *buf = malloc(sizeof(char) * 1024);
    if (FD_ISSET(fd, &rd_set)) {
      memset(buf, 0, sizeof(buf));
      if(n = read(fd, buf, sizeof(buf)) > 0)
	printf("data:%s\n", buf);
      plength = htons(n);
    }
    if (FD_ISSET(fd, &rd_set)) {
      memset(buf, 0, sizeof(buf));
      if(n = read(fd, buf, sizeof(plength)) > 0)
	printf("data:%s\n", buf);
    }
    free(buf);
  }

  if(setns(fd, 0)) {
    perror("Setns\n");
    exit(1);
  }
  return 0;
}
