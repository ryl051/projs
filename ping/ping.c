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

unsigned short calculate_cksum(unsigned short* ptr, int nbytes) {
    long sum = 0;
    unsigned short oddbyte;
    unsigned short answer;

    // Sum 16-bit words
    while (nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }

    // If there's a leftover byte (odd length), add it
    if (nbytes == 1) {
        oddbyte = 0;
        *((unsigned char*)&oddbyte) = *(unsigned char*)ptr;
        sum += oddbyte;
    }

    // Fold 32-bit sum into 16 bits
    sum = (sum >> 16) + (sum & 0xffff); // Add carry
    sum += (sum >> 16);                 // Add carry again just in case
    answer = (unsigned short)~sum;      // One's complement
    
    return answer;
}

/*
1. build socket
2. prep packet
3. checksum
4. send / receive
*/
int main() {
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("Socket creation failed. Are you root?");
        return 1;
    }

    char buf[1024];
    memset(buf, 0, sizeof(buf));
    struct icmp* icmp_packet = (struct icmp*) buf;

    // fill in icmp header
    icmp_packet->icmp_type = ICMP_ECHO;
    icmp_packet->icmp_code = 0;
    icmp_packet->icmp_id = htons(1234);
    icmp_packet->icmp_seq = htons(1);
    icmp_packet->icmp_cksum = 0;
    icmp_packet->icmp_cksum = calculate_cksum((unsigned short*)buf, sizeof(buf));

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "8.8.8.8", &dest_addr.sin_addr);
    int bytes = sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));

    char dest_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &dest_addr.sin_addr, dest_ip, sizeof(dest_ip));
    printf("Sent %d bytes to %s...\n", bytes, dest_ip);

    char recvbuf[1024];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    ssize_t bytes_received = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr*)&from_addr, &from_len);   
	struct ip* iph = (struct ip*) recvbuf;
	int ip_hdr_len = iph->ip_hl * 4;

	struct icmp* icmp_reply = (struct icmp*)(recvbuf + ip_hdr_len);
	if (icmp_reply->icmp_type == ICMP_ECHOREPLY) {
        char from_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &from_addr.sin_addr, from_ip, sizeof(from_ip));
        printf("Received a response from %s (%zd bytes)\n", from_ip, bytes_received);
    } else {
        printf("Unexpected ICMP type: %d\n", icmp_reply->icmp_type);
    }
}
