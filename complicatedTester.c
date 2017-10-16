#include "my_pthread.c"
#include <stdio.h>

my_pthread_mutex_t* xlock;
my_pthread_mutex_t* ylock;

void* inc_x(void *x_void_ptr){

	/* increment x to 100 */
	int *x_ptr = (int *)x_void_ptr;
	while(*x_ptr<100){
		my_pthread_mutex_lock(xlock);
		(*x_ptr)++;
		my_pthread_mutex_unlock(xlock);
	};

	return NULL;
}

void* inc_y(void *y_void_ptr){

	/* increment y to 100 */
	int *y_ptr = (int *)y_void_ptr;
	while(*y_ptr<100){
		my_pthread_mutex_lock(ylock);
		(*y_ptr)++;
		my_pthread_mutex_unlock(ylock);
	};

	return NULL;
}

int main(){
	int THREAD_NUM = 6;
	my_pthread_t threads[THREAD_NUM];

	int x = 0, y = 0;
	xlock = malloc(sizeof(my_pthread_mutex_t));
	ylock = malloc(sizeof(my_pthread_mutex_t));
	my_pthread_mutex_init(xlock, NULL);
	my_pthread_mutex_init(ylock, NULL);

	int i;

	for(i=0; i<6; i+=2){
		my_pthread_create(&threads[i], NULL, &inc_x, &x);
		my_pthread_create(&threads[i+1], NULL, &inc_y, &y);
	}

	for(i=0; i<THREAD_NUM; i++){
		my_pthread_join(threads[i], NULL);
	}

	my_pthread_mutex_destroy(xlock);
	my_pthread_mutex_destroy(ylock);

	printf("Final values: x: %d y: %d\n", x, y);	//300, 300
	
	return 0;
}