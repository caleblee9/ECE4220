/*
 ============================================================================
 Name        : onOffLab1.c
 Author      : Caleb Lee
 Version     :
 Copyright   : 
 Description : Speaker square wave
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>

#define BUTTON1 27	//define ports
#define BUTTON2 0
#define BUTTON3 1
#define BUTTON4 24
#define BUTTON5 28
#define SPEAKER 22


int main(void) {
	wiringPiSetup();
	

	pinMode(BUTTON1, INPUT);
	pinMode(BUTTON2, INPUT);
	pinMode(BUTTON3, INPUT);
	pinMode(BUTTON4, INPUT);	//set button pinmodes
	pinMode(BUTTON5, INPUT);

	pinMode(SPEAKER, OUTPUT);	//speaker mode


	printf("Please enter a number 1 - 5: ");		//user entered number
	int num;
	scanf("%d", &num);
	while(num < 1 || num > 5) {
		printf("Invalid option, please enter 1 -5: ");	//make sure # is 1-5
		scanf("%d", &num);
	}
	while(1) {
		switch(num) {
			case 1:
				if(digitalRead(BUTTON1)) {
					digitalWrite(SPEAKER, HIGH);
					usleep(400);
					digitalWrite(SPEAKER, LOW);
					usleep(400);
				} else {
					break;
				}
			case 2:
				if(digitalRead(BUTTON2)) {
					digitalWrite(SPEAKER, HIGH);
					usleep(400);
					digitalWrite(SPEAKER, LOW);
					usleep(400);		
				} else {
					break;
				}

			case 3:
				if(digitalRead(BUTTON3)) {
					digitalWrite(SPEAKER, HIGH);
					usleep(400);
					digitalWrite(SPEAKER, LOW);		//if user presses equivalent button as number, speaker activates
					usleep(400);		
				} else {
					break;
				}

			case 4:
				if(digitalRead(BUTTON4)) {
					digitalWrite(SPEAKER, HIGH);
					usleep(400);
					digitalWrite(SPEAKER, LOW);
					usleep(400);		
				} else {
					break;
				}

			case 5:
				if(digitalRead(BUTTON5)) {
					digitalWrite(SPEAKER, HIGH);
					usleep(400);
					digitalWrite(SPEAKER, LOW);
					usleep(400);		
				} else {
					break;
				}

		}


	}

	return EXIT_SUCCESS;
}
