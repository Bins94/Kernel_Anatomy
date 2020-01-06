#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>

#define MAX_BUFFER 1024
#define PORT 20002 

int main(void) {
  int ssock, csock;
  int ret;
  struct sctp_initmsg initmsg;
  struct sockaddr_in6 saddr;
  struct msghdr msg;

  ssock = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
  if (ssock == -1) {
    perror("socket()");
    exit(1);
  }

  saddr.sin6_family = AF_INET6;
  saddr.sin6_addr = in6addr_any;
  saddr.sin6_port = htons(PORT);

  ret = bind(ssock, (struct sockaddr*)&saddr, sizeof(saddr));
  if (ret == -1) {
    perror("bind()");
    close(ssock);
    exit(1);
  }

  initmsg.sinit_num_ostreams = 5;
  initmsg.sinit_max_instreams = 5;
  initmsg.sinit_max_attempts = 4;
  ret = setsockopt(ssock, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg));
  if (ret == -1) {
    perror("setsockopt()");
    close(ssock);
    exit(1);
  }

  ret = listen(ssock, 5);
  if (ret == -1) {
    perror("listen()");
    close(ssock);
    exit(1);
  }
  
  while (1) {
    csock = accept(ssock, (struct sockaddr*)NULL, (int*)NULL);
    if(csock == -1) {
      perror("accept()");
      close(csock);
      exit(1);
    } else {
      printf("Connect!\n");
      recvmsg(csock, &msg, MSG_DONTWAIT);
      sendmsg(csock, &msg, MSG_DONTWAIT|MSG_DONTROUTE);
      close(csock);
    }
  }
  return 0;
}
