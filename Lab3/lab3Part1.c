#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <stdint.h>
#include <sys/timerfd.h>
#include <time.h>
#include <pthread.h>
#include <wiringPi.h>
#include "ece4220lab3.h"

#define RED 8		//RED LED wiringPi
#define YELLOW 9	//YELLOW LED wiringPi
#define GREEN 7		//GREEN LED wiring PI

#define HP 51		//raise priority

void *trafficLight(void *);

int main(void) {

	wiringPiSetup();
	clear_button();


	pinMode(RED, OUTPUT);
	pinMode(YELLOW, OUTPUT);	//pin MODEs
	pinMode(GREEN, OUTPUT);
	pinMode(21, OUTPUT);


	pinMode(27, INPUT);
	
	digitalWrite(GREEN, LOW);
	digitalWrite(YELLOW, LOW);
	digitalWrite(RED, LOW);		//turn LEDs off
	digitalWrite(21, LOW);

	pthread_t thr1;
	pthread_create(&thr1, NULL, trafficLight, NULL);	//create thread, join thread
	pthread_join(thr1, NULL);
	
	
return 0;
}
void *trafficLight(void * arg) {
	struct sched_param param;	
	param.sched_priority = HP;		//elevate priority
	sched_setscheduler(0, SCHED_FIFO, &param);


	while(1) {
		digitalWrite(GREEN, HIGH);
		sleep(3);
		digitalWrite(GREEN, LOW);
		digitalWrite(YELLOW, HIGH);		//turn green on, off, yellow on, yellow off
		sleep(3);
		digitalWrite(YELLOW, LOW);
		if(check_button()) {		//if button has been pressed
			digitalWrite(RED, HIGH);
			sleep(2);			//turn red on, and then off
			digitalWrite(RED, LOW);
			clear_button();
		}
	}
	pthread_exit(0);		//exit thread
}
