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
#include <sys/mman.h>

#define CHAR_DEV "/dev/Final" // "/dev/YourDevName"
#define BUTTON1 27	//define ports
#define BUTTON2 0
#define SW1 26
#define SW2 23


struct timeval eT;
void *printStatus(void *);
void *rt_Event(void *);
void *getData(void *);
char *getIP();
int cdev_id;
static char buffer[40];
int *flag = 0;
int main() {

	flag = mmap(NULL, sizeof(*flag), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);	
	wiringPiSetup();
	
	pinMode(BUTTON1, INPUT);
	pinMode(BUTTON2, INPUT);
	pinMode(SW1, INPUT);
	pinMode(SW2, INPUT);

	pullUpDnControl(BUTTON1, PUD_DOWN);
	pullUpDnControl(BUTTON2, PUD_DOWN);



	int s1 = digitalRead(SW1);
	int s2 = digitalRead(SW2);	
	int N_pipe2[2];
	if(pipe(N_pipe2) < 0) {
		printf("N_pipe2 creation error\n");
		exit(-1);
	}					//pipe and fork to allow parent and child communication
	int a;
	if((a = fork()) < 0) {
		printf("Fork error\n");
		exit(-1);
	}	
	if (a != 0){ 	//child thread running getData i.e. receiving the events from the parent
		pthread_t thread0;
		pthread_create(&thread0, NULL, getData, N_pipe2);
		pthread_join(thread0, NULL);
	} else { //parent thread sending the data through pipe2 for child to process
		pthread_t thread1;
		pthread_create(&thread1, NULL, rt_Event, &N_pipe2);	//rt Event thread
		pthread_join(thread1, 0);
	}
	uint64_t num_periods = 0;
	int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
	struct itimerspec itval;
	itval.it_interval.tv_sec = 1;
	itval.it_interval.tv_nsec = 0;
	itval.it_value.tv_sec = 0;			//set period to 1s and start time to 10 ms 
	itval.it_value.tv_nsec = 5000000;
	timerfd_settime(timer_fd, 0, &itval, NULL);
	read(timer_fd, &num_periods, sizeof(num_periods));
	while(1) {
		*flag = 0;
		read(timer_fd, &num_periods, sizeof(num_periods));
		printf("FLAG: %d\n", *flag);
		*flag = 1;
		usleep(10000);
		if(num_periods > 1) {
			puts("MISSED WINDOWN\n");
			exit(1);
		}
	}
	return 0;
}
void *getData(void * ptr) {
	int *N_pipe2 = (int *) ptr;
	int num = 0;
	while(1) {	
		pthread_t dyn;
		int b;
		if(read(N_pipe2[0], buffer, sizeof(buffer)) < 0) {
				printf("N_pipe2 read error\n");	//if receive data through pipe
				exit(-1);
		} else {
			if((b = fork()) < 0) {
				printf("Fork error\n");	//create child
				exit(-1);
			}
			if(b == 0) {
				if(strcmp(buffer, "S1") == 0) {
					pthread_create(&dyn, NULL, printStatus, "Switch 1");
					pthread_join(dyn, 0);
					num += 1;
				} else if(strcmp(buffer, "S2") == 0) {
					pthread_create(&dyn, NULL, printStatus, "Switch 2");
					pthread_join(dyn, 0);
					num += 1;
				}else if(strcmp(buffer, "B1") == 0) {
					pthread_create(&dyn, NULL, printStatus, "Button 1");
					pthread_join(dyn, 0);
					num += 1;
				}else if(strcmp(buffer, "B2") == 0) {
					pthread_create(&dyn, NULL, printStatus, "Button 2");
					pthread_join(dyn, 0);
					num += 1;
				}  	
			}	
		}
	}
	pthread_exit(0);
}
void *rt_Event(void *ptr) {
	if((cdev_id = open(CHAR_DEV, O_RDWR)) == -1) {
		printf("Cannot open device %s\n", CHAR_DEV);
		exit(1);
	}
	int *N_pipe2 = (int *) ptr;
	pthread_t t1;
	while(1) {
		bzero(buffer, 40);
		read(cdev_id, buffer, sizeof(buffer));
		if(buffer[0] != '\0') {
			if(write(N_pipe2[1], buffer, sizeof(buffer)) != sizeof(buffer)) {	//send buffer through pipe
				printf("Pipe time stamp write error\n");
				exit(-1);
			}
		}
	}
	pthread_exit(0);		
}
void *printStatus(void *ptr) {
	char *pin  = (char*) ptr;
	printf("%d\n", *flag);
	if(*flag == 1) {
		gettimeofday(&eT, NULL);
		printf("%s event detected\n", pin);
		printf("Time: %ld.%06ld\n", eT.tv_sec, eT.tv_usec);
		printf("From: %s\n", getIP());		//for testing purposes
		printf("Button 1: %d\n", digitalRead(BUTTON1));
		printf("Button 2: %d\n", digitalRead(BUTTON2));
		printf("Switch 1: %d\n", digitalRead(SW1));
		printf("Switch 2: %d\n", digitalRead(SW2));
		pthread_exit(0);
	}	
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
