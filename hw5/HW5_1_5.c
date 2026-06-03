#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<stdint.h>

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
    uint32_t src_addr;
    uint32_t dst_addr;
    uint8_t zero;
    uint8_t protocol;
    uint16_t length;
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
    unsigned char temp[BUFFER_SIZE];
    struct pseudo_header psh;

    if((int)(sizeof(struct pseudo_header) + transport_len) > BUFFER_SIZE){
        return 0;
    }

    memset(temp, 0, BUFFER_SIZE);
    memset(&psh, 0, sizeof(psh));

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

void print_ip(uint32_t ip_addr){
    struct in_addr addr;
    addr.s_addr = ip_addr;
    printf("%s", inet_ntoa(addr));
}

void print_mac(unsigned char *mac){
    printf("%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);
}

int main(int argc, char *argv[]){
    if(argc != 3){
        printf("Usage: sudo %s <interface> <target_port>\n", argv[0]);
        printf("Example: sudo %s enp46s0 9090\n", argv[0]);
        return 1;
    }

    char *interface = argv[1];
    int target_port = atoi(argv[2]);

    int socket_fd;
    unsigned char buffer[BUFFER_SIZE];

    struct ifreq ifr;
    struct ifreq old_ifr;
    struct sockaddr_ll bind_addr;

    socket_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(socket_fd < 0){
        perror("socket Fail.");
        return 1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);

    if(ioctl(socket_fd, SIOCGIFFLAGS, &ifr) < 0){
        perror("ioctl SIOCGIFFLAGS Fail.");
        close(socket_fd);
        return 1;
    }

    old_ifr = ifr;
    ifr.ifr_flags |= IFF_PROMISC;

    if(ioctl(socket_fd, SIOCSIFFLAGS, &ifr) < 0){
        perror("ioctl SIOCSIFFLAGS Fail.");
        close(socket_fd);
        return 1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);

    if(ioctl(socket_fd, SIOCGIFINDEX, &ifr) < 0){
        perror("ioctl SIOCGIFINDEX Fail.");
        ioctl(socket_fd, SIOCSIFFLAGS, &old_ifr);
        close(socket_fd);
        return 1;
    }

    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sll_family = AF_PACKET;
    bind_addr.sll_ifindex = ifr.ifr_ifindex;
    bind_addr.sll_protocol = htons(ETH_P_ALL);

    if(bind(socket_fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0){
        perror("bind Fail.");
        ioctl(socket_fd, SIOCSIFFLAGS, &old_ifr);
        close(socket_fd);
        return 1;
    }

    printf("Modifier is running on interface: %s\n", interface);
    printf("Target port: %d\n", target_port);
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

        if(bytes < (ssize_t)sizeof(struct ethhdr)){
            continue;
        }

        struct ethhdr *eth = (struct ethhdr *)buffer;
        unsigned short eth_type = ntohs(eth->h_proto);

        if(eth_type != ETH_P_IP){
            continue;
        }

        if(bytes < (ssize_t)(sizeof(struct ethhdr) + sizeof(struct iphdr))){
            continue;
        }

        struct iphdr *ip = (struct iphdr *)(buffer + sizeof(struct ethhdr));
        int ip_hdr_len = ip->ihl * 4;

        if(ip->version != 4){
            continue;
        }

        if(ip_hdr_len < (int)sizeof(struct iphdr)){
            continue;
        }

        if(bytes < (ssize_t)(sizeof(struct ethhdr) + ip_hdr_len)){
            continue;
        }

        uint16_t frag = ntohs(ip->frag_off);
        if(frag & 0x3fff){
            continue;
        }

        int ip_total_len = ntohs(ip->tot_len);

        if(ip_total_len < ip_hdr_len){
            continue;
        }

        if(bytes < (ssize_t)(sizeof(struct ethhdr) + ip_total_len)){
            continue;
        }

        if(ip->protocol == IPPROTO_UDP){
            if(ip_total_len < ip_hdr_len + (int)sizeof(struct udphdr)){
                continue;
            }

            struct udphdr *udp =
                (struct udphdr *)(buffer + sizeof(struct ethhdr) + ip_hdr_len);

            if(ntohs(udp->dest) != target_port){
                continue;
            }

            int udp_hdr_len = sizeof(struct udphdr);
            int udp_total_len = ntohs(udp->len);

            if(udp_total_len < udp_hdr_len){
                continue;
            }

            if(ip_total_len < ip_hdr_len + udp_total_len){
                continue;
            }

            int payload_len = udp_total_len - udp_hdr_len;

            if(payload_len <= 0){
                continue;
            }

            unsigned char *payload =
                buffer + sizeof(struct ethhdr) + ip_hdr_len + udp_hdr_len;

            printf("UDP packet captured: ");
            print_ip(ip->saddr);
            printf(":%u -> ", ntohs(udp->source));
            print_ip(ip->daddr);
            printf(":%u\n", ntohs(udp->dest));

            printf("Ethernet: ");
            print_mac(eth->h_source);
            printf(" -> ");
            print_mac(eth->h_dest);
            printf("\n");

            printf("Original payload length: %d\n", payload_len);

            modify_payload_same_length(payload, payload_len);

            ip->check = 0;
            ip->check = checksum((unsigned short *)ip, ip_hdr_len);

            udp->check = 0;
            udp->check = transport_checksum(ip,
                                            (unsigned char *)udp,
                                            udp_total_len,
                                            IPPROTO_UDP);

            struct sockaddr_ll send_addr;
            memset(&send_addr, 0, sizeof(send_addr));

            send_addr.sll_family = AF_PACKET;
            send_addr.sll_ifindex = ifr.ifr_ifindex;
            send_addr.sll_halen = ETH_ALEN;
            memcpy(send_addr.sll_addr, eth->h_dest, ETH_ALEN);

            ssize_t sent = sendto(socket_fd,
                                  buffer,
                                  sizeof(struct ethhdr) + ip_total_len,
                                  0,
                                  (struct sockaddr *)&send_addr,
                                  sizeof(send_addr));

            if(sent < 0){
                perror("sendto UDP Fail.");
            }else{
                printf("Modified UDP packet sent. bytes = %ld\n\n", sent);
            }
        }
        else if(ip->protocol == IPPROTO_TCP){
            if(ip_total_len < ip_hdr_len + (int)sizeof(struct tcphdr)){
                continue;
            }

            struct tcphdr *tcp =
                (struct tcphdr *)(buffer + sizeof(struct ethhdr) + ip_hdr_len);

            if(ntohs(tcp->dest) != target_port){
                continue;
            }

            int tcp_hdr_len = tcp->doff * 4;

            if(tcp_hdr_len < (int)sizeof(struct tcphdr)){
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

            unsigned char *payload =
                buffer + sizeof(struct ethhdr) + ip_hdr_len + tcp_hdr_len;

            printf("TCP packet captured: ");
            print_ip(ip->saddr);
            printf(":%u -> ", ntohs(tcp->source));
            print_ip(ip->daddr);
            printf(":%u\n", ntohs(tcp->dest));

            printf("Ethernet: ");
            print_mac(eth->h_source);
            printf(" -> ");
            print_mac(eth->h_dest);
            printf("\n");

            printf("Original payload length: %d\n", payload_len);

            modify_payload_same_length(payload, payload_len);

            ip->check = 0;
            ip->check = checksum((unsigned short *)ip, ip_hdr_len);

            tcp->check = 0;
            tcp->check = transport_checksum(ip,
                                            (unsigned char *)tcp,
                                            tcp_total_len,
                                            IPPROTO_TCP);

            struct sockaddr_ll send_addr;
            memset(&send_addr, 0, sizeof(send_addr));

            send_addr.sll_family = AF_PACKET;
            send_addr.sll_ifindex = ifr.ifr_ifindex;
            send_addr.sll_halen = ETH_ALEN;
            memcpy(send_addr.sll_addr, eth->h_dest, ETH_ALEN);

            ssize_t sent = sendto(socket_fd,
                                  buffer,
                                  sizeof(struct ethhdr) + ip_total_len,
                                  0,
                                  (struct sockaddr *)&send_addr,
                                  sizeof(send_addr));

            if(sent < 0){
                perror("sendto TCP Fail.");
            }else{
                printf("Modified TCP packet sent. bytes = %ld\n\n", sent);
            }
        }
    }

    if(ioctl(socket_fd, SIOCSIFFLAGS, &old_ifr) < 0){
        perror("restore interface flags Fail.");
    }else{
        printf("interface flags restored on %s\n", interface);
    }

    close(socket_fd);
    return 0;
}
