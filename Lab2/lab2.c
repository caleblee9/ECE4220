#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
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




void *entireSearch(void *);
void *rowSearch(void *);






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
	clock_t begin = clock();
	pthread_create(&thread1, NULL, (void *)&entireSearch, &thr1);
	pthread_join(thread1, NULL);
	pthread_exit();
	clock_t end = clock();
	double time_spent = (double)(end - begin);

	printf("Times found: %d\n", thr1.counter);
	printf("Time taken: %lf\n", time_spent);
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
	for(i = 0; i < rows; i++) {
		for(j = 0; i < cols; j++) {
			*(row + j) = *(array + h);
			h++;
			thr2.row = row;
			pthread_create(&thread2[i], NULL, (void *)&rowSearch, &thr2);
			pthread_join(thread2[i], NULL);
			pthread_exit();
			
		}					
	}

	

/*
------------------------------------------------------------------
-------------------------EACH COLUMN------------------------------
------------------------------------------------------------------
*/

	free(array);
	fclose(fp);
	return 0;
}



void *entireSearch(void *t1){
	T1 *thr1 = (T1 *)t1;
	int i;
	int counter = 0;
	for(i = 0; i < thr1->size; i++) {
		if(*(thr1->array + i) == thr1->val) {
			counter++;
		}
	}
	*thr1->counter = counter;
}
void *rowSearch(void *t2) {
	T2 *thr2 = (T2 *)t2;
	int i;
	for(i = 0; i < thr2->size; i++) {
		if(*(thr2->row + i) == thr2->val) {
			*(thr2->counter) = *(thr2->counter) + 1;	
		}
	}
}
