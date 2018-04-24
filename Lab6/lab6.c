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
#include <fcntl.h>

#define CHAR_DEV "/dev/Lab6" // "/dev/YourDevName"
	
char *getIP();
typedef struct master {
	char *ip; //client ip
	int num; //rand number generated
	char *lastVal; //of IP
} Master;
int main(int argc, char *argv[]) {
	if(argc < 2) {
		printf("usage: %s port\n", argv[0]);
		return 0;
	}
	srand(time(NULL));
	int rnd;
	Master m;	//to hold master information
	m.ip = malloc(sizeof(char) * 32);

	m.num = 0;	//set master number to 0
	struct in_addr ip;
	int mNum; //store parsed values
	char *fC; //to check for "#"
	char *lastVal; //received last val of ip
	char *tmp = malloc(sizeof(char) * 40);
	int i;
	int n;
	char buffer[40]; //message with size of 40
	int cdev_id;
	if((cdev_id = open(CHAR_DEV, O_WRONLY)) == -1) {
		printf("Cannot open device %s\n", CHAR_DEV);
		exit(1);
	}

	socklen_t fromlen;
	int sock = socket(AF_INET, SOCK_DGRAM, 0); //create the socket
	if(sock < 0) {
		printf("Error opening socket\n");
		exit(-1);
				printf("%s", m.lastVal);
	}
	struct sockaddr_in server;
	struct sockaddr_in addr;
	bzero(&server, sizeof(server)); //sets all values of server to 0
	server.sin_family = AF_INET; //Internet domain
	server.sin_addr.s_addr = INADDR_ANY;	//IP address on running machine
	server.sin_port = htons(atoi(argv[1]));	// get the port number from the user args
	

	if(bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0) { //binds socket to IP address of host and the user entered port number
		printf("error binding socket\n");
		exit(-1);
	}
	
	int broadEnable = 1;
	int ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadEnable, sizeof(broadEnable)); //set socket to allow broadcast
	if(ret < 0 ) {
		printf("Error setting socket to broadcast\n");
		exit(-1);
	}
	fromlen = sizeof(struct sockaddr_in); //size of socket structure

	while(1) {
		bzero(buffer, 40); //refresh buffer
		n = recvfrom(sock, buffer, 40, 0, (struct sockaddr *)&addr, &fromlen); //receive messages from clients
		if (n < 0) {
			printf("Receiving error\n");
			exit(-1);
		}
		if( addr.sin_addr.s_addr == inet_addr(getIP()) && buffer[0] != '#') {	//to avoid infinite loop
			continue;
		}
		strcpy(tmp, buffer);		
		fC = strtok(tmp, " ");
		if(strcmp(buffer, "WHOIS\n") == 0)  {
			if(m.num != 0 && strcmp(m.ip, getIP()) == 0) {	//if client enters whois, let them know if we are the master
				bzero(buffer, 40);
				addr.sin_addr.s_addr = inet_addr("128.206.19.255");
				sprintf(buffer, "Master: Caleb on board %s\n", getIP());
				n = sendto(sock, buffer, 40, 0, (struct sockaddr *)&addr, fromlen);
				if (n < 0) {
					printf("send error\n");
					exit(1);

				}
			}
			continue;
		} else if (strcmp(buffer, "VOTE\n") == 0) {
			bzero(buffer, 40);
			m.num = 0;
			rnd = (rand() % 10 + 1);	//generate rnd number
			sprintf(buffer, "# %s %d\n", getIP(), rnd);	
			addr.sin_addr.s_addr = inet_addr("128.206.19.255"); 	//set IP to broadcast (.255)
			n = sendto(sock, buffer, 40, 0, (struct sockaddr *)&addr, fromlen);
			if (n < 0 ) {
				printf("Error sento\n");
				exit(1);
			}
			continue;
		} else if(strcmp(fC, "#") == 0) {
			ip.s_addr = inet_addr(strtok(NULL, " "));
			mNum = atoi(strtok(NULL, "\n"));
			if(mNum > m.num) {
				m.num = mNum;
				strcpy(m.ip, inet_ntoa(ip));	//checks to see if we are the new master
			} else if(mNum == m.num) {
				if(ip.s_addr > inet_addr(m.ip)) {
					strcpy(m.ip, inet_ntoa(ip));
				}	
			}	
		} else if (buffer[0] == '@') { //if we receive a note
			printf("Note received\n");
			if(write(cdev_id, buffer, sizeof(buffer)) != sizeof(buffer)) {	//send note through char dev
				printf("Error in sending char device\n");
				exit(1);
			}
			if(m.num != 0 && strcmp(m.ip, getIP()) == 0) {	//if we are master
	
				addr.sin_addr.s_addr = inet_addr("128.206.19.255");
				n = sendto(sock, buffer, 40, 0, (struct sockaddr *)&addr, fromlen);	//send note to slaves
				if(n < 0) {
					printf("error in sending note\n");
					exit(1);
				}
			}
		}
	}
	free(tmp);
	free(m.ip);
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
