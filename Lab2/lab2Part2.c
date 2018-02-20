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

#define HP 51

void *readLine(void*);  //grabs the line from the file, stores it in the buffer
void *storeLine(void*);	//takes the line from the buffer and stores in the string array

typedef struct readThread {
	FILE *fp;		//file to grab line from
	int start;
	int period;
	char *buffer;
	int num;
} RT;
typedef struct readBuffer {
	char *buffer;
	char array[20][50];
}RB;
int main(int argc, char *argv[]) {
	FILE *fp1;
	FILE *fp2;

	if((fp1 = fopen("first.txt", "r")) == NULL) {
		printf("Cannot open first file specified, please check to make sure file exists\n");
		return 0;
	} 
	if((fp2 = fopen("second.txt", "r")) == NULL) {
		printf("Cannot open second file specified, please check to make sure file exists\n");
		return 0;
	} 


	pthread_t thr1, thr2, thr3;
	char *buffer = malloc(sizeof(char) * 50);


	RT t1;
	t1.fp = fp1;
	t1.period = 800000000;
	t1.start = 200000000;
	t1.buffer = buffer;
	t1.num = 1;


	RT t2;
	t2.fp = fp2;
	t2.period = 800000000;
	t2.start = 600000000;
	t2.buffer = buffer;
	t2.num = 2;


	RB rb1;
	rb1.buffer = buffer;
	
	pthread_create(&thr1, NULL, readLine, &t1);	
	pthread_create(&thr2, NULL, readLine, &t2);
	pthread_create(&thr3, NULL, storeLine, &rb1);

	pthread_join(thr1, NULL);
	pthread_join(thr2, NULL);
	pthread_join(thr3, NULL);

//	system("clear");
	int i;
	for(i = 0; i < 20; i++) {
		printf("%s", rb1.array[i]);
	}

	printf("\n");
	free(buffer);
	fclose(fp1);
	fclose(fp2);
	return 0;
}
void *readLine(void* t1){
	RT *thr1 = (RT *)t1;
	int i;
	uint64_t num_periods = 0;
	struct sched_param param;	
	param.sched_priority = HP;
	sched_setscheduler(0, SCHED_FIFO, &param);


	int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);

	struct itimerspec itval;
		itval.it_interval.tv_sec = 0;
		itval.it_interval.tv_nsec = thr1->period;
		itval.it_value.tv_sec = 0;
		itval.it_value.tv_nsec = thr1->start;

		timerfd_settime(timer_fd, 0, &itval, NULL);
	for(i = 0; i < 10; i++) {
		fgets(thr1->buffer, 50, thr1->fp);
		read(timer_fd, &num_periods, sizeof(num_periods));
		if(num_periods > 1) {
			puts("MISSED WINDOW\n");
			exit(1);
		}	
	}
}
void *storeLine(void* rb){
	RB *rb1 = (RB *) rb;
	int i;	
	uint64_t num_periods = 0;
	struct sched_param param;	
	param.sched_priority = HP;
	sched_setscheduler(0, SCHED_FIFO, &param);
	int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
	struct itimerspec itval;
	itval.it_interval.tv_sec = 0;
	itval.it_interval.tv_nsec = 400000000;
	itval.it_value.tv_sec = 0;
	itval.it_value.tv_nsec = 400000000;
	timerfd_settime(timer_fd, 0, &itval, NULL);
	for(i = 0; i < 20; i++){
		strcpy(rb1->array[i], rb1->buffer);
		read(timer_fd, &num_periods, sizeof(num_periods));
		if(num_periods > 1) {
			puts("MISSED WINDOW\n");
			exit(1);
		}
	
	}
}
