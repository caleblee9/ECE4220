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
#include <wiringPi.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#define CHAR_DEV "/dev/Final" // "/dev/YourDevName"
#define BUTTON1 27	//define ports
#define BUTTON2 0
#define SW1 26
#define SW2 23


int events[4] = {0};
struct timeval times[4];
struct timeval noT;

void printStatus(char *, struct timeval);
void *rt_Event(void *);
void *printEvents(void *);
char *getIP();
int cdev_id;
static char buffer[40];

char list[64];


int main() {

	wiringPiSetup();
	list[0] = ' ';	
	if((cdev_id = open(CHAR_DEV, O_RDONLY)) == -1) {
		printf("Cannot open device %s\n", CHAR_DEV);
		exit(1);
	}

	pthread_t t1, t2;
	pthread_create(&t1, NULL, rt_Event, NULL);
	pthread_create(&t2, NULL, printEvents, NULL);

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
				events[0] = 1;
				gettimeofday(&times[0]);
			} else if(strcmp(buffer, "S2") == 0) {
				events[1] = 1;
				gettimeofday(&times[1]);
			} else if(strcmp(buffer, "B1") == 0) {
				events[2] = 1;
				gettimeofday(&times[2]);
			} else if(strcmp(buffer, "B2") == 0) {
				events[3] = 1;
				gettimeofday(&times[0]);
			} 
		}
	}
	pthread_exit(0);		
}
void *printEvents(void *ptr) {
	int i;
	uint64_t num_periods = 0;
	int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
	struct itimerspec itval;
	itval.it_interval.tv_sec = 1;
	itval.it_interval.tv_nsec = 0;
	itval.it_value.tv_sec = 0;			//set period to 75 ms and start time to 50 ms 
	itval.it_value.tv_nsec = 1000000;
	timerfd_settime(timer_fd, 0, &itval, NULL);
	read(timer_fd, &num_periods, sizeof(num_periods));
	while(1){
		read(timer_fd, &num_periods, sizeof(num_periods));
		for(i = 0; i < 4; i++) {
			if(events[i] == 1) {
				switch(i) {
					case 0:
						printStatus("Switch 1", times[0]);
						break;
					case 1:
						printStatus("Switch 2", times[1]);
						break;
					case 2:
						printStatus("Button 1", times[2]);
						break;
					case 3:
						printStatus("Button 2", times[3]);
						break;			
				}
			}
			events[i] = 0;
		}
		gettimeofday(&noT);
		printf("STATUS\n");
		printf("Time: %ld.%06ld\n", noT.tv_sec, noT.tv_usec);
		printf("From: %s\n", getIP());		//for testing purposes
		printf("Button 1: %d\n", digitalRead(BUTTON1));
		printf("Button 2: %d\n", digitalRead(BUTTON2));
		printf("Switch 1: %d\n", digitalRead(SW1));
		printf("Switch 2: %d\n", digitalRead(SW2));
		if(num_periods > 1) {
			puts("MISSED WINDOWN\n");
			exit(1);
		}	
	}
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
void printStatus(char *pin, struct timeval eT) {
	printf("%s event detected------------------------------------\n", pin);
	printf("Time: %ld.%06ld\n", eT.tv_sec, eT.tv_usec);
	printf("From: %s\n", getIP());		//for testing purposes
	printf("Button 1: %d\n", digitalRead(BUTTON1));
	printf("Button 2: %d\n", digitalRead(BUTTON2));
	printf("Switch 1: %d\n", digitalRead(SW1));
	printf("Switch 2: %d\n", digitalRead(SW2));
}

