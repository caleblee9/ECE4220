#include <stdio.h>
#include <stdlib.h>
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
	char *buffer;		//where to store line
	int start;
	int period;
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
		printf("Cannot open a file specified, please check to make sure file exists\n");
		return 0;
	} 
	if((fp2 = fopen("second.txt", "r")) == NULL) {
		printf("Cannot open a file specified, please check to make sure file exists\n");
		return 0;
	} 


	pthread_t thr1, thr2, thr3;
	char *buffer = malloc(sizeof(char) * 50);


	RT t1;
	t1.fp = fp1;
	t1.buffer = buffer;
	t1.period = 200000000;
	t1.start = 50000000;
	t1.num = 1;


	RT t2;
	t2.fp = fp1;
	t2.buffer = buffer;
	t2.period = 200000000;
	t2.start = 150000000;
	t2.num = 2;


	RB rb1;
	rb1.buffer = buffer;
	
	pthread_create(&thr1, NULL, (void *)&readLine, &t1);
	pthread_create(&thr3, NULL, (void *)&storeLine,&rb1);	
	pthread_create(&thr2, NULL, (void *)&readLine, &t2);
	pthread_join(thr2, NULL);
	pthread_join(thr1, NULL);
	pthread_join(thr3, NULL);

//	system("clear");
	int i;
	for(i = 0; i < 20; i++) {
		if(rb1.array[i] != "") {
			printf("%s", rb1.array[i]);
		}
	}

	printf("\n");
	fclose(fp1);
	fclose(fp2);
	free(buffer);
	return 0;
}
void *readLine(void* t1){
	RT *thr1 = (RT *)t1;
	struct sched_param param;	
	param.sched_priority = HP;
	sched_setscheduler(0, SCHED_FIFO, &param);
	int timer_1 = timerfd_create(CLOCK_MONOTONIC, 0);
	struct itimerspec itval;
	itval.it_interval.tv_sec = 0;
	itval.it_interval.tv_nsec = thr1->period;
	itval.it_value.tv_sec = 0;
	itval.it_value.tv_nsec = thr1->start;
	timerfd_settime(timer_1, 0, &itval, NULL);
	uint64_t num_periods = 0;
	int i;
	for(i = 0; i < 10; i++) {
		printf("Thread %d\n", thr1->num);
		fgets(thr1->buffer, 50, thr1->fp);
		read(timer_1, &num_periods, sizeof(num_periods));
		if(num_periods > 1) {
			puts("MISSED WINDOW\n");
			exit(1);
		}
	}
}
void *storeLine(void* rb){
	RB *rb1 = (RB *) rb;
	struct sched_param param;	
	param.sched_priority = HP;
	sched_setscheduler(0, SCHED_FIFO, &param);
	int timer_1 = timerfd_create(CLOCK_MONOTONIC, 0);
	struct itimerspec itval;
	itval.it_interval.tv_sec = 0;
	itval.it_interval.tv_nsec = 100000000;
	itval.it_value.tv_sec = 0;
	itval.it_value.tv_nsec = 100000000;
	timerfd_settime(timer_1, 0, &itval, NULL);
	uint64_t num_periods = 0;
	int i;
	for(i = 0; i < 20; i++){
		printf("Thread 3\n");
		strcpy(rb1->array[i], rb1->buffer);
		read(timer_1, &num_periods, sizeof(num_periods));
		if(num_periods > 1) {
			puts("MISSED WINDOW\n");
			exit(1);
		}
	}
}
