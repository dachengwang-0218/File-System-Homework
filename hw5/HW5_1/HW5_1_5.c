#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<sys/socket.h>
#include<sys/ioctl.h>
#include<sys/types.h>

#include<net/if.h>
#include<arpa/inet.h>
#include<netinet/in.h>

#include<linux/if_packet.h>
#include<linux/if_ether.h>
#include<linux/ip.h>
#include<linux/udp.h>
#include<linux/tcp.h>

#define BUFFER_SIZE 65536
#define MODIFY_TEXT "MODIFIED_BY_HW5_1_5"

struct pseudo_header{
    unsigned int src_addr;
    unsigned int dst_addr;
    unsigned char zero;
    unsigned char protocol;
    unsigned short length;
};

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

unsigned short transport_checksum(struct iphdr *ip,
                                  unsigned char *transport_header,
                                  int transport_len,
                                  int protocol){
    char temp[BUFFER_SIZE];
    struct pseudo_header psh;

    memset(temp, 0, BUFFER_SIZE);

    psh.src_addr = ip->saddr;
    psh.dst_addr = ip->daddr;
    psh.zero = 0;
    psh.protocol = protocol;
    psh.length = htons(transport_len);

    memcpy(temp, &psh, sizeof(struct pseudo_header));
    memcpy(temp + sizeof(struct pseudo_header),
           transport_header,
           transport_len);

    return checksum((unsigned short *)temp,
                    sizeof(struct pseudo_header) + transport_len);
}

void modify_payload_same_length(unsigned char *payload, int payload_len){
    int modify_len = strlen(MODIFY_TEXT);

    memset(payload, ' ', payload_len);

    if(modify_len > payload_len){
        modify_len = payload_len;
    }

    memcpy(payload, MODIFY_TEXT, modify_len);
}

void print_ip(unsigned int ip_addr){
    struct in_addr addr;
    addr.s_addr = ip_addr;
    printf("%s", inet_ntoa(addr));
}

