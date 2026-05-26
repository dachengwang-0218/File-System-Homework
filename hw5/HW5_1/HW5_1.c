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

#define PACKET_NUM 200
#define BUFFER_SIZE 65536

void print_mac(unsigned char *mac){
    printf("%02X:%02X:%02X%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void run_mode1(int socket_fd, unsigned char *buffer){
    int ipv4_count = 0, arp_count = 0, rarp_count = 0, eth_others_count = 0;
    int tcp_count = 0, udp_count = 0, icmp_count = 0, igmp_count = 0, ip_others_count = 0;

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
}

void run_mode2(int socket_fd, unsigned char *buffer, unsigned int my_ip){
    int udp_count = 0;

    printf("----- packets captured -----\n");

    while(udp_count < 5){
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

        //only accept IPv4
        if(eth_type != ETH_P_IP){
            continue;
        }

        //packet length must longer than ethernet header + ipv4 header
        if(bytes < sizeof(struct ethhdr) + sizeof(struct iphdr)){
            continue;
        }

        struct iphdr *ip = (struct iphdr *)(buffer + sizeof(struct ethhdr));

        //only accept UDP
        if(ip->protocol != IPPROTO_UDP){
            continue;
        }

        //the destination ip address msut be the interface ip
        if(ip->daddr != my_ip){
            continue;
        }

        int ip_header_len = ip->ihl * 4;
        //packet length must longer than ethernet header + ipv4 header + udp header
        if(bytes < sizeof(struct ethhdr) + ip_header_len + sizeof(struct udphdr)){
            continue;
        }

        struct udphdr *udp = (struct udphdr *)(buffer + sizeof(struct ethhdr) + ip_header_len);
        struct in_addr src_ip, dest_ip;
        src_ip.s_addr = ip->saddr;
        dest_ip.s_addr = ip->daddr;

        udp_count++;
        printf("packet %d:\n", udp_count);

        printf("Source MAC address: ");
        print_mac(eth->h_source);
        printf("Destination MAC address: ");
        print_mac(eth->h_dest);

        printf("IP->protocol\t= UDP\n");
        printf("IP->src_ip\t= %s\n", inet_ntoa(src_ip));
        printf("IP->dst_ip\t= %s\n", inet_ntoa(dest_ip));
        printf("Src_port\t= %u\n", ntohs(udp->source));
        printf("Dst_port\t= %u\n", ntohs(udp->dest));

        if(udp_count != 5){
            putchar('\n');
        }
    }
}

int main(int argc, char *argv[]){
    char *mode = argv[1];
    char *interface = argv[2];

    int socket_fd;
    unsigned char buffer[BUFFER_SIZE];

    struct ifreq ifr;
    struct ifreq old_ifr;
    struct in_addr my_ip;
    
    socket_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(socket_fd < 0){
        perror("socket Fail.");
        return 1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);

    if(ioctl(socket_fd, SIOCGIFADDR, &ifr) < 0){
        perror("ioctl SIOCGIFADDR Fail.");
        close(socket_fd);
        return 1;
    }else{
        struct sockaddr_in *ip_addr = (struct sockaddr_in *)&ifr.ifr_addr;
        my_ip = ip_addr->sin_addr;
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

    if(strcmp(mode, "mode1") == 0){
        run_mode1(socket_fd, buffer);
    }else if(strcmp(mode, "mode2") == 0){
        run_mode2(socket_fd, buffer, my_ip.s_addr);
    }

    if(ioctl(socket_fd, SIOCSIFFLAGS, &old_ifr) < 0){
        perror("restore interface flags Fail.");
    }else{
        printf("interface flags restore on %s\n", interface);
    }

    close(socket_fd);
    return 0;
}