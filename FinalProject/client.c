#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <wiringPi.h>
#include <time.h>
#include <pthread.h>

#define CHAR_DEV "/dev/Final" // "/dev/YourDevName"
#define BUTTON1 27	//define ports
#define BUTTON2 0
#define SW1 26
#define SW2 23
#define RED 8		//RED LED wiringPi
#define YELLOW 9	//YELLOW LED wiringPi
#define GREEN 7		//GREEN LED wiringPi
#define dec7E 29
#define dec7A 5
#define dec7B 6
#define dec7C 25
#define dec7D 2

int lNum = 0; //holds place for list
struct timeval eT;	//event time
static char buffer[40];	//character device buffer
char list[32][70] = {"\0"};//holds events that occured within interval
int sock;
unsigned int length;
struct sockaddr_in server, from;  //for server user
struct hostent *hp;
char *port;
char *portfield;
double eTime;
//ADC


#define SPI_CHANNEL 0	// 0 or 1
#define SPI_SPEED 2000000	// Max speed is 3.6 MHz when VDD = 5 V
#define ADC_CHANNEL 2	// Between 0 and 3


uint16_t get_ADC(int channel);	// prototype

float b4[5];
float checkZero[10];
int zero = 0;
int out = 0;
float past = 0.0;
float now = 0.0;
int j = 0;

void check(float);
void *ADC(void *);
//ADC

void error(const char *);  //for printing errors
void printStatus();	//debugging
void *rt_Event(void *);	//button/switch pushes
void *sendEvents(void *); //logging of events to historian
char *getIP();	//getting IP dynamically
void *receiveEvents(void *);	//for when historian calls for an LED to switch on/off
int cdev_id;	//character device
void format(char *);	//formating for the historian

int main(int argc, char *argv[]) {
	if (argc != 3)
   	{
	   	printf("usage %s hostname port\n", argv[0]); //make sure user enters correct commandline arguements
       		exit(1);
  	}
	wiringPiSetup();
	pinMode(dec7E, OUTPUT);
	pinMode(dec7A, OUTPUT);
	pinMode(dec7B, OUTPUT);		//dec 7 segment
	pinMode(dec7C, OUTPUT);
	pinMode(dec7D, OUTPUT);
	
	digitalWrite(dec7E, HIGH);
	digitalWrite(dec7A, LOW);
	digitalWrite(dec7B, LOW);		//set initial display to "0"
	digitalWrite(dec7C, LOW);
	digitalWrite(dec7D, LOW);

	port = argv[1];
	portfield = argv[2];
/*
--------------------------------------------------------------------------------
-----------------------------------SERVER SETUP---------------------------------
--------------------------------------------------------------------------------
*/
	sock = socket(AF_INET, SOCK_DGRAM, 0); // Creates socket. Connectionless.
   	if (sock < 0)
	   	error("socket");

   	server.sin_family = AF_INET;		// symbol constant for Internet domain
   	hp = gethostbyname(port);		// converts hostname input
   	if (hp == 0)
		error("Unknown host");

  	bcopy((char *)hp->h_addr, (char *)&server.sin_addr, hp->h_length);
   	server.sin_port = htons(atoi(portfield));	// port field
   	length = sizeof(struct sockaddr_in);		// size of structure


/*
-----------------------------------------------------------------------
----------------------------EVENTS SETUP-------------------------------
-----------------------------------------------------------------------
*/
	wiringPiSetup();
	if((cdev_id = open(CHAR_DEV, O_RDONLY)) == -1) {
		printf("Cannot open device %s\n", CHAR_DEV);		//see if character device can be accessed
		exit(1);
	}

	pthread_t t1, t2, t3, t4;
	pthread_create(&t1, NULL, rt_Event, NULL);		//button press and switches
	pthread_create(&t2, NULL, sendEvents, NULL);		//threads to run "RTU"
	pthread_create(&t3, NULL, receiveEvents, NULL);		//LED events
	pthread_create(&t4, NULL, ADC, NULL);			//ADC events
	pthread_join(t1, 0);
	pthread_join(t2, 0);
	pthread_join(t3, 0);
	pthread_join(t4, 0);
   	close(sock);
	return 0;
}
/*
-------------------------------------------------------------------------
-------------------------EVENT LOGGING PART 1----------------------------
-------------------------------------------------------------------------
*/
void *rt_Event(void *ptr) {
	while(1) {
		bzero(buffer, 40);
		read(cdev_id, buffer, sizeof(buffer));	//read in character device if button or switch is activated
		if(buffer[0] != '\0') {
			if(strcmp(buffer, "S1") == 0) {
				digitalWrite(dec7A, HIGH);
				digitalWrite(dec7B, HIGH);	//switch 1 displays 3 on segment display
				digitalWrite(dec7C, LOW);
				digitalWrite(dec7D, LOW);

				format("SW1");
			} else if(strcmp(buffer, "S2") == 0) {
				digitalWrite(dec7A, LOW);
				digitalWrite(dec7B, LOW);		//displayer 4 on segment display
				digitalWrite(dec7C, HIGH);
				digitalWrite(dec7D, LOW);

				format("SW2");
			} else if(strcmp(buffer, "B1") == 0) {
				digitalWrite(dec7A, HIGH);
				digitalWrite(dec7B, LOW);
				digitalWrite(dec7C, LOW);		//button 1 push event displays a 1
				digitalWrite(dec7D, LOW);

				format("BT1");
			} else if(strcmp(buffer, "B2") == 0) {
				digitalWrite(dec7A, LOW);
				digitalWrite(dec7B, HIGH);		//button 2 push event displays a 2
				digitalWrite(dec7C, LOW);
				digitalWrite(dec7D, LOW);

				format("BT2");
			} 
		}
	}
	pthread_exit(0);		
}
/*
-------------------------------------------------------------------------
-------------------------EVENT LOGGING PART 2----------------------------
-------------------------------------------------------------------------
*/

