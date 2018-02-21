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
	int light;
	char *led;
} lThread;

void *lightOn(void *);
void *btnPress(void *);
int main(int argc, char *argv[]) {

	int P1 = atoi(argv[1]);
	int P2 = atoi(argv[2]);
	int P3 = atoi(argv[3]);

	wiringPiSetup();
	
	pinMode(RED, OUTPUT);
	pinMode(YELLOW, OUTPUT);
	pinMode(GREEN, OUTPUT);
	pinMode(BTN1, INPUT);

	digitalWrite(RED, LOW);
	digitalWrite(GREEN, LOW);
	digitalWrite(YELLOW, LOW);
	
	sem_init(&mutex, 0, 1);	

	lThread green;
	green.pri = P1;
	green.light = GREEN;
	green.led = "GREEN";
	

	lThread yellow;
	yellow.pri = P2;
	yellow.light = YELLOW;
	yellow.led = "YELLOW";

	pthread_t thr1, thr2, thr3;

	while(1) {
		pthread_create(&thr1, NULL, lightOn, &green);
		pthread_create(&thr2, NULL, lightOn, &yellow);
		pthread_create(&thr3, NULL, btnPress, &P3);
	
		pthread_join(thr1, NULL);
		pthread_join(thr2, NULL);
		pthread_join(thr3, NULL);
	}
	sem_destroy(&mutex);
	return 0;		
}
void *lightOn(void *t){
	lThread *thr = (lThread *)t;

	struct sched_param param;	
	param.sched_priority = thr->pri;
	sched_setscheduler(0, SCHED_OTHER, &param);

	sem_wait(&mutex);
	printf("%s\n", thr->led);
	digitalWrite(thr->light, HIGH);
	sleep(3);
	digitalWrite(thr->light, LOW);
	sem_post(&mutex);
	pthread_exit(0);
}
void *btnPress(void *prior) {
	int *pri = (int *)prior;
	struct sched_param param;	
	param.sched_priority = *pri;
	sched_setscheduler(0, SCHED_OTHER, &param);
	sem_wait(&mutex);
	if(check_button()) {
		digitalWrite(RED, HIGH);
		sleep(2);
		digitalWrite(RED, LOW);
		clear_button();
	}
	sem_post(&mutex);
	pthread_exit(0);
}
