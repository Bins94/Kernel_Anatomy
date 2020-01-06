#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>

#define MAX_BUFFER 1024
#define PORT 20002
#define IPV6_REMOTE "fe80::5054:ff:fe12:3456"

int main(void) {
  int csock;
  int ret;
  struct sockaddr_in6 saddr;
  char *buf = malloc(MAX_BUFFER);

  csock = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
  if(csock == -1) {
    perror("socket()");
    exit(1);
  }

  saddr.sin6_family = AF_INET6;
  saddr.sin6_port = htons(PORT);
  inet_pton(AF_INET6, IPV6_REMOTE, &(saddr.sin6_addr));
  saddr.sin6_flowinfo = htonl(0);
  saddr.sin6_scope_id = if_nametoindex("br0");

  ret = connect(csock, (struct sockaddr*)&saddr, sizeof(saddr));
  if(ret == -1) {
    perror("connect()");
    close(csock);
    exit(1);
  }
  printf("Connected!\n");
  memset(buf, 0, sizeof(buf));
  buf = "Hello world\n";
  printf("Send out msg\n");
  ret = sendto(csock, buf, sizeof(buf), MSG_DONTWAIT, (struct sockaddr*)&saddr, sizeof(saddr));
  if(!ret) {
    perror("sendto failed\n");
    close(csock);
    exit(1);
  }

  printf("Recv msg\n");
  recvfrom(csock, buf, sizeof(buf), MSG_DONTWAIT, (struct sockaddr*)&saddr, sizeof(saddr));
  printf(buf);

  close(csock);
  return 0;
}