int main(int argc, char *argv[]){
    if(argc != 2){
        printf("Usage: sudo %s <interface>\n", argv[0]);
        printf("Example: sudo %s enp0s3\n", argv[0]);
        return 1;
    }

    int target_port = 9090;
    char *interface = argv[1];

    int socket_fd;
    unsigned char buffer[BUFFER_SIZE];

    struct ifreq ifr;
    struct sockaddr_ll sll;

    socket_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(socket_fd < 0){
        perror("socket Fail.");
        return 1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);

    if(ioctl(socket_fd, SIOCGIFINDEX, &ifr) < 0){
        perror("ioctl SIOCGIFINDEX Fail.");
        close(socket_fd);
        return 1;
    }

    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_protocol = htons(ETH_P_ALL);

    if(bind(socket_fd, (struct sockaddr *)&sll, sizeof(sll)) < 0){
        perror("bind Fail.");
        close(socket_fd);
        return 1;
    }

    printf("Modifier is running on interface: %s\n", interface);
    printf("Payload will be changed to: %s\n\n", MODIFY_TEXT);

    while(1){
        struct sockaddr_ll recv_addr;
        socklen_t recv_addr_len = sizeof(recv_addr);

        ssize_t bytes = recvfrom(socket_fd,
                                 buffer,
                                 BUFFER_SIZE,
                                 0,
                                 (struct sockaddr *)&recv_addr,
                                 &recv_addr_len);

        if(bytes < 0){
            perror("recvfrom Fail.");
            break;
        }

        if(recv_addr.sll_pkttype == PACKET_OUTGOING){
            continue;
        }

        if(bytes < sizeof(struct ethhdr)){
            continue;
        }

        struct ethhdr *eth = (struct ethhdr *)buffer;
        unsigned short eth_type = ntohs(eth->h_proto);

        if(eth_type != ETH_P_IP){
            continue;
        }

        if(bytes < sizeof(struct ethhdr) + sizeof(struct iphdr)){
            continue;
        }

        struct iphdr *ip = (struct iphdr *)(buffer + sizeof(struct ethhdr));
        int ip_hdr_len = ip->ihl * 4;

        if(bytes < sizeof(struct ethhdr) + ip_hdr_len){
            continue;
        }

        if(ntohs(ip->frag_off) & 0x1fff){
            continue;
        }

        int ip_total_len = ntohs(ip->tot_len);

        if(bytes < sizeof(struct ethhdr) + ip_total_len){
            continue;
        }

        if(ip->protocol == IPPROTO_UDP){
            if(ip_total_len < ip_hdr_len + sizeof(struct udphdr)){
                continue;
            }

            struct udphdr *udp =
                (struct udphdr *)(buffer + sizeof(struct ethhdr) + ip_hdr_len);

            int udp_hdr_len = sizeof(struct udphdr);
            int udp_total_len = ntohs(udp->len);
            int payload_len = udp_total_len - udp_hdr_len;

            if(payload_len <= 0){
                continue;
            }

            if(ntohs(udp->dest) != target_port){
                continue;
            }

            unsigned char *payload =
                buffer + sizeof(struct ethhdr) + ip_hdr_len + udp_hdr_len;

            printf("UDP packet captured: ");
            print_ip(ip->saddr);
            printf(":%u -> ", ntohs(udp->source));
            print_ip(ip->daddr);
            printf(":%u\n", ntohs(udp->dest));

            printf("Original payload length: %d\n", payload_len);

            modify_payload_same_length(payload, payload_len);

            ip->check = 0;
            ip->check = checksum((unsigned short *)ip, ip_hdr_len);

            if(udp->check != 0){
                udp->check = 0;
                udp->check = transport_checksum(ip,
                                                (unsigned char *)udp,
                                                udp_total_len,
                                                IPPROTO_UDP);
            }

            ssize_t sent = sendto(socket_fd,
                                  buffer,
                                  sizeof(struct ethhdr) + ip_total_len,
                                  0,
                                  (struct sockaddr *)&sll,
                                  sizeof(sll));

            if(sent < 0){
                perror("sendto UDP Fail.");
            }else{
                printf("Modified UDP packet sent.\n\n");
            }
        }
        else if(ip->protocol == IPPROTO_TCP){
            if(ip_total_len < ip_hdr_len + sizeof(struct tcphdr)){
                continue;
            }

            struct tcphdr *tcp =
                (struct tcphdr *)(buffer + sizeof(struct ethhdr) + ip_hdr_len);

            int tcp_hdr_len = tcp->doff * 4;

            if(tcp_hdr_len < sizeof(struct tcphdr)){
                continue;
            }

            if(ip_total_len < ip_hdr_len + tcp_hdr_len){
                continue;
            }

            int tcp_total_len = ip_total_len - ip_hdr_len;
            int payload_len = tcp_total_len - tcp_hdr_len;

            if(payload_len <= 0){
                continue;
            }

            if(ntohs(tcp->dest) != target_port){
                continue;
            }

            unsigned char *payload =
                buffer + sizeof(struct ethhdr) + ip_hdr_len + tcp_hdr_len;

            printf("TCP packet captured: ");
            print_ip(ip->saddr);
            printf(":%u -> ", ntohs(tcp->source));
            print_ip(ip->daddr);
            printf(":%u\n", ntohs(tcp->dest));

            printf("Original payload length: %d\n", payload_len);

            modify_payload_same_length(payload, payload_len);

            ip->check = 0;
            ip->check = checksum((unsigned short *)ip, ip_hdr_len);

            tcp->check = 0;
            tcp->check = transport_checksum(ip,
                                            (unsigned char *)tcp,
                                            tcp_total_len,
                                            IPPROTO_TCP);

            ssize_t sent = sendto(socket_fd,
                                  buffer,
                                  sizeof(struct ethhdr) + ip_total_len,
                                  0,
                                  (struct sockaddr *)&sll,
                                  sizeof(sll));

            if(sent < 0){
                perror("sendto TCP Fail.");
            }else{
                printf("Modified TCP packet sent.\n\n");
            }
        }
    }

    close(socket_fd);
    return 0;
}
