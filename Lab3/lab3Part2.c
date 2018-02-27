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
#include <semaphore.h>

#define RED 8		//RED LED wiringPi
#define YELLOW 9	//YELLOW LED wiringPi
#define GREEN 7		//GREEN LED wiring PI
#define BTN1 27
#define HP 51		//raise priority

sem_t mutex;

typedef struct lThread {
	int pri;
	int light;		//thread for green and yellow LEDs
	char *led;
} lThread;

void *lightOn(void *);
void *btnPress(void *);
int main(int argc, char *argv[]) {
	
	if(argc != 4) {
		printf("Invalid argument count, please use format ./a.out <p1> <p2> <p3>\n");	//proper arg count
		return 0;
	}

	int P1 = atoi(argv[1]);
	int P2 = atoi(argv[2]);		//get priorities
	int P3 = atoi(argv[3]);

	wiringPiSetup();
	
	pinMode(RED, OUTPUT);
	pinMode(YELLOW, OUTPUT);
	pinMode(GREEN, OUTPUT);
	pinMode(BTN1, INPUT);
	pinMode(21, OUTPUT);

	digitalWrite(RED, LOW);
	digitalWrite(GREEN, LOW);
	digitalWrite(YELLOW, LOW);	//turn off all LEDs at start
	digitalWrite(21, LOW);
	
	sem_init(&mutex, 0, 1);		//intialize semaphore to 1

	lThread green;
	green.pri = P1;
	green.light = GREEN;
	green.led = "GREEN";
					//setup yellow and green leds

	lThread yellow;
	yellow.pri = P2;
	yellow.light = YELLOW;
	yellow.led = "YELLOW";

	system("clear");
	pthread_t thr1, thr2, thr3;

	pthread_create(&thr1, NULL, lightOn, &green);	//green led
	pthread_create(&thr2, NULL, lightOn, &yellow);	//yellow led	
	pthread_create(&thr3, NULL, btnPress, &P3);	//red led
							//create and join threads
	pthread_join(thr1, NULL);
	pthread_join(thr2, NULL);
	pthread_join(thr3, NULL);
	sem_destroy(&mutex);
	return 0;		
}
void *lightOn(void *t){
	lThread *thr = (lThread *)t;

	struct sched_param param;	
	param.sched_priority = thr->pri;		//set priority
	sched_setscheduler(0, SCHED_FIFO, &param);
	while(1) {
		sem_wait(&mutex);	//wait for semaphore
		printf("%s\n", thr->led);
		digitalWrite(thr->light, HIGH);		//turn on LED
		sleep(2);	
		digitalWrite(thr->light, LOW);		//turn off LED
		sem_post(&mutex);
		usleep(10000);		//sleep to give time to schedule
	}
	pthread_exit(0);
}
void *btnPress(void *prior) {
	int *pri = (int *)prior;
	struct sched_param param;	
	param.sched_priority = *pri;
	sched_setscheduler(0, SCHED_FIFO, &param);
	while(1) {
		sem_wait(&mutex);
		if(check_button()) {		//only if button was pressed
			printf("RED\n");
			digitalWrite(RED, HIGH);	//turn on Red led
			sleep(2);
			digitalWrite(RED, LOW);		//turn off red led
			clear_button();		//clear button flag
		}	
		sem_post(&mutex);
		usleep(10000);
	}
	pthread_exit(0);
}
