#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<sys/socket.h>
#include<sys/ioctl.h>
#include<sys/types.h>

#include<net/if.h>
#include<arpa/inet.h>

#include<linux/if_packet.h>
#include<linux/if_ether.h>
#include<linux/ip.h>

#define PACKET_NUM 200
#define BUFFER_SIZE 65536

int main(int argc, char *argv[]){
    char *interface = argv[2];

    int socket_fd;
    unsigned char buffer[BUFFER_SIZE];

    int ipv4_count = 0, arp_count = 0, rarp_count = 0, eth_others_count = 0;
    int tcp_count = 0, udp_count = 0, icmp_count = 0, igmp_count = 0, ip_others_count = 0;

    struct ifreq ifr;
    struct ifreq old_ifr;
    
    socket_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(socket_fd < 0){
        perror("socket Fail.");
        return 1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);

    if(ioctl(socket_fd, SIOCGIFADDR, &ifr) < 0){
        perror("ioctl SIOCGIFADDR Fail.");
    }else{
        struct sockaddr_in *ip_addr = (struct sockaddr_in *)&ifr.ifr_addr;
        printf("My current non-loopback IPv4 address(es): %s\n", inet_ntoa(ip_addr->sin_addr));
    }

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

    printf("bind to interface: %s\n\n", interface);

    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));

    if(ioctl(socket_fd, SIOCGIFINDEX, &ifr) < 0){
        perror("ioctl SIOCGIFINDEX Fail.");
        ioctl(socket_fd, SIOCSIFFLAGS, &old_ifr);
        close(socket_fd);
        return 1;
    }

    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_protocol = htons(ETH_P_ALL);

    if(bind(socket_fd, (struct sockaddr *)&sll, sizeof(sll)) < 0){
        perror("bind Fail.");
        ioctl(socket_fd, SIOCSIFFLAGS, &old_ifr);
        close(socket_fd);
        return 1;
    }

    for(int i = 0 ; i < PACKET_NUM ; i++){
        ssize_t bytes = recvfrom(socket_fd, buffer, BUFFER_SIZE, 0, NULL, NULL);
        if(bytes < 0){
            perror("recvfrom Fail.");
            break;
        }

        if(bytes < sizeof(struct ethhdr)){
            continue;
        }

        struct ethhdr *eth = (struct ethhdr *)buffer;
        unsigned short eth_type = ntohs(eth->h_proto);

        if(eth_type == ETH_P_IP){
            ipv4_count++;
            struct iphdr *ip = (struct iphdr *)(buffer + sizeof(struct ethhdr));

            switch (ip->protocol){
                case IPPROTO_TCP:
                    tcp_count++;
                    break;

                case IPPROTO_UDP:
                    udp_count++;
                    break;
                
                case IPPROTO_ICMP:
                    icmp_count++;
                    break;
                
                case IPPROTO_IGMP:
                    igmp_count++;
                    break;
                
                default:
                    ip_others_count++;
                    break;
            }
        }else if(eth_type == ETH_P_ARP){
            arp_count++;
        }else if(eth_type == ETH_P_RARP){
            rarp_count++;
        }else{
            eth_others_count++;
        }
    }

    printf("----- statistics -----\n");
    printf("Total captured packets: %d\n\n", PACKET_NUM);

    printf("[Ethernet Layer]\n");
    printf("IPv4\t: %d\n", ipv4_count);
    printf("ARP\t: %d\n", arp_count);
    printf("RARP\t: %d\n", rarp_count);
    printf("Others\t: %d\n", eth_others_count);

    printf("[IPv4 Protocol]\n");
    printf("TCP\t: %d\n", tcp_count);
    printf("UDP\t: %d\n", udp_count);
    printf("ICMP\t: %d\n", icmp_count);
    printf("IGMP\t: %d\n", igmp_count);
    printf("Others\t: %d\n", ip_others_count);

    printf("----- finish -----\n");

    if(ioctl(socket_fd, SIOCSIFFLAGS, &old_ifr) < 0){
        perror("restore interface flags Fail.");
    }else{
        printf("interface flags restore on %s\n", interface);
    }

    close(socket_fd);

    return 0;
}