void *sendEvents(void *ptr) {
	int n;
	char msg[70];

   	int i;
	uint64_t num_periods = 0;
	int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
	struct itimerspec itval;
	itval.it_interval.tv_sec = 1;
	itval.it_interval.tv_nsec = 0;
	itval.it_value.tv_sec = 0;			//set period to 1ms and start time to 50 ms 
	itval.it_value.tv_nsec = 1000000;
	timerfd_settime(timer_fd, 0, &itval, NULL);
	read(timer_fd, &num_periods, sizeof(num_periods));
	while(1){
		read(timer_fd, &num_periods, sizeof(num_periods));
		i = 0;
			while(strcmp(list[i], "\0") != 0) { //if the list of events actually has an event
			bzero(msg, 70); //refresh buffer
			strcpy(msg, list[i]);	//get the event
			n = sendto(sock, msg, 70, 0, (const struct sockaddr *)&server,length);//send event information to historian
   			if (n < 0)
	   			error("Sendto");
			bzero(list[i], 70); //see if there are more events
			i++;
		}
		lNum = 0;
		if(num_periods > 1) {
			puts("MISSED WINDOWN\n");
			exit(1);
		}	
	}
	
	pthread_exit(0);
}
/*
-------------------------------------------------------------------------
-------------------------DYNAMIC IP GENERATION---------------------------
-------------------------------------------------------------------------
*/

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
/*
-------------------------------------------------------------------------
---------------------------STATUS PRINTING-------------------------------
-------------------------------------------------------------------------
*/

void format(char *event) {
	gettimeofday(&eT);
	eTime = eT.tv_sec * (uint64_t)1000000 + (eT.tv_usec); //total time
	sprintf(list[lNum++], "%s %s %f %d %d %d %d %d %d %d %f", event, getIP(), eTime, digitalRead(BUTTON1),
	digitalRead(BUTTON2),digitalRead(SW1),digitalRead(SW2),digitalRead(RED),digitalRead(YELLOW),digitalRead(GREEN), now);
	fflush(stdout);		
}


void error(const char *msg)
{
    perror(msg);		//error printing
    exit(0);
}
/*
-------------------------------------------------------------------------
---------------------------LED EVENT RECEPTION---------------------------
-------------------------------------------------------------------------
*/

