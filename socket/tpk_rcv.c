#include <sys/mman.h>
#include <stdio.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <string.h>
#include <net/if.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <math.h>
#include <sys/time.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_packet.h>
#include <signal.h>
#include <poll.h>

unsigned int blocksiz = 1 << 22, framesiz = 1 << 11;
unsigned int blocknum = 64;

int main(void) {
	int sk, error, len, ret, vers = TPACKET_V3;
	struct tpacket_req3 a;
	uint8_t *buf;
	struct sockaddr src;
	struct pollfd pfd;

	a.tp_block_size = blocksiz;
	a.tp_frame_size = framesiz;
	a.tp_block_nr = blocknum;
	a.tp_frame_nr = (blocksiz * blocknum) / framesiz;
	a.tp_retire_blk_tov = 60;
	a.tp_feature_req_word = TP_FT_REQ_FILL_RXHASH;

	len = sizeof(struct sockaddr);
	sk = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (!sk)
                printf("Socket create failed!\n");

	ret = setsockopt(sk, SOL_PACKET, PACKET_VERSION, &vers, sizeof(vers));
	if (ret < 0) {
		perror("[0]setsockopt");
		exit(1);
	}

	ret = setsockopt(sk, SOL_PACKET, PACKET_RX_RING, &a, sizeof(struct tpacket_req3));
	if (ret < 0) {
		perror("[1]setsockopt");
		exit(1);
	}

	pfd.fd = sk;
	pfd.revents = 0;
	pfd.events = POLLIN|POLLERR;

	buf = mmap(NULL, a.tp_block_size*a.tp_block_nr, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, sk, 0);

	if (buf == MAP_FAILED) {
		printf("%p\n", buf);
		perror("mmap");
		exit(1);
	}

	while(1) {
	ret = poll(&pfd, 1, 1);
	}

	return 0;
}
