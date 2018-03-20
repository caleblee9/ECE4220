#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <wiringPi.h>
#include <semaphore.h>
#include <unistd.h>
#include "ece4220lab3.h"
#include <sys/timerfd.h>
#include <sched.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>

#define BTN1 27


typedef struct data {
	struct timeval tvRT;
	unsigned char prevval;
	struct timeval tvPrev;	//struct to send data to
	unsigned char nextval;
	struct timeval tvNext;
	
} Data;
void *getData(void *);
void *rt_Event(void *);
void *dynThread(void *);

static unsigned char *val;
static struct timeval *tvRT;	//buffers
struct timeval *tvGPS;
int main() {
	
	val = mmap(NULL, sizeof(*val), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);	
	tvRT = mmap(NULL, sizeof(*tvRT), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);	//mmap to share memory between parent and children
	tvGPS = mmap(NULL, sizeof(*tvGPS), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);


	clear_button();	//make sure button is off
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
	wiringPiSetup();	//setup Wiring PI
	pinMode(BTN1, INPUT); //set PinMode for the button
	pullUpDnControl(BTN1, PUD_DOWN);

	if (a != 0){
		int N_pipe1;	//named pipe


		if((N_pipe1 = open("/tmp/N_pipe1", O_RDONLY)) < 0) {
			printf("pipe N_pipe1 error\n");		//make sure the pipe can be opened
			return 0;
		}

		pthread_t thread0;
		pthread_create(&thread0, NULL, getData, N_pipe2);
		while(1) {		//main loop to get GPS data and store it in buffer as well as time stamp
			if(read(N_pipe1, val, sizeof(*val)) < 0) {
				printf("N_pipe1 pipe read error\n");	
				return 0;
			} else{
				gettimeofday(tvGPS, NULL);
				usleep(250000);
			}
		}
		pthread_join(thread0, NULL);
	} else {
		pthread_t thread1;
		pthread_create(&thread1, NULL, rt_Event, &N_pipe2);	//rt Event thread
		pthread_join(thread1, 0);
	}
	return 0;
}

/*
----------------------------------------------------------------------------------
----------------------------------------PTHREAD0----------------------------------
----------------------------------------------------------------------------------
*/
void *getData(void * ptr) {
	int *N_pipe2 = (int *) ptr;
	while(1) {
		pthread_t dyn;
		Data five;
		int b;
		if(read(N_pipe2[0], tvRT, sizeof(*tvRT)) < 0) {
				printf("N_pipe2 read error\n");	//if receive data through pipe
				exit(-1);
		} else {
			if((b = fork()) < 0) {
				printf("Fork error\n");	//create child
				exit(-1);
			}
			if(b == 0) {
				five.tvRT = *tvRT;	//create thread to process data
				pthread_create(&dyn, NULL, dynThread, &five);
			}		
		}
	}
	pthread_exit(0);
}
/*
--------------------------------------------------------------------------------------------
---------------------------------------PTHREAD1---------------------------------------------
--------------------------------------------------------------------------------------------
*/
void *rt_Event(void * ptr) {
	int *N_pipe2 = (int *) ptr;
	uint64_t num_periods = 0;
	int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
	struct itimerspec itval;
	itval.it_interval.tv_sec = 0;
	itval.it_interval.tv_nsec = 75000000;
	itval.it_value.tv_sec = 0;			//set period to 75 ms and start time to 50 ms 
	itval.it_value.tv_nsec = 5000000;
	timerfd_settime(timer_fd, 0, &itval, NULL);
	read(timer_fd, &num_periods, sizeof(num_periods));
	while(1){
		read(timer_fd, &num_periods, sizeof(num_periods));
		if(check_button()) {
			gettimeofday(tvRT, NULL);
			if(write(N_pipe2[1], tvRT, sizeof(*tvRT)) != sizeof(*tvRT)) {	//send timestamp of rtEvent through pipe
				printf("Pipe time stamp write error\n");
				exit(-1);
			}
			clear_button();
		}
		if(num_periods > 1) {
			puts("MISSED WINDOWN\n");
			exit(1);
		}
	}
	pthread_exit(0);		
}
/*
---------------------------------------------------------------------
------------------------------CHILD THREADS--------------------------
---------------------------------------------------------------------
*/
void *dynThread(void * ptr) {
	Data *five = (Data *) ptr;
	five->prevval = *val;		//get before vals for the rt Event
	five->tvPrev = *tvGPS;
	while(1) {
		if(*val != five->prevval && *val != 0) {
			five->nextval = *val;			//get next vals through the event
			gettimeofday(&five->tvNext, NULL);
			break;	
		}
	}
	struct timeval diff;
	timersub(&five->tvNext, &five->tvPrev, &diff);
	double valDiff = (double) five->nextval - five->prevval;	
								//calculate information
	double slope = (valDiff) / (diff.tv_usec);
	timersub(&five->tvRT, &five->tvPrev, &diff);
	unsigned char est = (double) (slope * diff.tv_usec) + (double) five->prevval;


	printf("Previous Event Time: %ld\n", five->tvPrev);
	printf("Previous Event GPS Value: %d\n", five->prevval);
	printf("Push button event time: %ld\n", five->tvRT);
	printf("Estimated GPS Value: %d \n", est);		//print information
	printf("After Event Time: %ld \n", five->tvNext);
	printf("After Event GPS Value: %d\n",five->nextval);
	pthread_exit(0);	
}
