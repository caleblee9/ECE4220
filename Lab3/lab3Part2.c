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
#define BTN1 27
#define HP 51		//raise priority

void *lightOn(void *);
int main(int argc, char *argv[]) {

	wiringPiSetup();	

	pthread_t thr1, thr2, thr3;
		
}
void *lightOn(void *LED);
