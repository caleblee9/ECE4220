/*
 ============================================================================
 Name        : onOffLab1.c
 Author      : Caleb Lee
 Version     :
 Copyright   : 
 Description : Turns LEDs off and on every second
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>

#define LED1 7		//define ports
#define LED2 21

int main(void) {

	wiringPiSetup();

	pinMode(LED1, OUTPUT);		//LEDs set to output
	pinMode(LED2, OUTPUT);
	while(1) {
		
		digitalWrite(LED1, HIGH);	//LED1 on
		sleep(1);
		digitalWrite(LED1, LOW);	//LED1 off
		digitalWrite(LED2, HIGH);	//LED2 on
		sleep(1);
		digitalWrite(LED2, LOW);	//LED1 off
		
		
	}
	return EXIT_SUCCESS;
}
