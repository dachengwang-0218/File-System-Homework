#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>

#include<linux/ip.h>
#include<linux/udp.h>

#define BUFFER_SIZE 4096

unsigned short checksum(unsigned short *buf, int len){
    unsigned long sum = 0;
    
    while(len > 1){
        sum += *buf++;
        len -= 2;
    }

    if(len == 1){
        sum += *(unsigned char *)buf;
    }

    while(sum >> 16){
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return (unsigned short)(~sum);
}

int main(int argc, char *argv[]){
    char *receiver_ip = argv[1];
    int receiver_port = atoi(argv[2]);
    char *spoofed_source_ip = argv[3];

    int sockfd;
    char packet[BUFFER_SIZE];
    memset(packet, 0, sizeof(packet));

    char *message = "oooi, shia mi shi i";
    int ip_hdr_len = sizeof(struct iphdr);
    int udp_hdr_len = sizeof(struct udphdr);
    int data_len = strlen(message);
    int packet_len = ip_hdr_len + udp_hdr_len + data_len;

    struct iphdr *ip = (struct iphdr *)packet;
    struct udphdr *udp = (struct udphdr *)(packet + ip_hdr_len);
    char *data = packet + ip_hdr_len + udp_hdr_len;
    strcpy(data, message);

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if(sockfd < 0){
        perror("socket Fail.");
        return 1;
    }
    int opt = 1;
    if(setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)) < 0){
        perror("setsockopt IP_HDRINCL Fail.");
        close(sockfd);
        return 1;
    }

    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->tot_len = htons(packet_len);
    ip->id = htons(22222);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_UDP;
    ip->saddr = inet_addr(spoofed_source_ip);
    ip->daddr = inet_addr(receiver_ip);
    ip->check = 0;
    ip->check = checksum((unsigned short *)ip, ip_hdr_len);

    udp->source = htons(22222);
    udp->dest = htons(receiver_port);
    udp->len = htons(udp_hdr_len + data_len);
    udp->check = 0;
    
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(receiver_port);
    dest_addr.sin_addr.s_addr = inet_addr(receiver_ip);

    if(sendto(sockfd, packet, packet_len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0){
        perror("sendto Fail.");
        close(sockfd);
        return 1;
    }

    printf("封包送出成功.\n");
    printf("Spoofed Source IP:\t%s\n", spoofed_source_ip);
    printf("Destination IP:\t%s\n", receiver_ip);
    printf("Destination Port:\t%d\n", receiver_port);
    printf("Message:\t%s\n", message);
    
    close(sockfd);
    return 0;
}