#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <wiringPi.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#define CHAR_DEV "/dev/Final" // "/dev/YourDevName"
#define BUTTON1 27	//define ports
#define BUTTON2 0
#define SW1 26
#define SW2 23
#define RED 8		//RED LED wiringPi
#define YELLOW 9	//YELLOW LED wiringPi
#define GREEN 7	

int lNum = 0; //holds place for list
struct timeval eT;	//event time
struct timeval noT;	//status time
static char buffer[40];	//character device buffer
char list[16][40] = {"\0\0"};//holds events that occured within interval
char *port;
char *portfield;
void error(const char *);
void printStatus();
void *rt_Event(void *);
void *sendEvents(void *);
char *getIP();
int cdev_id;
void format(char *);

int main(int argc, char *argv[]) {
	if (argc != 3)
   	{
	   	printf("usage %s hostname port\n", argv[0]);
       		exit(1);
  	}
	port = argv[1];
	portfield = argv[2];
/*
-----------------------------------------------------------------------
----------------------------EVENTS SETUP-------------------------------
-----------------------------------------------------------------------
*/
	wiringPiSetup();
	if((cdev_id = open(CHAR_DEV, O_RDONLY)) == -1) {
		printf("Cannot open device %s\n", CHAR_DEV);
		exit(1);
	}

	pthread_t t1, t2;
	pthread_create(&t1, NULL, rt_Event, NULL);
	pthread_create(&t2, NULL, sendEvents, NULL);

	pthread_join(t1, 0);
	pthread_join(t2, 0);
	return 0;
}
void *rt_Event(void *ptr) {
	while(1) {
		bzero(buffer, 40);
		read(cdev_id, buffer, sizeof(buffer));
		if(buffer[0] != '\0') {
			if(strcmp(buffer, "S1") == 0) {
				format("SW1");
			} else if(strcmp(buffer, "S2") == 0) {
				format("SW2");
			} else if(strcmp(buffer, "B1") == 0) {
				format("BT1");
			} else if(strcmp(buffer, "B2") == 0) {
				format("BT2");
			} 
		}
	}
	pthread_exit(0);		
}
void *sendEvents(void *ptr) {
	int sock, n;
   	unsigned int length;
   	struct sockaddr_in server, from;
   	struct hostent *hp;
   	char msg[40];

   	sock = socket(AF_INET, SOCK_DGRAM, 0); // Creates socket. Connectionless.
   	if (sock < 0)
	   	error("socket");

   	server.sin_family = AF_INET;		// symbol constant for Internet domain
   	hp = gethostbyname(port);		// converts hostname input
   	if (hp == 0)
		error("Unknown host");

  	bcopy((char *)hp->h_addr, (char *)&server.sin_addr, hp->h_length);
   	server.sin_port = htons(atoi(portfield));	// port field
   	length = sizeof(struct sockaddr_in);		// size of structure

	int i;
	uint64_t num_periods = 0;
	int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
	struct itimerspec itval;
	itval.it_interval.tv_sec = 1;
	itval.it_interval.tv_nsec = 0;
	itval.it_value.tv_sec = 0;			//set period to 1ms and start time to 50 ms 
	itval.it_value.tv_nsec = 1000000;
	timerfd_settime(timer_fd, 0, &itval, NULL);
	read(timer_fd, &num_periods, sizeof(num_periods));
	while(1){
		read(timer_fd, &num_periods, sizeof(num_periods));
		i = 0;
		while(strcmp(list[i], "\0\0") != 0) {
			bzero(msg, 40); //refresh buffer
			strcpy(msg, list[i]);
			printf("%s", msg);
			n = sendto(sock, msg, strlen(msg), 0, (const struct sockaddr *)&server,length);
   			if (n < 0)
	   			error("Sendto");
			bzero(list[i], 40);
			i++;
		}
		printf("\nSTATUS\n");
		printStatus();
		lNum = 0;
		if(num_periods > 1) {
			puts("MISSED WINDOWN\n");
			exit(1);
		}	
	}
	
   	close(sock);
	pthread_exit(0);
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

void printStatus() {
	printf("From: %s\n", getIP());
	printf("Button 1: %d\n", digitalRead(BUTTON1));
	printf("Button 2: %d\n", digitalRead(BUTTON2));
	printf("Switch 1: %d\n", digitalRead(SW1));
	printf("Switch 2: %d\n", digitalRead(SW2));			//debugging
	printf("RED:    %d\n", digitalRead(RED));
	printf("YELLOW: %d\n", digitalRead(YELLOW));
	printf("GREEN:  %d\n\n", digitalRead(GREEN));
}

void format(char *event) {
	gettimeofday(&eT);
	sprintf(list[lNum++], "%s %ld.%06ld %d %d %d %d %d %d %d\n",event,eT.tv_sec, eT.tv_usec,digitalRead(BUTTON1),
	digitalRead(BUTTON2),digitalRead(SW1),digitalRead(SW2),digitalRead(RED),digitalRead(YELLOW),digitalRead(GREEN));
}
void error(const char *msg)
{
    perror(msg);
    exit(0);
}
