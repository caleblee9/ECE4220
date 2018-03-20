#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
char *getIP(); 
int main(int argc, char *argv[]) {
	int n;
	char buffer[40];
	socklen_t fromlen;
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0) {
		printf("Error opening socket\n");
		exit(-1);
	}
	struct sockaddr_in server;
	struct sockaddr_in addr;
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(atoi(argv[1]));
	

	if(bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
		printf("error binding socket\n");
		exit(-1);
	}
	
	int broadEnable = 1;
	int ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadEnable, sizeof(broadEnable));
	if(ret < 0 ) {
		printf("Error setting socket to broadcast\n");
		exit(-1);
	}
	fromlen = sizeof(struct sockaddr_in);

	while(1) {
		bzero(buffer, 40);
		n = recvfrom(sock, buffer, 40, 0, (struct sockaddr *)&addr, &fromlen);
		if (n < 0) {
			printf("Receiving error\n");
			exit(-1);
		}
		printf("Received message: %s", buffer);
		addr.sin_addr.s_addr = inet_addr("192.168.1.255");

	}
        return 0;
}
char *getIP() {	
	int n;
    	struct ifreq ifr;
	char array[] = "wlan0";
	n = socket(AF_INET, SOCK_DGRAM, 0);
    	//Type of address to retrieve - IPv4 IP address
    	ifr.ifr_addr.sa_family = AF_INET;
    	//Copy the interface name in the ifreq structure
    	strncpy(ifr.ifr_name , array , IFNAMSIZ - 1);
    	ioctl(n, SIOCGIFADDR, &ifr);
    	close(n);
    	//display result
    return inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr);

}