void *receiveEvents(void *ptr) {
	int n;
	char msg1[40];
	while(1) {
		bzero(msg1, 40);
		n = recvfrom(sock, msg1, 40, 0, (struct sockaddr *)&from, &length);
		if (n < 0) {
			printf("Recieve error\n");
			exit(0);
		}	
		if(strcmp(msg1, "R") == 0) {
			digitalWrite(dec7A, HIGH);
			digitalWrite(dec7B, LOW);		//whenever RED event detected, 5 shows
			digitalWrite(dec7C, HIGH);
			digitalWrite(dec7D, LOW);

			if(digitalRead(RED) == 0) {
				digitalWrite(RED, HIGH);
			} else {
				digitalWrite(RED, LOW);
			}
			format("RED");
		} else if(strcmp(msg1, "Y") == 0) {
			digitalWrite(dec7A, LOW);
			digitalWrite(dec7B, HIGH);
			digitalWrite(dec7C, HIGH);		//yellow event shows a 6
			digitalWrite(dec7D, LOW);

			if(digitalRead(YELLOW) == 0) {
				digitalWrite(YELLOW, HIGH);
			} else {
				digitalWrite(YELLOW, LOW);
			}
			format("YELLOW");
		} else if(strcmp(msg1, "G") == 0) {
			digitalWrite(dec7A, HIGH);
			digitalWrite(dec7B, HIGH);		//green event shows a 7
			digitalWrite(dec7C, HIGH);
			digitalWrite(dec7D, LOW);

			if(digitalRead(GREEN) == 0) {
				digitalWrite(GREEN, HIGH);
			} else {
				digitalWrite(GREEN, LOW);
			}
			format("GREEN");
		}
	}
	pthread_exit(0);
}
void *ADC(void *ptr){
	uint16_t ADCvalue;
	double voltage = 0;
	// Configure the SPI
	if(wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) < 0) {
		error("wiringPiSPISetup failed\n");

	}
	// Loop that constantly reads the converted value from the selected channel, and
	// prints it to the screen.
	// This is a simple test, with a sampling frequency of ~1 Hz. Remember that sleep()
	// is not the most accurate function...



	while(1){
		ADCvalue = get_ADC(ADC_CHANNEL);
		voltage = (double) ((ADCvalue * 3.3 ) / 1023); //convert digital to actual voltage
		check(voltage);
		usleep(500);	//sample every 500 microseconds or 2000 times per second
	}
	pthread_exit(0);

}



void check(float ADC){
	if(ADC > 3.18 || ADC < 1.70){ //if outside bounds
		if(out == 1){	//whenever first breaking the bounds
			digitalWrite(dec7A, LOW);
			digitalWrite(dec7B, LOW);		//add ADC events represented by 8 on segment display
			digitalWrite(dec7C, LOW);
			digitalWrite(dec7D, HIGH);

			past = ADC;
			now = ADC;
			checkZero[j] = ADC;	//check for no power
			if(checkZero[0] == ADC && checkZero[1] == ADC && checkZero[2] == ADC && checkZero[3] == ADC && checkZero[4] == ADC){
				zero = 1; //if 5 reads in a row are the same, assume "no power"
			}
			if(j == 9){
				j = 0;
			} else {
				j++;
			}

		} else if(out == 0){
			digitalWrite(dec7A, LOW);
			digitalWrite(dec7B, LOW);
			digitalWrite(dec7C, LOW);
			digitalWrite(dec7D, HIGH);

			now = ADC;
			format("ADC-Out"); //event is logged for going out of bounds
			out = 1;

		}
	} else {
	 	if(out == 1){
			if(zero == 1) {
				digitalWrite(dec7A, LOW);
				digitalWrite(dec7B, LOW);
				digitalWrite(dec7C, LOW);
				digitalWrite(dec7D, HIGH);

				now = 0;
				format("ADC-Zero");	//event is logged whenever it is no power
				zero = 0;
			}
			digitalWrite(dec7A, LOW);
			digitalWrite(dec7B, LOW);
			digitalWrite(dec7C, LOW);
			digitalWrite(dec7D, HIGH);

			now = ADC;
			format("ADC-Back");	//event is logged whenver voltage starts to go back into bounds
			out = 0;
		}
		checkZero[j] = ADC;	//for checking to see if no power occurs
	}
}

uint16_t get_ADC(int ADC_chan)
{
	uint8_t spiData[3];
	spiData[0] = 0b00000001; // Contains the Start Bit
	spiData[1] = 0b10000000 | (ADC_chan << 4);	// Mode and Channel: M0XX0000
												// M = 1 ==> single ended
									// XX: channel selection: 00, 01, 10 or 11
	spiData[2] = 0;	// "Don't care", this value doesn't matter.

	// The next function performs a simultaneous write/read transaction over the selected
	// SPI bus. Data that was in the spiData buffer is overwritten by data returned from
	// the SPI bus.
	wiringPiSPIDataRW(SPI_CHANNEL, spiData, 3);

	// spiData[1] and spiData[2] now have the result (2 bits and 8 bits, respectively)

	return ((spiData[1] << 8) | spiData[2]);
}
