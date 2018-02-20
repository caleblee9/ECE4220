#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

typedef struct t1 {
	int val;
	int *counter;		//thread 1 struct for multiple input parameters
	int *array;
	int size;
} T1;




typedef struct t2{
	int val;		//val user enters in command line
	int *counter;		//thread 2 struct
	int *row;	//numbers in the row
	int size;	//size of the row
} T2;



typedef struct t3{
	int val;		//val to check for
	int *counter;		//counter of number of occurrences
	int *col;		//numbers in the columns
	int size;		//size of the columns
} T3;

typedef struct t4{
	int val;		//number to compare to
	int num;		//number we're checking
	int *counter;		//counter for number of occurrences
} T4;


void *entireSearch(void *);	//entire matrix search
void *rowSearch(void *);	//searches rows
void *colSearch(void *);	//searches cols
void *elemCheck(void *);	//checks every element




int main(int argc, char *argv[]) {
	if(argc != 3) {
		printf("Insufficient amount of arguments\n");
		return 0;
	}
	pthread_t thread1;		//create the thread
	int num = atoi(argv[2]);	//number to check for
	
	FILE *fp;
	if((fp = fopen(argv[1], "r")) == NULL ) {
		printf("Cannot open file, please check to make sure file exists\n");	//make sure file can be opened
		return 0;
	}

	int rows = 0;
	fscanf(fp, "%d", &rows);
	int cols = 0;
	fscanf(fp, "%d", &cols);
	int size = rows * cols;		//get number of rows, columns, and number of elements = size

	int *array = malloc(sizeof(int) * size);	//store values in array
	int i;
	for(i = 0; i < size; i++) {
		fscanf(fp, "%d", (array + i));		//scan them in
	}



/*
----------------------------------------------------------------
-----------------------ENTIRE MATRIX----------------------------
----------------------------------------------------------------
*/	
	T1 thr1;		//first thread
	thr1.val = num;
	thr1.array = array;
	thr1.size = size;
	thr1.counter = malloc(sizeof(int));
	(*thr1.counter) = 0;
	clock_t begin = clock();		//start clock
	pthread_create(&thread1, NULL, (void *)&entireSearch, &thr1);//run the search of entire matrix
	pthread_join(thread1, NULL);			//wait until thread1 is complete
	clock_t end = clock();				//end timer
	double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;	//get total time

	system("clear");		
	printf("----Entire Matrix----\n");
	printf("Times found: %d\n", *thr1.counter);	//print results
	printf("Time taken: %lf ms\n", time_spent);
	free(thr1.counter);
/*
--------------------------------------------------------------------
---------------------------EACH ROW---------------------------------
--------------------------------------------------------------------
*/
	pthread_t thread2[rows];	//number of threads per row
	T2 thr2;
	thr2.val = num;
	thr2.size = cols;
	int counter = 0;
	thr2.counter = &counter;	//using pass by reference to store values
	int *row = malloc(sizeof(int) * cols);	//store values in row. number of elements is equal to the number of columns
	int j;
	int h = 0;
	begin = clock();
	for(i = 0; i < rows; i++) {
		for(j = 0; j < cols; j++) {
			*(row + j) = *(array + h);
			h++;	//holds array location
		}				
		thr2.row = row;
		pthread_create(&thread2[i], NULL, (void *)&rowSearch, &thr2);	//for each row, create a thread and search that row
		pthread_join(thread2[i], NULL);		
	}
	end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;


	printf("\n\n----Each Row----\n");
	printf("Times found: %d\n", counter);		//print results
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
	thr3.counter = &counter;	//pass by reference
	int *col = malloc(sizeof(int) * rows);
	h = 0;
	int k = 0;
	begin = clock();
	for(i = 0; i < cols; i++) {
		h = k;
		for(j = 0; j < rows; j++) {
			*(col + j) = *(array + h);
			h = h + rows;	//each of the columns holds the position in the array incrememnted by the number of rows
		}				
		thr3.col = col;
		pthread_create(&thread3[i], NULL, (void *)&colSearch, &thr3);	//for each column, create a thread
		pthread_join(thread3[i], NULL);
		k++;	//which column we're on
	}
	end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;


	printf("\n\n----Each Column----\n");
	printf("Times found: %d\n", counter);			//print stats
	printf("Time taken: %lf ms\n\n\n", time_spent);
	free(col);
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
		pthread_create(&thread4[i], NULL, (void *)&elemCheck, &thr4);	//for each element create a thread
		pthread_join(thread4[i], NULL);	
	}
	end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;


	printf("\n\n----Each Element----\n");
	printf("Times found: %d\n", counter);		//print stats
	printf("Time taken: %lf ms\n\n\n", time_spent);


	free(array);
	fclose(fp);
	return 0;
}



void *entireSearch(void *t1){
	T1 *thr1 = (T1 *)t1;
	int i;
	for(i = 0; i < thr1->size; i++) {
		if(*(thr1->array + i) == thr1->val) {	//if array contains the value, increment the counter
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
