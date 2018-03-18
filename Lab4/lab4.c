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
#include <time.h>

#define BTN1 27


typedef struct data {
	struct timeval tvRT;
	unsigned char prevval;
	struct timeval tvPrev;
	unsigned char nextval;
	struct timeval tvNext;
	
} Data;
unsigned char val;
struct timeval tvGPS;	//for time stamps
struct timeval tvRT;

void *getData(void *);
void *rt_Event(void *);
void *dynThread(void *);


int main() {
	clear_button();
	int N_pipe2[2];
	if(pipe(N_pipe2) < 0) {
		printf("N_pipe2 creation error\n");
		exit(-1);
	}
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
		while(1) {		//loop to get GPS data and store it in buffer as well as time stamp
			if(read(N_pipe1, &val, sizeof(val)) < 0) {
				printf("N_pipe1 pipe read error\n");
				return 0;
			}
			gettimeofday(&tvGPS, NULL);
			usleep(250000);
		}
		pthread_join(thread0, NULL);
	} else {
		pthread_t thread1;
		pthread_create(&thread1, NULL, rt_Event, &N_pipe2);
		pthread_join(thread1, 0);
	}
	return 0;
}
void *getData(void * ptr) {
	int *N_pipe2 = (int *) ptr;
	while(1) {
		pthread_t dyn;
		Data five;
		int b;
		five.prevval = val;
		gettimeofday(&five.tvPrev, NULL);
		if(read(N_pipe2[0], &five.tvRT, sizeof(tvRT)) < 0) {
				printf("N_pipe2 read error\n");
				exit(-1);
		} else {
			if((b = fork()) < 0) {
				printf("Fork error\n");
				exit(-1);
			}
			if(b == 0) {
				pthread_create(&dyn, NULL, dynThread, &five);
			}		
		}
	}
	pthread_exit(0);
}
void *rt_Event(void * ptr) {
	int *N_pipe2 = (int *) ptr;
	uint64_t num_periods = 0;
	int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);

	struct itimerspec itval;
		itval.it_interval.tv_sec = 0;
		itval.it_interval.tv_nsec = 75000000;
		itval.it_value.tv_sec = 0;
		itval.it_value.tv_nsec = 5000000;

		timerfd_settime(timer_fd, 0, &itval, NULL);
		read(timer_fd, &num_periods, sizeof(num_periods));
		while(1){
			read(timer_fd, &num_periods, sizeof(num_periods));
			if(check_button()) {
				printf("Button Pressed\n");
				gettimeofday(&tvRT, NULL);
				if(write(N_pipe2[1], &tvRT, sizeof(tvRT)) != sizeof(tvRT)) {
					printf("Pipe time stamp write error\n");
					exit(-1);
				}
				clear_button();
			}
			if(num_periods > 1) {
				puts("MISSED WINDOWN\n");
				exit(1);
			}
			usleep(10000);		
		}
		pthread_exit(0);		
}
void *dynThread(void * ptr) {
	Data *five = (Data *) ptr;
	while(1) {
		if(val != five->prevval) {
			five->nextval = val;
			gettimeofday(five->tvNext, NULL);
			break;	
		}
	}
	double slope = (double)(five->nextval - five->prevval) / (double) (five->tvNext.tv_usec - five->tvPrev.tv_usec) ;
	double est = slope * (tvRT.tv_usec - five->tvPrev.tv_usec) + five->prevval;
	printf("Estimated Value: %.2lf\n", est);
	pthread_exit(0);	
}
