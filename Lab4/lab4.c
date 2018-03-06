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


#define BTN1 27
unsigned char val;	//val that we're reading in
struct timeval tv;
sem_t mutex;
void *getData(void *);
void *rt_Event(void *);
int main() {
	int a = fork();
	wiringPiSetup();	//setup Wiring PI
	sem_init(&mutex, 0 , 1);	//initialize semaphore
	pinMode(BTN1, INPUT); //set PinMode for the button
	if (a != 0){
		int N_pipe1;	//named pipe


		if((N_pipe1 = open("/tmp/N_pipe1", O_RDONLY)) < 0) {
			printf("pipe N_pipe1 error\n");		//make sure the pipe can be opened
			return 0;
		}

		pthread_t thread0;
		pthread_create(&thread0, NULL, getData, NULL);
		while(1) {		//loop to get GPS data and store it in buffer as well as time stamp
			if(read(N_pipe1, &val, sizeof(val)) < 0) {
				printf("N_pipe1 pipe read error\n");
				return 0;
			}
			gettimeofday(&tv, NULL);
		
		}
		pthread_join(thread0, NULL);
	} else {
		pthread_t thread1;
		pthread_create(&thread1, NULL, rt_Event, NULL);
		pthread_join(thread1, 0);
	}
	return 0;
}
void *getData(void * ptr) {
	while(1) {
		usleep(250000);
		printf("GPS Data: %d\n", (int)val);
		printf("Seconds: %lf Microseconds: %ld\n", (double)tv.tv_sec, tv.tv_usec);
			
	}
	pthread_exit(0);
}
void *rt_Event(void * ptr) {
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
				gettimeofday(&tv, NULL);
				printf("REAL TIME EVENT SECONDS: %lf MICROSECONDS %ld\n", (double) tv.tv_sec, tv.tv_usec);
			}
			clear_button();
			if(num_periods > 1) {
				puts("MISSED WINDOWN\n");
				exit(1);
			}		
		}
		pthread_exit(0);		
}
