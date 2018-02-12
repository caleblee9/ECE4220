#include <stdlib.h>
#include <stdio.h>
#include <time.h>


typedef struct t1 {
	int val;
	int *counter;
	int *array;
	int size;
} T1;




typedef struct t2{
	int val;
	int *counter;
	int *row;
	int size;
} T2;



typedef struct t3{
	int val;
	int *counter;
	int *col;
	int size;
} T3;

typedef struct t4{
	int val;
	int num;
	int *counter;
} T4;


void *entireSearch(void *);
void *rowSearch(void *);
void *colSearch(void *);
void *elemCheck(void *);




int main(int argc, char *argv[]) {
	pthread_t thread1;
	int num = atoi(argv[2]);
	
	FILE *fp;
	if((fp = fopen(argv[1], "r")) == NULL ) {
		printf("Cannot open file, please check to make sure file exists\n");
		return 0;
	}

	int rows = 0;
	fscanf(fp, "%d", &rows);
	int cols = 0;
	fscanf(fp, "%d", &cols);
	int size = rows * cols;

	int *array = malloc(sizeof(int) * size);
	int i;
	for(i = 0; i < size; i++) {
		fscanf(fp, "%d", (array + i));
	}



/*
----------------------------------------------------------------
-----------------------ENTIRE MATRIX----------------------------
----------------------------------------------------------------
*/
	T1 thr1;
	thr1.val = num;
	thr1.array = array;
	thr1.size = size;
	thr1.counter = malloc(sizeof(int));
	(*thr1.counter) = 0;
	clock_t begin = clock();
	pthread_create(&thread1, NULL, (void *)&entireSearch, &thr1);
	pthread_join(thread1, NULL);
	clock_t end = clock();
	double time_spent = (double)(end - begin);

	system("clear");
	printf("----Entire Matrix----\n");
	printf("Times found: %d\n", *thr1.counter);
	printf("Time taken: %lf ms\n", time_spent);
	free(thr1.counter);
/*
--------------------------------------------------------------------
---------------------------EACH ROW---------------------------------
--------------------------------------------------------------------
*/
	pthread_t thread2[rows];
	T2 thr2;
	thr2.val = num;
	thr2.size = cols;
	int counter = 0;
	thr2.counter = &counter;
	int *row = malloc(sizeof(int) * cols);
	int j;
	int h = 0;
	begin = clock();
	for(i = 0; i < rows; i++) {
		for(j = 0; j < cols; j++) {
			*(row + j) = *(array + h);
			h++;	
		}				
		thr2.row = row;
		pthread_create(&thread2[i], NULL, (void *)&rowSearch, &thr2);
		pthread_join(thread2[i], NULL);		
	}
	end = clock();
	time_spent = (double)(end - begin);


	printf("\n\n----Each Row----\n");
	printf("Times found: %d\n", counter);
	printf("Time taken: %lf ms\n", time_spent);
	free(row);
	

/*
------------------------------------------------------------------
-------------------------EACH COLUMN------------------------------
------------------------------------------------------------------
*/
	pthread_t thread3[cols];
	T3 thr3;
	thr3.val = num;
	thr3.size = rows;
	counter = 0;
	thr3.counter = &counter;
	int *col = malloc(sizeof(int) * rows);
	h = 0;
	int k = 0;
	begin = clock();
	for(i = 0; i < cols; i++) {
		h = k;
		for(j = 0; j < rows; j++) {
			*(col + j) = *(array + h);
			h = h + 15;	
		}				
		thr3.col = col;
		pthread_create(&thread3[i], NULL, (void *)&colSearch, &thr3);
		pthread_join(thread3[i], NULL);
		k++;	
	}
	end = clock();
	time_spent = (double)(end - begin);


	printf("\n\n----Each Column----\n");
	printf("Times found: %d\n", counter);
	printf("Time taken: %lf ms\n\n\n", time_spent);
	free(row);
/*
------------------------------------------------------------------
-------------------------EACH ELEMENT-----------------------------
------------------------------------------------------------------
*/
	pthread_t thread4[size];
	T4 thr4;
	thr4.val = num;
	counter = 0;
	thr4.counter = &counter;
	begin = clock();
	for(i = 0; i < size; i++) {
		thr4.num = *(array + i);
		pthread_create(&thread4[i], NULL, (void *)&elemCheck, &thr4);
		pthread_join(thread4[i], NULL);	
	}
	end = clock();
	time_spent = (double)(end - begin);


	printf("\n\n----Each Element----\n");
	printf("Times found: %d\n", counter);
	printf("Time taken: %lf ms\n\n\n", time_spent);


	free(array);
	fclose(fp);
	return 0;
}



void *entireSearch(void *t1){
	T1 *thr1 = (T1 *)t1;
	int i;
	for(i = 0; i < thr1->size; i++) {
		if(*(thr1->array + i) == thr1->val) {
			(*thr1->counter)++;
		}
	}
}
void *rowSearch(void *t2) {
	T2 *thr2 = (T2 *)t2;
	int i;
	for(i = 0; i < thr2->size; i++) {
		if(*(thr2->row + i) == thr2->val) {
			(*thr2->counter)++;	
		}
	}
}
void *colSearch(void *t3) {
	T3 *thr3 = (T3 *)t3;
	int i;
	for(i = 0; i < thr3->size; i++) {
		if(*(thr3->col + i) == thr3->val) {
			(*thr3->counter)++;	
		}
	}
}
void *elemCheck(void *t4) {
	T4 *thr4 = (T4 *)t4;
	int i;
	if(thr4->val == thr4->num) {
		(*thr4->counter)++;
	}	
}
