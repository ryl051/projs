#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <signal.h>

#define PKT_SIZE 64 // Size of our ICMP packet (64 bytes)
#define TIMEOUT_SEC 5 // Timeout in seconds 

int init_socket() {
    int s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (s < 0) {
        perror("socket");
        fprintf(stderr, "You need to be root to create sockets");
        return -1;
    }

    struct timeval tv_out;
    tv_out.tv_sec = TIMEOUT_SEC;
    tv_out.tv_usec = 0;

    // set a receive timeout to prevent indefinite blocking
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
    return s;
}

/*
┌─────────────┬─────────────┬─────────────┬─────────────┐
│   Type      │    Code     │   Checksum  │             │
│   (8 bits)  │   (8 bits)  │  (16 bits)  │             │
├─────────────┼─────────────┼─────────────┼─────────────┤
│   Identifier│  Sequence   │             │             │
│  (16 bits)  │  (16 bits)  │             │             │
└─────────────┴─────────────┴─────────────┴─────────────┘
*/
void build_packet(char* sendbuf, int seq) {
    memset(sendbuf, 0, PKT_SIZE);
    struct icmp* icmp_packet = (struct icmp*)sendbuf;

    icmp_packet->icmp_type = ICMP_ECHO;
    icmp_packet->icmp_code = 0;
    icmp_packet->icmp_id = getpid() & 0xffff;
    icmp_packet->icmp_seq = seq;

    memset(sendbuf + sizeof(struct icmphdr), 0x42, PKT_SIZE - sizeof(struct icmphdr));

    icmp_packet->icmp_cksum = 0;
    // icmp_packet->icmp_cksum = in_
}

int main() {
    return -1;
}

